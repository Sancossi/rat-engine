"""Build the six Canal City source pilots with Blender 5.2.

Run blender --background --python build.py -- --output <NEW directory>.
Outputs are deliberately never overwritten: edited .blend files are artist-owned.
"""
import argparse
import json
import math
from pathlib import Path
import random
import shutil
import sys

import bpy
import numpy as np
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
from catalog import inventory, PILOTS

RNG = random.Random(1909)
CURRENT = None
ROOT = None
MATS = {}
FIGURE_HEIGHTS={"scale_rat_small":1.2,"scale_rat_medium":1.8,"scale_rat_adult":2.8,"scale_rat_high":4.0}


def mesh(name, vertices, faces, material, bevel=0.015, group="structure"):
    data = bpy.data.meshes.new(name)
    data.from_pydata(vertices, [], faces)
    data.update()
    obj = bpy.data.objects.new(name, data)
    CURRENT.objects.link(obj)
    obj.parent = ROOT
    obj["export_group"] = group
    obj.data.materials.append(MATS[material])
    # World-distance planar projection gives consistent texel density on repeated
    # masonry. Each face chooses its dominant normal; the shared maps tile.
    uv = data.uv_layers.new(name="UVMap")
    for polygon in data.polygons:
        normal = polygon.normal
        axes = (1, 2) if abs(normal.x) > 0.5 else ((0, 2) if abs(normal.y) > 0.5 else (0, 1))
        for loop_index in polygon.loop_indices:
            co = data.vertices[data.loops[loop_index].vertex_index].co
            uv.data[loop_index].uv = (co[axes[0]], co[axes[1]])
    if bevel:
        mod = obj.modifiers.new("Hand cut eased edges", "BEVEL")
        mod.width = bevel
        mod.segments = 2
        mod.affect = "EDGES"
        mod = obj.modifiers.new("Weighted corner normals", "WEIGHTED_NORMAL")
        mod.keep_sharp = True
    return obj


def box(name, center, size, material="stone", bevel=0.02, group="structure"):
    x, y, z = center
    a, b, c = (v / 2 for v in size)
    vertices = [(x + dx*a, y + dy*b, z + dz*c) for dz in (-1, 1) for dy in (-1, 1) for dx in (-1, 1)]
    faces = [(0, 2, 3, 1), (4, 5, 7, 6), (0, 1, 5, 4), (2, 6, 7, 3), (0, 4, 6, 2), (1, 3, 7, 5)]
    return mesh(name, vertices, faces, material, bevel, group)


def prism(name, polygon_xz, y, depth, material="stone", bevel=0.018, group="structure"):
    n = len(polygon_xz)
    vertices = [(x, y-depth/2, z) for x, z in polygon_xz] + [(x, y+depth/2, z) for x, z in polygon_xz]
    faces = [tuple(reversed(range(n))), tuple(range(n, 2*n))]
    faces.extend((i, (i+1) % n, (i+1) % n+n, i+n) for i in range(n))
    # Enforce outward winding independently of caller polygon ordering.
    obj = mesh(name, vertices, faces, material, bevel, group)
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(obj.data)
    bm.free()
    return obj


def rod(name, a, b, radius=0.025, material="iron", sides=8, group="ironwork", radius_end=None):
    a, b = Vector(a), Vector(b)
    direction = (b-a).normalized()
    u = direction.cross(Vector((0, 0, 1)))
    if u.length < 0.01:
        u = direction.cross(Vector((0, 1, 0)))
    u.normalize()
    v = direction.cross(u).normalized()
    radius_end = radius if radius_end is None else radius_end
    vertices = []
    for pos, r in ((a, radius), (b, radius_end)):
        vertices.extend(tuple(pos + r*(math.cos(i*2*math.pi/sides)*u + math.sin(i*2*math.pi/sides)*v)) for i in range(sides))
    faces = [tuple(reversed(range(sides))), tuple(range(sides, 2*sides))]
    faces.extend((i, (i+1) % sides, (i+1) % sides+sides, i+sides) for i in range(sides))
    if radius_end==0:
        vertices=vertices[:sides]+[tuple(b)]
        faces=[tuple(reversed(range(sides)))]+[(i,(i+1)%sides,sides) for i in range(sides)]
    return mesh(name, vertices, faces, material, min(.004,(b-a).length*.1,radius*.2), group)


def wire(name, points, radius=0.018, material="brass", group="ironwork", closed=False):
    # Direct tube geometry avoids curve conversion differences between exporters.
    verts, faces = [], []
    for i, point in enumerate(points):
        p = Vector(point)
        previous = Vector(points[(i-1) % len(points)] if i or closed else points[i])
        following = Vector(points[(i+1) % len(points)] if i+1 < len(points) or closed else points[i])
        tangent = (following-previous).normalized()
        u = tangent.cross(Vector((0, 1, 0)))
        if u.length < 0.01:
            u = tangent.cross(Vector((1, 0, 0)))
        u.normalize()
        v = tangent.cross(u).normalized()
        for j in range(6):
            verts.append(tuple(p + radius*(u*math.cos(j*math.tau/6)+v*math.sin(j*math.tau/6))))
    for i in range(len(points) if closed else len(points)-1):
        for j in range(6):
            faces.append((i*6+j, i*6+(j+1)%6, ((i+1)%len(points))*6+(j+1)%6, ((i+1)%len(points))*6+j))
    if not closed:
        faces += [tuple(reversed(range(6))), tuple(range((len(points)-1)*6, len(points)*6))]
    return mesh(name, verts, faces, material, 0, group)


def ring(name, center, radius, tube=0.025, material="brass", group="ironwork"):
    x,y,z = center
    return wire(name, [(x+radius*math.cos(i*math.tau/32), y, z+radius*math.sin(i*math.tau/32)) for i in range(32)], tube, material, group, True)


def brick(name, center, size, group="structure"):
    return box(name, center, size, RNG.choice(["stone", "stone", "stone_light", "stone_dark"]), min(size)*0.065, group)


def masonry(width, height, depth, center=(0,0,0), course=0.25, block=0.5, group="structure"):
    cx,cy,cz = center
    rows = max(1, round(height/course))
    h = height/rows
    for row in range(rows):
        offset = block/2 if row % 2 else 0
        cuts = sorted({-width/2, width/2} | {round(-width/2+offset+i*block,6) for i in range(1, math.ceil(width/block)+1) if -width/2 < -width/2+offset+i*block < width/2})
        for a,b in zip(cuts, cuts[1:]):
            bottom=0 if row==0 else row*h+.009
            top=(row+1)*h-.009
            brick("Ashlar_%02d" % row, (cx+(a+b)/2,cy,cz+(bottom+top)/2), (b-a-.018,depth-.01,top-bottom),group)


def arch_ring(cx, cy, spring, radius, thickness, depth, segments=19, material=None, group="structure"):
    for i in range(segments):
        a,b = i*math.pi/segments+.008, (i+1)*math.pi/segments-.008
        points = [(cx+r*math.cos(t),spring+r*math.sin(t)) for r,t in ((radius,a),(radius,b),(radius+thickness,b),(radius+thickness,a))]
        prism("Radial arch voussoir",points,cy,depth,material or RNG.choice(["stone_light","stone"]),.018,group)


def post(x,y,z,height=1,stone=False):
    if stone:
        masonry(.48,height,.48,(x,y,z),course=.25,block=.5,group="posts")
        for level in (.08,height-.12):
            box("Pier dressed belt",(x,y,z+level),(.58,.58,.1),"stone_light",.015,"posts")
        for dx in (-.17,.17):
            for level in (.3,height-.3):
                rod("Pier anchor boss",(x+dx,y-.245,z+level),(x+dx,y-.275,z+level),.025,"brass",group="posts")
    else:
        box("Iron upright",(x,y,z+height/2),(.09,.09,height),"iron",.01)
        for level in (.08,.2,height-.13):
            box("Gilded collar",(x,y,z+level),(.13,.13,.045),"brass",.008)
    box("Post cap",(x,y,z+height+.025),(.58 if stone else .16,)*2+(.05,),"stone_light" if stone else "brass",.014,"posts")
    rod("Finial",(x,y,z+height+.05),(x,y,z+height+.16),.065,"brass",radius_end=0)


def railing(x=0,y=0,z=0,width=2,height=1):
    # Each bay has a pointed gothic lancet with a nested diamond and scrolls.
    for px in (x-width/2+.07,x+width/2-.07):
        post(px,y,z,height-.17)
    for level in (.14,height-.16,height-.025):
        rod("Handrail datum" if level==height-.025 else "Continuous rail",(x-width/2,y,z+level),(x+width/2,y,z+level),.025,"brass" if level==height-.025 else "iron")
    bays = max(2,round(width/.5))
    for i in range(bays):
        cx=x-width/2+(i+.5)*width/bays
        half=width/bays*.42
        wire("Pointed lancet",[(cx-half,y,z+.15),(cx-half,y,z+.42),(cx-half*.65,y,z+.60),(cx,y,z+.77),(cx+half*.65,y,z+.60),(cx+half,y,z+.42),(cx+half,y,z+.15)],.018,"iron")
        wire("Diamond tracery",[(cx,y-.005,z+.28),(cx-half*.62,y-.005,z+.43),(cx,y-.005,z+.58),(cx+half*.62,y-.005,z+.43)],.012,"brass",closed=True)
        rod("Center spike",(cx,y,z+.58),(cx,y,z+.8),.023,"brass",radius_end=0)
        for sign in (-1,1):
            wire("Forged volute",[(cx+sign*(half*.35+.05*math.cos(t)),y,z+.22+.065*math.sin(t)) for t in np.linspace(0,math.tau*1.1,16)],.01,"iron")
        if height>1.1:
            rod("Extended lancet stem",(cx,y,z+.78),(cx,y,z+height-.16),.018,"iron")
            wire("High guard crown",[(cx-half,y,z+.8),(cx-half*.6,y,z+height-.25),(cx,y,z+height-.16),(cx+half*.6,y,z+height-.25),(cx+half,y,z+.8)],.016,"brass")


def lantern(x=0,y=0,z=0,scale=1):
    s=scale
    for level,w,d,h,mat in [(0.025,.30,.30,.05,"iron"),(.075,.35,.35,.05,"brass"),(.18,.20,.20,.08,"iron"),(.25,.27,.27,.04,"brass"),(.54,.27,.27,.045,"brass")]:
        box("Lantern moulding",(x,y,z+level*s),(w*s,d*s,h*s),mat,.009*s,"lamp")
    box("Amber glass",(x,y,z+.39*s),(.185*s,.185*s,.27*s),"amber",.02*s,"lamp")
    for dx in (-.112,.112):
        for dy in (-.112,.112):
            rod("Lantern corner bar",(x+dx*s,y+dy*s,z+.26*s),(x+dx*s,y+dy*s,z+.53*s),.012*s,"iron",group="lamp")
    rod("Lantern copper roof",(x,y,z+.565*s),(x,y,z+.67*s),.185*s,"brass",sides=4,group="lamp",radius_end=.04*s)
    rod("Lantern crown",(x,y,z+.66*s),(x,y,z+.75*s),.04*s,"iron",group="lamp",radius_end=.012*s)


def canal_wall():
    # Revised overall H4.5 includes .75m lamp fixtures; structural paving H3.75.
    for x in (-3.25,3.25):
        masonry(1.5,3.5,4,(x,0,0),course=.25,block=.5)
    # Central low culvert under a solid quay; arch opening remains actual empty space.
    radius,spring=1.1,.65
    for x in (-1.925,1.925):
        masonry(1.15,3.5,4,(x,0,0),course=.25)
    arch_ring(0,-.0,spring,radius,.32,4,15)
    outer=radius+.32
    for i in range(12):
        a=-outer+i*outer*2/12+.009
        b=-outer+(i+1)*outer*2/12-.009
        za=spring+math.sqrt(max(0,outer*outer-a*a))
        zb=spring+math.sqrt(max(0,outer*outer-b*b))
        top=math.ceil(max(za,zb)/.25)*.25
        prism("Culvert spandrel",[(a,za),(b,zb),(b,top),(a,top)],0,3.99,"stone",.015)
        if top <3.5:
            masonry(b-a,3.5-top,4,((a+b)/2,0,top),course=.25,block=.5)
    # Floor tiles on quay are walkable deck group, separate from retaining walls.
    for ix in range(16):
        for iy in range(8):
            brick("Quay paving",(-3.75+ix*.5,-1.75+iy*.5,3.625),(.485,.485,.25),"deck")
    for x in (-3.6,-1.6,1.6,3.6):
        masonry(.48,3.5,.32,(x,-1.98,0),course=.25,block=.5,group="posts")
        for h in (.14,1.12,2.12,3.38):
            box("Projecting pilaster belt",(x,-1.99,h),(.61,.4,.14),"stone_light",.02,"posts")
            for dx in (-.17,.17):
                rod("Brass anchor",(x+dx,-2.20,h),(x+dx,-2.23,h),.025,"brass",group="posts")
        box("Quay pilaster cap",(x,-1.8,3.625),(.65,.65,.25),"stone_light",.02,"posts")
        lantern(x,-1.8,3.75)
    for x in (-3.6,3.6):
        for h in (1.5,3.3):
            box("Ring plate",(x,-2.065,h),(.22,.055,.3),"iron",.014,"ironwork")
            ring("Mooring ring",(x,-2.14,h-.07),.17,.028)
    # Service ladder offset from the culvert.
    for x in (2.12,2.82):
        rod("Ladder stile",(x,-2.12,.3),(x,-2.12,3.85),.036)
    for h in np.arange(.4,3.85,.3):
        rod("Ladder rung",(2.1,-2.12,float(h)),(2.84,-2.12,float(h)),.025,"brass")


def bridge():
    width,depth,deck=8,3,3
    radius,spring=2.6,.0
    for x in (-3.3,3.3):
        masonry(1.4,deck-.25,depth,(x,0,0),course=.25,block=.5,group="abutments")
    arch_ring(0,0,spring,radius,.35,depth,23,group="arch")
    outer=radius+.35
    # Rectangular spandrel courses fitted to the outer arch profile.
    for i in range(24):
        a=-outer+i*2*outer/24+.008
        b=-outer+(i+1)*2*outer/24-.008
        za=math.sqrt(max(0,outer*outer-a*a))
        zb=math.sqrt(max(0,outer*outer-b*b))
        if max(za,zb)<deck-.08:
            prism("Fitted spandrel",[(a,za),(b,zb),(b,deck-.08),(a,deck-.08)],0,depth-.04,"stone",.013,"arch")
    for ix in range(16):
        for iy in range(6):
            brick("Bridge paving",(-3.75+ix*.5,-1.25+iy*.5,deck+.025),(.488,.488,.15),"deck")
    for y in (-1.48,1.48):
        box("Bridge string course",(0,y,deck-.18),(8,.15,.18),"stone_light",.025,"deck")
        for x in (-3.72,3.72):
            post(x,y,deck+.1,1.1,stone=True)
            lantern(x,y,deck+1.25,scale=1)
        for x in (-2.5,0,2.5):
            railing(x,y,deck+.12,2.25,1)
    # Removable heraldic cloth is a separate export group; no cloth simulation.
    banner=[(-.55,0),(-.55,-1.55),(0,-1.9),(.55,-1.55),(.55,0)]
    prism("Burgundy bridge standard",[(x-2.9,z+3.5) for x,z in banner],-1.7,.035,"burgundy",.012,"banner")
    wire("Banner gold hem",[(x*.9-2.9,-1.725,z*.94+3.46) for x,z in banner],.018,"brass","banner",True)
    rod("Banner rod",(-3.55,-1.71,3.6),(-2.25,-1.71,3.6),.035,"brass",group="banner")
    ring("Faith halo",(-2.9,-1.735,2.8),.21,.021,group="banner")
    rod("Faith staff",(-2.9,-1.735,2.25),(-2.9,-1.735,3.14),.021,"brass",group="banner")
    for angle in (0,math.pi/2):
        wire("Faith star",[(-2.9+.10*math.cos(t+angle),-1.735,2.8+.10*math.sin(t+angle)) for t in (0,math.pi*.5,math.pi,math.pi*1.5)],.012,"brass","banner",True)


def stairs(width=2.4,steps=8,rise=.3,tread=.56,center_x=0,handrails=True):
    for i in range(steps):
        height=(i+1)*rise
        masonry(width,height,tread,(center_x,(i+.5)*tread,0),course=rise,block=.5,group="stair_mass")
        box("Eased stair nosing",(center_x,i*tread+.045,height-.03),(width,.11,.06),"stone_light",.012,"treads")
    if not handrails:
        return
    for x in (center_x-width/2+.08,center_x+width/2-.08):
        for i in (0,steps-1):
            post(x,(i+.5)*tread,(i+1)*rise,.75,stone=False)
        for level in (.42,.72):
            rod("Sloping handrail",(x,.2,rise+level),(x,(steps-.5)*tread,steps*rise+level),.03,"brass")
        for i in range(1,steps-1):
            rod("Stair baluster",(x,(i+.5)*tread,(i+1)*rise),(x,(i+.5)*tread,(i+1)*rise+.69),.02)


def paired_stairs():
    # Small lane is a .8m recess within the 2.4m assembly; its support profile
    # follows the half-steps. This prevents low wooden treads being buried in
    # the solid .30m risers. Main clear lane:1.6m, small useful lane:.6m.
    stairs(width=1.6,center_x=.4,handrails=False)
    for i in range(16):
        top=(i+1)*.15
        y=(i+.5)*.28
        box("Small lane stone support",(-.8,y,(top-.045)/2),(.79,.27,top-.045),"stone_dark",.01,"overlay_support")
        box("Overlay wooden tread",(-.8,y,top-.0225),(.8,.28,.045),"oak",.009,"overlay_treads")
        box("Overlay brass nose",(-.8,i*.28+.012,top-.009),(.78,.024,.018),"brass",.004,"overlay_treads")
        for x in (-1.12,-.48):
            rod("Overlay bolt",(x,y-.06,top-.004),(x,y-.06,top),.015,"brass",group="overlay_treads")
    for x in (-1.18,-.42,1.12):
        for y,z in ((.14,.15),(4.34,2.4)):
            post(x,y,z,.8)
        rod("Paired lane handrail",(x,.14,1.0),(x,4.34,3.25),.025,"brass")


def ellipsoid(name,center,radii,material="fur",group="figure",segments=20,rings=12):
    cx,cy,cz=center
    rx,ry,rz=radii
    vertices=[(cx,cy,cz-rz)]
    for j in range(1,rings):
        phi=-math.pi/2+j*math.pi/rings
        for i in range(segments):
            theta=i*math.tau/segments
            vertices.append((cx+rx*math.cos(phi)*math.cos(theta),cy+ry*math.cos(phi)*math.sin(theta),cz+rz*math.sin(phi)))
    top=len(vertices)
    vertices.append((cx,cy,cz+rz))
    faces=[(0,1+(i+1)%segments,1+i) for i in range(segments)]
    for j in range(rings-2):
        for i in range(segments):
            a=1+j*segments+i
            b=1+j*segments+(i+1)%segments
            faces.append((a,b,b+segments,a+segments))
    faces.extend((top,1+(rings-2)*segments+i,1+(rings-2)*segments+(i+1)%segments) for i in range(segments))
    obj=mesh(name,vertices,faces,material,0,group)
    for face in obj.data.polygons:
        face.use_smooth=True
    return obj


def robe(height,child=False):
    segments=40
    levels=[(.065,.145),(.10,.15),(.28,.125),(.46,.10),(.62,.135),(.73,.155),(.78,.09)]
    vertices=[]
    for z,r in levels:
        for i in range(segments):
            angle=i*math.tau/segments
            pleat=1+.055*math.cos(angle*10)+.02*math.cos(angle*17)
            vertices.append((height*r*math.cos(angle)*pleat,height*r*.78*math.sin(angle)*pleat,height*z))
    faces=[tuple(reversed(range(segments))),tuple(range((len(levels)-1)*segments,len(levels)*segments))]
    for j in range(len(levels)-1):
        for i in range(segments):
            a=j*segments+i
            b=j*segments+(i+1)%segments
            faces.append((a,b,b+segments,a+segments))
    obj=mesh("Pleated travelling robe",vertices,faces,"burgundy",0,"robe")
    for face in obj.data.polygons:
        face.use_smooth=True
    for sign in (-1,1):
        wire("Embroidered robe seam",[(sign*height*x,-height*y,height*z) for x,y,z in ((.045,.08,.76),(.07,.11,.65),(.045,.085,.46),(.07,.11,.28),(.09,.13,.08))],height*.004,"brass","robe")


def ratfolk(height,child=False,tall=False):
    robe(height,child)
    # Feet touch z0 exactly. Body-height measurement excludes the tail/staff group.
    for sign in (-1,1):
        ellipsoid("Rat foot",(sign*.072*height,-.046*height,.032*height),(.045*height,.092*height,.032*height),"fur","body")
        ellipsoid("Draped sleeve",(sign*.145*height,-.005*height,.60*height),(.065*height,.085*height,.155*height),"burgundy","robe")
        wire("Folded forearm sleeve",[(sign*x*height,-y*height,z*height) for x,y,z in ((.18,.03,.54),(.12,.09,.52),(.04,.14,.55))],.035*height,"burgundy","robe")
        ellipsoid("Clasped hand",(sign*.035*height,-.151*height,.553*height),(.037*height,.025*height,.022*height),"fur","body")
    ellipsoid("Raised hood",(0,.024*height,.81*height),(.145*height,.115*height,.15*height),"burgundy","robe")
    head_width=.123 if child else .111
    ellipsoid("Rat head",(0,-.055*height,.859*height),(head_width*height,.122*height,.103*height),"fur","body")
    ellipsoid("Long rat muzzle",(0,-.18*height,.824*height),(.069*height,.145*height,.052*height),"fur_light","body")
    ellipsoid("Dark nose",(0,-.309*height,.829*height),(.039*height,.024*height,.028*height),"iron","body")
    for sign in (-1,1):
        # Ear upper point is exactly1.0*height, making standing height unambiguous.
        ellipsoid("Round rat ear",(sign*.119*height,-.008*height,.942*height),(.074*height,.031*height,.058*height),"fur","body")
        ellipsoid("Ear inset",(sign*.119*height,-.031*height,.942*height),(.052*height,.009*height,.043*height),"ear","body")
        ellipsoid("Black eye",(sign*.078*height,-.142*height,.879*height),(.012*height,.012*height,.016*height),"iron","body",12,8)
        ellipsoid("Eye glint",(sign*.079*height,-.153*height,.883*height),(.004*height,.003*height,.004*height),"stone_light","body",8,6)
        for offset in (-.01,.01):
            wire("Whisker",[(sign*.037*height,-.26*height,(.83+offset)*height),(sign*.15*height,-.27*height,(.838+offset)*height)],height*.0018,"fur_light","body")
    wire("Rat tail",[(0,.075*height,.15*height),(.09*height,.15*height,.08*height),(.23*height,.17*height,.04*height),(.34*height,.10*height,.032*height),(.36*height,0,.038*height),(.31*height,-.04*height,.055*height)],height*.017,"ear","accessories")
    # A small pendant and shoulder clasp lend scale to the silhouette.
    ring("Robe pendant",(0,-.128*height,.675*height),height*.026,height*.004,"brass","robe")
    if tall:
        rod("Walking staff",(.23*height,-.13*height,.0),(.23*height,-.13*height,.86*height),height*.012,"oak",group="accessories")
        ellipsoid("Staff pommel",(.23*height,-.13*height,.86*height),(.026*height,.026*height,.035*height),"brass","accessories")


def gothic_points(width,height,spring,segments=16):
    # Pointed two-centred arch: intersection at apex, spring tangents vertical.
    half=width/2
    radius=((height-spring)**2+half**2)/(2*half)
    center=radius-half
    top_angle=math.atan2(height-spring,-center)
    left=[(center+radius*math.cos(math.pi+(top_angle-math.pi)*i/segments),spring+radius*math.sin(math.pi+(top_angle-math.pi)*i/segments)) for i in range(segments+1)]
    return [(-half,0),(-half,spring)]+left[1:]+[(-x,z) for x,z in reversed(left[:-1])]+[(half,0)]


def door(width=1.6,height=3,depth=.4):
    global ROOT
    # Captioned dimension denotes leaf opening. The removable stone surround is
    # explicit additional envelope, recorded separately in the generated manifest.
    spring=height*.64
    inner=gothic_points(width,height,spring)
    frame=.28
    for x in (-width/2-frame/2,width/2+frame/2):
        masonry(frame,spring,depth,(x,0,0),course=.25,block=.5,group="frame")
        for level in (.12,spring-.14):
            box("Door jamb moulding",(x+math.copysign(.06,x),-.03,level),(.4,depth+.08,.16),"stone_light",.018,"frame")
    # Radial blocks between matching pointed curves, not a filled triangular cap.
    inside=inner[1:-1]
    outside=gothic_points(width+2*frame,height+frame,spring)[1:-1]
    for i in range(len(inside)-1):
        a,b=inside[i],inside[i+1]
        c,d=outside[i+1],outside[i]
        prism("Pointed archivolt",[a,b,c,d],0,depth,"stone_light",.012,"frame")
    hinge=bpy.data.objects.new("door_hinge",None)
    CURRENT.objects.link(hinge)
    hinge.parent=ROOT
    hinge.location=(-width/2,-.035,0)
    hinge["hinge_axis"]="local +Z"
    oldroot=ROOT
    ROOT=hinge
    # Mesh coordinates are hinge local; closed leaf fits the clear aperture.
    poly=[(x+width/2,z+.018) for x,z in gothic_points(width-.035,height-.035,spring)]
    prism("Door leaf",poly,-.055,.13,"oak",.013,"leaf")
    for i in range(9):
        x=.07+i*(width-.14)/8
        localx=x-width/2
        roof=height-(height-spring)*(abs(localx)/(width/2))**1.7
        box("Vertical board joint",(x,-.124,roof/2),(.012,.007,roof-.045),"iron",0,"leaf")
    border=[(x+width/2,-.14,z+.035) for x,z in gothic_points(width-.16,height-.14,spring)]
    wire("Leaf gilded border",border,.014,"brass","leaf",True)
    for level in (.42,1.2,2):
        box("Strap hinge",(.52,-.15,level),(1.08,.05,.07),"iron",.012,"leaf")
        for x in (.12,.42,.72,.98):
            rod("Brass rivet",(x,-.18,level),(x,-.20,level),.025,"brass",sides=8,group="leaf")
        rod("Hinge barrel",(.03,-.05,level-.1),(.03,-.05,level+.1),.045,"brass",group="leaf")
    ring("Door pull",(width-.3,-.2,1.2),.09,.018,group="leaf")
    for cx in (width*.25,width*.5,width*.75):
        wire("Door gothic tracery",[(cx-.16,-.145,1.7),(cx-.16,-.145,2.3),(cx,-.145,2.65),(cx+.16,-.145,2.3),(cx+.16,-.145,1.7)],.018,"brass","leaf")
    ROOT=oldroot
    return hinge


def make_textures(output):
    output.mkdir()
    size=256
    yy,xx=np.mgrid[0:size,0:size].astype(np.float32)/size
    rng=np.random.default_rng(1909)
    noise=rng.random((size,size)).astype(np.float32)
    wave=(np.sin(xx*math.tau*7+np.sin(yy*math.tau*3)) + np.sin(yy*math.tau*11))/2
    fine=(noise-.5)*.08+wave*.035
    definitions={
        "stone":((.12,.155,.18),.87,0),"stone_light":((.19,.225,.25),.83,0),
        "stone_dark":((.075,.098,.115),.9,0),"iron":((.035,.05,.055),.43,.85),
        "brass":((.49,.285,.09),.34,.82),"oak":((.085,.038,.021),.78,0),
        "burgundy":((.16,.023,.045),.94,0),"amber":((1,.3,.025),.26,0),
        "fur":((.17,.18,.19),.9,0),"fur_light":((.27,.28,.28),.87,0),"ear":((.24,.12,.12),.9,0),
    }
    manifests={}
    for name,(color,roughness,metallic) in definitions.items():
        mat=bpy.data.materials.new("CC_"+name)
        mat.use_nodes=True
        shader=mat.node_tree.nodes.get("Principled BSDF")
        shader.inputs["Metallic"].default_value=metallic
        shader.inputs["Roughness"].default_value=roughness
        base=np.clip(np.asarray(color)[None,None,:]*(1+fine[:,:,None]*2),0,1)
        if name=="oak":
            grain=np.sin(xx*math.tau*40+np.sin(yy*math.tau)*.8)
            base*=1+grain[:,:,None]*.15
        dhy,dhx=np.gradient(fine)
        normals=np.dstack((-dhx*2,-dhy*2,np.ones_like(dhx)))
        normals/=np.linalg.norm(normals,axis=2)[:,:,None]
        maps={"basecolor":base,"normal":normals*.5+.5,"roughness":np.repeat(np.clip(roughness+fine,0,1)[:,:,None],3,2),"metallic":np.full((size,size,3),metallic)}
        if name=="amber":
            maps["emission"]=base.copy()
        manifest={"metallic":metallic,"roughness":roughness,"textures":{}}
        for kind,pixels in maps.items():
            rgba=np.ones((size,size,4),dtype=np.float32)
            rgba[:,:,:3]=pixels
            image=bpy.data.images.new(name+"_"+kind,width=size,height=size,alpha=False)
            image.colorspace_settings.name="sRGB" if kind in {"basecolor","emission"} else "Non-Color"
            image.pixels.foreach_set(rgba.ravel())
            image.filepath_raw=str(output/(name+"_"+kind+".png"))
            image.file_format="PNG"
            image.save()
            node=mat.node_tree.nodes.new("ShaderNodeTexImage")
            node.image=image
            node.label=kind
            node.location=(-600,-180*len(manifest["textures"]))
            if kind=="normal":
                normal=mat.node_tree.nodes.new("ShaderNodeNormalMap")
                normal.inputs["Strength"].default_value=.35
                mat.node_tree.links.new(node.outputs["Color"],normal.inputs["Color"])
                mat.node_tree.links.new(normal.outputs["Normal"],shader.inputs["Normal"])
            else:
                target={"basecolor":"Base Color","roughness":"Roughness","metallic":"Metallic","emission":"Emission Color"}[kind]
                mat.node_tree.links.new(node.outputs["Color"],shader.inputs[target])
            manifest["textures"][kind]="textures/"+name+"_"+kind+".png"
        if name=="amber":
            shader.inputs["Emission Strength"].default_value=4
            manifest["emission_strength"]=4
        MATS[name]=mat
        manifests[name]=manifest
    return manifests


def module(asset_id,builder):
    global CURRENT,ROOT
    CURRENT=bpy.data.collections.new(asset_id)
    bpy.context.scene.collection.children.link(CURRENT)
    ROOT=bpy.data.objects.new(asset_id,None)
    ROOT["asset_id"]=asset_id
    ROOT["units"]="metres"
    ROOT["origin"]="ground center; stairs front edge"
    CURRENT.objects.link(ROOT)
    builder()
    return CURRENT


def export_model(collection,path,animation=False):
    # Artist source keeps individual stones. Game export joins by semantic group
    # and parent, preserving the animated hinge and split deck/rails/abutments.
    temporary=bpy.data.collections.new("Temporary game export")
    bpy.context.scene.collection.children.link(temporary)
    duplicates={}
    for source in collection.objects:
        if source.type=="EMPTY":
            duplicate=source.copy()
            duplicate.name="door_hinge" if source.name.startswith("door_hinge") else "Root"
            temporary.objects.link(duplicate)
            duplicates[source]=duplicate
    for source,duplicate in duplicates.items():
        duplicate.parent=duplicates.get(source.parent)
    depsgraph=bpy.context.evaluated_depsgraph_get()
    groups={}
    for source in collection.objects:
        if source.type!="MESH":
            continue
        data=bpy.data.meshes.new_from_object(source.evaluated_get(depsgraph),preserve_all_data_layers=True,depsgraph=depsgraph)
        # Applied bevels can contain collapsed corner slivers. Explicit game
        # triangulation removes numerical zero-area faces before FBX conversion.
        import bmesh
        bm=bmesh.new()
        bm.from_mesh(data)
        bmesh.ops.triangulate(bm,faces=list(bm.faces))
        collapsed=[face for face in bm.faces if face.calc_area()<1e-9 or any(edge.calc_length()<1e-6 for edge in face.edges)]
        if collapsed:
            bmesh.ops.delete(bm,geom=collapsed,context="FACES_ONLY")
        bm.to_mesh(data)
        bm.free()
        duplicate=bpy.data.objects.new(source.name+"_game",data)
        temporary.objects.link(duplicate)
        duplicate.parent=duplicates.get(source.parent)
        duplicate.matrix_basis=source.matrix_basis.copy()
        group=source.get("export_group","structure")
        groups.setdefault((group,source.parent),[]).append(duplicate)
    for (group,parent),objects in groups.items():
        bpy.ops.object.select_all(action="DESELECT")
        for obj in objects:
            obj.select_set(True)
        bpy.context.view_layer.objects.active=objects[0]
        if len(objects)>1:
            bpy.ops.object.join()
        objects[0].name=collection.name+"_"+group
    bpy.context.view_layer.update()
    bpy.ops.object.select_all(action="DESELECT")
    for obj in temporary.objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active=next(o for o in temporary.objects if o.type=="MESH")
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={"EMPTY","MESH"},
        apply_unit_scale=True,axis_forward="-Z",axis_up="Y",use_mesh_modifiers=True,
        mesh_smooth_type="FACE",add_leaf_bones=False,bake_anim=animation,
        bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,
        bake_anim_step=1,bake_anim_simplify_factor=0,path_mode="RELATIVE")
    for obj in list(temporary.objects):
        data=obj.data
        bpy.data.objects.remove(obj,do_unlink=True)
        if data and data.users==0:
            bpy.data.meshes.remove(data)
    bpy.data.collections.remove(temporary)


def animate(hinge,opening):
    hinge.animation_data_clear()
    for frame in range(61):
        t=frame/60
        eased=t*t*(3-2*t)
        hinge.rotation_euler.z=math.radians(-100)*(eased if opening else 1-eased)
        hinge.keyframe_insert(data_path="rotation_euler",frame=frame,group="Door hinge")
    hinge.animation_data.action.name="door_open" if opening else "door_close"
    hinge.animation_data.action.use_fake_user=True


def instance(collection,location,angle=0,name=None):
    obj=bpy.data.objects.new(name or collection.name,None)
    obj.instance_type="COLLECTION"
    obj.instance_collection=collection
    obj.location=location
    obj.rotation_euler.z=angle
    bpy.context.scene.collection.objects.link(obj)
    return obj


def light(name,location,energy,color,size=5):
    data=bpy.data.lights.new(name,"AREA")
    data.energy=energy
    data.color=color
    data.shape="DISK"
    data.size=size
    obj=bpy.data.objects.new(name,data)
    bpy.context.scene.collection.objects.link(obj)
    obj.location=location
    obj.rotation_euler=(Vector((0,0,2))-obj.location).to_track_quat("-Z","Y").to_euler()


def camera(location,target,scale):
    data=bpy.data.cameras.new("Catalogue camera")
    obj=bpy.data.objects.new("Catalogue camera",data)
    bpy.context.scene.collection.objects.link(obj)
    obj.location=location
    obj.rotation_euler=(Vector(target)-obj.location).to_track_quat("-Z","Y").to_euler()
    data.type="ORTHO"
    data.ortho_scale=scale
    bpy.context.scene.camera=obj


def label(text,location,size=.22,align="CENTER",font=None):
    data=bpy.data.curves.new("Dimension label","FONT")
    data.body=text
    data.size=size
    data.align_x=align
    if font:
        data.font=font
    obj=bpy.data.objects.new("Label "+text,data)
    bpy.context.scene.collection.objects.link(obj)
    obj.location=location
    obj.rotation_euler=bpy.context.scene.camera.rotation_euler.copy()
    obj.data.materials.append(MATS["label"])
    return obj


def comparison(collections,output,font):
    global CURRENT,ROOT
    scene=bpy.data.scenes.new("SCALE | Dremma generations")
    bpy.context.window.scene=scene
    CURRENT=bpy.data.collections.new("Dimension guides (not exported models)")
    scene.collection.children.link(CURRENT)
    ROOT=None
    render_setup(scene)
    scene.render.resolution_x=2400
    scene.render.resolution_y=1600
    camera((0,-32,22),(0,-1,2),27)
    ink=bpy.data.materials.new("Presentation dimension ink")
    ink.use_nodes=True
    shader=ink.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value=(.8,.72,.5,1)
    shader.inputs["Emission Color"].default_value=(.8,.72,.5,1)
    shader.inputs["Emission Strength"].default_value=.65
    MATS["label"]=ink
    box("Comparison ground",(0,0,-.1),(200,200,.15),"stone_dark",0,"presentation")
    label("ДРЁММА · ГОРОД РАЗНЫХ ПОКОЛЕНИЙ",(0,2.5,7),.45,font=font)
    label("РОСТ ЖИТЕЛЕЙ",(-6,1.5,5.1),.3,font=font)
    names=["Ребёнок","Небольшой взрослый","Обычный житель","Взрослый дрёмец"]
    for (key,height),x,name in zip(FIGURE_HEIGHTS.items(),(-10,-7.7,-5,-2.1),names):
        instance(collections[key],(x,1.5,0))
        dx=x+.8
        wire("Height guide",[(dx,1.5,0),(dx,1.5,height)],.009,"brass","presentation")
        for z in (0,height):
            wire("Height tick",[(dx-.12,1.5,z),(dx+.12,1.5,z)],.009,"brass","presentation")
        label(str(height).replace(".",",")+" м",(x,1.5,height+.28),.3,font=font)
        label(name,(x,.4,.3),.21,font=font)
    label("СВЕТОВЫЕ ПРОЁМЫ · РАЗМЕРНЫЕ КОНТУРЫ",(6,2.5,6.6),.28,font=font)
    for x,width,height,name in ((1,1.2,2.2,"Малый"),(3.4,1.6,3,"Средний"),(6.2,2.2,4.2,"Высокий"),(10,3.5,6,"Парадный")):
        points=[(x-width/2,1.5,0),(x-width/2,1.5,height),(x+width/2,1.5,height),(x+width/2,1.5,0)]
        wire("Opening guide only",points,.018,"brass","presentation")
        label(name+"\n"+str(width).replace(".",",")+" × "+str(height).replace(".",",")+" м",(x,-.1,.6),.23,font=font)
    instance(collections["stairs_paired"],(-6,-9,0))
    label("ДВЕ ДОРОЖКИ · ОДИН ПОДЪЁМ",(-6,-11,.8),.3,font=font)
    label("Камень: 8 × 0,30 м\nДерево: 16 × 0,15 м\nПодъём 2,40 м · вынос 4,48 м",(-6,-12,.85),.24,font=font)
    for x,key,height in ((1,"railing_iron",1),(5,"railing_high",1.5)):
        instance(collections[key],(x,-5,0))
        label("Поручень "+str(height).replace(".",",")+" м",(x,-6.6,.6),.25,font=font)
    instance(collections["door_standard"],(9,-5,0))
    label("Готовая дверь\nпроём 1,6 × 3,0 м",(9,-6.8,.6),.23,font=font)
    return scene


def render_setup(scene):
    scene.render.engine="CYCLES"
    scene.cycles.samples=48
    scene.cycles.use_denoising=True
    try:
        preferences=bpy.context.preferences.addons["cycles"].preferences
        preferences.compute_device_type="OPTIX"
        preferences.get_devices()
        for device in preferences.devices:
            device.use=device.type=="OPTIX"
        if any(d.use for d in preferences.devices):
            scene.cycles.device="GPU"
    except Exception as error:
        print("GPU setup unavailable, using CPU:",error)
    scene.render.resolution_x=1600
    scene.render.resolution_y=1200
    scene.render.resolution_percentage=100
    scene.render.image_settings.file_format="PNG"
    scene.world=bpy.data.worlds.new("Midnight blue atmosphere")
    scene.world.use_nodes=True
    scene.world.node_tree.nodes["Background"].inputs[0].default_value=(.08,.11,.15,1)
    scene.world.node_tree.nodes["Background"].inputs[1].default_value=.4
    scene.view_settings.view_transform="AgX"
    light("Soft blue key",(1,-8,15),2600,(.65,.78,1),9)
    light("Warm rim",(-7,4,11),3200,(1,.68,.35),7)
    light("Cold rear",(7,8,12),3300,(.46,.66,1),8)


def main():
    global CURRENT,ROOT
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output",type=Path,required=True)
    parser.add_argument("--skip-render",action="store_true")
    args=parser.parse_args(sys.argv[sys.argv.index("--")+1:])
    output=args.output.resolve()
    if output.exists():
        parser.error("Output directory already exists. Choose a NEW output to preserve artist edits.")
    output.mkdir(parents=True)
    for folder in ("models","animations","previews"):
        (output/folder).mkdir()
    (output/"fonts").mkdir()
    font_source=Path(__file__).resolve().parents[2]/"Content"/"fonts"
    for filename in ("NotoSans-Regular.ttf","OFL.txt"):
        shutil.copy2(font_source/filename,output/"fonts"/filename)
    (output/"fonts"/".gitattributes").write_text(
        "# Preserve the upstream license bytes covered by validation-report.json.\n"
        "OFL.txt -text -whitespace\n",encoding="utf-8")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    library=bpy.context.scene
    library.name="SOURCE | Editable modules"
    library.unit_settings.system="METRIC"
    library.unit_settings.scale_length=1
    library.render.fps=30
    library.frame_start=0
    library.frame_end=60
    materials=make_textures(output/"textures")
    builders={"lantern_amber":lantern,"railing_iron":railing,"canal_wall":canal_wall,"bridge_arch":bridge,
              "stairs_medium":stairs,"door_standard":door,"railing_high":lambda:railing(height=1.5),"stairs_paired":paired_stairs}
    builders.update({key:lambda h=height:ratfolk(h,child=h==1.2,tall=h==4) for key,height in FIGURE_HEIGHTS.items()})
    collections={key:module(key,builder) for key,builder in builders.items()}
    report={"blender":bpy.app.version_string,"units":"metres","assets":{}}
    manifest=inventory()
    for key,collection in collections.items():
        bpy.context.view_layer.update()
        points=[o.matrix_world@Vector(p) for o in collection.objects if o.type=="MESH" for p in o.bound_box]
        low=[min(p[i] for p in points) for i in range(3)]
        high=[max(p[i] for p in points) for i in range(3)]
        entry={"bounds_min_m":low,"bounds_max_m":high,"dimensions_m":[high[i]-low[i] for i in range(3)],
               "editable_mesh_objects":sum(o.type=="MESH" for o in collection.objects),
               "base_triangles":sum(len(p.vertices)-2 for o in collection.objects if o.type=="MESH" for p in o.data.polygons)}
        report["assets"][key]=entry
        export_model(collection,output/"models"/(key+".fbx"))
        asset=next(a for a in manifest["assets"] if a["id"]==key)
        asset.update(source_status="generated_pending_independent_validation",source="canal_city_foundation.blend#"+key,
                     export="models/"+key+".fbx",materials=sorted({s.name.removeprefix("CC_") for o in collection.objects if o.type=="MESH" for s in o.data.materials}),
                     measured_geometry=entry)
        if key=="canal_wall":
            asset["walking_surface_height_m"]=3.75
            asset["dimension_interpretation"]="Provisional: main overall H4.5 includes .75m lamps. Structural platform H3.75. Front rings/anchors project .23m beyond reference structural depth4."
        elif key=="bridge_arch":
            asset["design_dimensions_m"][1]=3
            asset["walking_surface_height_m"]=3.1
        elif key=="stairs_medium":
            asset["stair_profile"]={"steps":8,"rise_m":.3,"tread_m":.56,"total_rise_m":2.4,"total_run_m":4.48}
        elif key=="stairs_paired":
            asset["stair_profile"]={"stone_steps":8,"stone_rise_m":.3,"stone_tread_m":.56,"wood_steps":16,"wood_rise_m":.15,"wood_tread_m":.28,"total_rise_m":2.4,"total_run_m":4.48,"overlay_total_width_m":.8,"overlay_useful_width_m":.6}
        elif key in FIGURE_HEIGHTS:
            asset["standing_body_height_m"]=FIGURE_HEIGHTS[key]
            body_points=[o.matrix_world@Vector(p) for o in collection.objects if o.type=="MESH" and o.get("export_group")=="body" for p in o.bound_box]
            body_dims=[max(p[i] for p in body_points)-min(p[i] for p in body_points) for i in range(3)]
            asset["observed_body_dimensions_m"]=body_dims
            asset["design_dimensions_m"]=body_dims
            asset["dimension_basis"]="Dremma caption fixes standing height; width/depth are observed authored silhouette, excluding staff/tail and not inherited placeholder bounds"
        elif key in {"railing_iron","railing_high"}:
            asset["handrail_height_m"]=1.5 if key=="railing_high" else 1
    hinge=bpy.data.objects["door_hinge"]
    for clip,opening in (("door_open",True),("door_close",False)):
        animate(hinge,opening)
        export_model(collections["door_standard"],output/"animations"/(clip+".fbx"),True)
    animate(hinge,True)
    library.frame_set(0)
    next(a for a in manifest["assets"] if a["id"]=="door_standard")["animations"]=["animations/door_open.fbx","animations/door_close.fbx"]
    (output/"materials.json").write_text(json.dumps(materials,indent=2)+"\n",encoding="utf-8")
    (output/"catalog.json").write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf-8")
    (output/"generation-report.json").write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    # Collection instance staging keeps source edits live in all compositions.
    stage=bpy.data.scenes.new("PREVIEW | Six foundation modules")
    bpy.context.window.scene=stage
    positions={"bridge_arch":(-1,3,0),"canal_wall":(-5,-4,0),"stairs_medium":(2,-4,0),"door_standard":(6,2,0),"railing_iron":(4,-6,0),"lantern_amber":(6,-5,0)}
    for key,position in positions.items():
        instance(collections[key],position)
    CURRENT=bpy.data.collections.new("Presentation only")
    stage.collection.children.link(CURRENT)
    ROOT=None
    box("Backdrop",(0,0,-.18),(200,200,.25),"stone_dark",.01,"presentation")
    render_setup(stage)
    camera((17,-24,21),(0,-.2,1.4),23)
    if not args.skip_render:
        stage.render.filepath=str(output/"previews"/"foundation_modules.png")
        bpy.ops.render.render(write_still=True)
    # Diorama has a lower canal and bridge approach at 3.1m. Quay units remain
    # unscaled, sunk .65m; declared source dimensions are unaffected by staging.
    diorama=bpy.data.scenes.new("DIORAMA | Lantern crossing")
    bpy.context.window.scene=diorama
    instance(collections["bridge_arch"],(0,0,0))
    for x,angle in ((-6,math.pi/2),(6,-math.pi/2)):
        instance(collections["canal_wall"],(x,0,-.65),angle)
    instance(collections["door_standard"],(-6,2,3.1))
    instance(collections["stairs_paired"],(6,-8.48,.7))
    instance(collections["scale_rat_adult"],(-5,1,3.1))
    instance(collections["scale_rat_high"],(6,1,3.1))
    for x in (-6,6):
        for y in (-3.5,3.5):
            if (x,y)!=(6,-3.5):
                instance(collections["railing_iron"],(x,y,3.1))
            instance(collections["lantern_amber"],(x-1,y,3.1))
    CURRENT=bpy.data.collections.new("Diorama ground and water (presentation)")
    diorama.collection.children.link(CURRENT)
    ROOT=None
    box("Canal foundation",(0,-2.5, -1.7),(17,16,.4),"stone_dark",.12,"presentation")
    for x in (-6,6):
        masonry(4,.85,8,(x,0,-1.5),course=.25,block=.5,group="presentation")
    masonry(3,2.2,1,(6,-8.98,-1.5),course=.25,block=.5,group="presentation")
    water=bpy.data.materials.new("Presentation water")
    water.use_nodes=True
    shader=water.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value=(.012,.085,.105,1)
    shader.inputs["Metallic"].default_value=.6
    shader.inputs["Roughness"].default_value=.23
    noise=water.node_tree.nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value=18
    bump=water.node_tree.nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value=.24
    bump.inputs["Distance"].default_value=.12
    water.node_tree.links.new(noise.outputs["Fac"],bump.inputs["Height"])
    water.node_tree.links.new(bump.outputs["Normal"],shader.inputs["Normal"])
    MATS["water"]=water
    box("Still canal water",(0,0,-.03),(8,10,.07),"water",0,"presentation")
    render_setup(diorama)
    camera((16,-22,18),(0,-1,1.8),26)
    if not args.skip_render:
        diorama.render.filepath=str(output/"previews"/"lantern_crossing.png")
        bpy.ops.render.render(write_still=True)
    closeup=bpy.data.scenes.new("DETAIL | Door and amber lantern")
    bpy.context.window.scene=closeup
    instance(collections["door_standard"],(-.5,0,0))
    instance(collections["lantern_amber"],(1.6,-.2,1))
    instance(collections["railing_iron"],(1.6,1.5,0))
    CURRENT=bpy.data.collections.new("Closeup presentation")
    closeup.collection.children.link(CURRENT)
    ROOT=None
    masonry(.7,1,.7,(1.6,-.2,0),course=.25,block=.7,group="presentation")
    box("Detail ground",(0,0,-.15),(200,200,.25),"stone_dark",.01,"presentation")
    render_setup(closeup)
    camera((6,-12,7),(.3,0,1.6),6.8)
    if not args.skip_render:
        closeup.render.filepath=str(output/"previews"/"door_lantern_detail.png")
        bpy.ops.render.render(write_still=True)
    font=bpy.data.fonts.load(str(output/"fonts"/"NotoSans-Regular.ttf"))
    dimension_scene=comparison(collections,output,font)
    if not args.skip_render:
        dimension_scene.render.filepath=str(output/"previews"/"dremma_scale_comparison.png")
        bpy.ops.render.render(write_still=True)
    font.filepath="//fonts/NotoSans-Regular.ttf"
    bpy.context.window.scene=diorama
    # Relative image paths survive relocation. Save with the inviting diorama active.
    for image in bpy.data.images:
        if image.filepath:
            image.filepath="//textures/"+Path(image.filepath).name
    bpy.ops.wm.save_as_mainfile(filepath=str(output/"canal_city_foundation.blend"),compress=True)
    print("CANAL_CITY_COMPLETE",output)


if __name__=="__main__":
    main()
