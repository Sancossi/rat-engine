"""Independently load delivered Blender/FBX assets, check geometry and animation.

No generator imports: validation observes saved deliverables in a fresh Blender.
Run blender --factory-startup --background --python-exit-code 1 --python
validate.py -- --input <CanalCity> --report <new report.json>.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys

import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

PILOTS={"canal_wall","bridge_arch","stairs_medium","railing_iron","lantern_amber","door_standard",
        "scale_rat_small","scale_rat_medium","scale_rat_adult","scale_rat_high","railing_high","stairs_paired"}


def check_meshes(objects):
    meshes=[obj for obj in objects if obj.type=="MESH"]
    assert meshes,"No meshes"
    for obj in meshes:
        assert obj.data.vertices and obj.data.polygons, "Empty mesh: "+obj.name
        assert obj.data.uv_layers,"Missing UV: "+obj.name
        assert obj.data.materials,"Missing material: "+obj.name
        for vertex in obj.data.vertices:
            assert all(math.isfinite(n) for n in vertex.co),"Non-finite vertex: "+obj.name
        for uv in obj.data.uv_layers.active.data:
            assert all(math.isfinite(n) for n in uv.uv),"Non-finite UV: "+obj.name
        for polygon in obj.data.polygons:
            assert polygon.area>1e-10,"Degenerate face: "+obj.name
    points=[obj.matrix_world@Vector(p) for obj in meshes for p in obj.bound_box]
    return {"mesh_count":len(meshes),"triangles":sum(len(p.vertices)-2 for obj in meshes for p in obj.data.polygons),
            "bounds_min_m":[min(p[i] for p in points) for i in range(3)],
            "bounds_max_m":[max(p[i] for p in points) for i in range(3)]}


def material_paths(root):
    missing=[]
    for image in bpy.data.images:
        if image.source=="FILE":
            path=Path(bpy.path.abspath(image.filepath))
            if not path.is_file():
                missing.append(str(path))
    assert not missing,"Missing textures: "+repr(missing)


def ray_clear(objects,origin,direction,distance):
    for obj in objects:
        if obj.type!="MESH":
            continue
        inverse=obj.matrix_world.inverted()
        hit,_,_,_=obj.ray_cast(inverse@Vector(origin),inverse.to_3x3()@Vector(direction),distance=distance)
        assert not hit,"Clearance ray blocked by "+obj.name


def world_mesh(objects):
    """Observe evaluated geometry, including source bevels and current animation."""
    vertices=[]
    faces=[]
    graph=bpy.context.evaluated_depsgraph_get()
    for obj in objects:
        evaluated=obj.evaluated_get(graph)
        data=evaluated.to_mesh()
        offset=len(vertices)
        vertices.extend(evaluated.matrix_world@v.co for v in data.vertices)
        faces.extend(tuple(offset+i for i in p.vertices) for p in data.polygons)
        evaluated.to_mesh_clear()
    return vertices,faces


def door_sweep(scene,frame_objects,leaf_objects):
    """Reject masonry crossings throughout a complete baked door animation.

    BVH overlap detects intersecting surfaces rather than just a pivot or a
    single test point. A ray parity check also detects leaf vertices contained
    wholly inside the closed frame. All moving hardware is included.
    """
    assert frame_objects and leaf_objects,"Door frame or moving leaf missing"
    scene.frame_set(0)
    vertices,faces=world_mesh(frame_objects)
    frame_tree=BVHTree.FromPolygons(vertices,faces)
    low=Vector(tuple(min(v[i] for v in vertices) for i in range(3)))
    high=Vector(tuple(max(v[i] for v in vertices) for i in range(3)))
    direction=Vector((1,.137,.071)).normalized()
    for frame_number in range(61):
        scene.frame_set(frame_number)
        moving,moving_faces=world_mesh(leaf_objects)
        leaf_tree=BVHTree.FromPolygons(moving,moving_faces)
        assert not frame_tree.overlap(leaf_tree),f"Door leaf intersects masonry at frame {frame_number}"
        for point in moving:
            if not all(low[i]<point[i]<high[i] for i in range(3)):
                continue
            origin=point.copy()
            hits=0
            while True:
                location,_,index,_=frame_tree.ray_cast(origin,direction)
                if index is None:
                    break
                hits+=1
                origin=location+direction*1e-5
                assert hits<256,"Unstable door containment ray"
            assert hits%2==0,f"Door leaf is inside masonry at frame {frame_number}"
    scene.frame_set(0)
    return {"sample_frames":list(range(61)),"surface_intersections":0,"contained_leaf_vertices":0,"geometry":"evaluated world meshes; moving hardware included"}


def resident_passage(frame_objects,body_objects):
    """Sweep every evaluated body vertex through the opening along its normal.

    The supplied body includes head, ears, robe and feet; optional staff/tail
    groups are deliberately excluded from the standing resident fit contract.
    """
    vertices,faces=world_mesh(frame_objects)
    tree=BVHTree.FromPolygons(vertices,faces)
    front=min(v.y for v in vertices)-.1
    distance=max(v.y for v in vertices)-front+.1
    body,_=world_mesh(body_objects)
    assert body,"No ordinary adult geometry for door fit"
    for point in body:
        hit=tree.ray_cast(Vector((point.x,front,point.z)),Vector((0,1,0)),distance)
        assert hit[2] is None,f"Ordinary adult cannot pass the door at x={point.x}, z={point.z}"
    return {"standing_height_m":max(p.z for p in body)-min(p.z for p in body),"swept_body_vertices":len(body),"masonry_hits":0,"alignment":"centered X=0, standing Z=0, movement along Y; body/robe included, tail/staff excluded"}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input",type=Path,required=True)
    parser.add_argument("--report",type=Path,required=True)
    parser.add_argument("--record-catalog",action="store_true",help="After all checks pass, record this source qualification in catalog.json")
    args=parser.parse_args(sys.argv[sys.argv.index("--")+1:])
    root=args.input.resolve()
    catalog_report=None
    if args.record_catalog:
        try:
            catalog_report=args.report.resolve().relative_to(root).as_posix()
        except ValueError:
            parser.error("--record-catalog requires --report inside --input so the catalog reference remains portable")
    assert not args.report.exists(),"Choose a new report path"
    manifest=json.loads((root/"catalog.json").read_text(encoding="utf-8"))
    ids=[asset["id"] for asset in manifest["assets"]]
    assert len(ids)==len(set(ids)),"Duplicate catalogue IDs"
    implemented={a["id"] for a in manifest["assets"] if a["source"] is not None}
    assert implemented==PILOTS,"Incomplete or falsely populated source catalogue"
    for asset in manifest["assets"]:
        assert asset["references"] and len(asset["design_dimensions_m"])==3
        assert all(value>0 for value in asset["design_dimensions_m"])
        if asset["id"] not in PILOTS:
            assert asset["source_status"]=="pending" and asset["export"] is None
    bpy.ops.wm.open_mainfile(filepath=str(root/"canal_city_foundation.blend"))
    bpy.context.window.scene=bpy.data.scenes["SOURCE | Editable modules"]
    scene=bpy.context.scene
    assert scene.unit_settings.scale_length==1 and scene.unit_settings.system=="METRIC"
    assert scene.render.fps==30
    scene.frame_set(0)
    material_paths(root)
    for font in bpy.data.fonts:
        if font.filepath and font.filepath!="<builtin>":
            assert Path(bpy.path.abspath(font.filepath)).is_file(),"Missing font: "+font.filepath
    report={"blender":bpy.app.version_string,"catalogue_entries":len(ids),"source_pilots":len(PILOTS),"source":{},"fbx":{},"animations":{},"failures":[]}
    report["render_configuration"]={s.name:{"engine":s.render.engine,"device":s.cycles.device,"samples":s.cycles.samples,"resolution":[s.render.resolution_x,s.render.resolution_y]} for s in bpy.data.scenes if s.name.startswith(("PREVIEW","DETAIL","DIORAMA","SCALE"))}
    for key in sorted(PILOTS):
        objects=list(bpy.data.collections[key].objects)
        report["source"][key]=check_meshes(objects)
        roots=[obj for obj in objects if obj.parent is None]
        assert len(roots)==1 and roots[0].type=="EMPTY"
        assert roots[0].location.length<1e-7
    ray_clear(bpy.data.collections["bridge_arch"].objects,(0,-5,1.2),(0,1,0),10)
    ray_clear(bpy.data.collections["canal_wall"].objects,(0,-5,.5),(0,1,0),10)
    # Step heights are observed from saved nosing geometry, not generator config.
    steps=sorted(max((obj.matrix_world@v.co).z for v in obj.data.vertices) for obj in bpy.data.collections["stairs_medium"].objects if obj.name.startswith("Eased stair nosing"))
    assert len(steps)==8 and all(abs(z-(i+1)*.3)<1e-5 for i,z in enumerate(steps)),steps
    report["stair_surface_heights_m"]=steps
    paired=bpy.data.collections["stairs_paired"]
    wood=sorted(max((obj.matrix_world@v.co).z for v in obj.data.vertices) for obj in paired.objects if obj.name.startswith("Overlay wooden tread"))
    stone=sorted(max((obj.matrix_world@v.co).z for v in obj.data.vertices) for obj in paired.objects if obj.name.startswith("Eased stair nosing"))
    assert len(wood)==16 and len(stone)==8
    assert all(abs(z-(i+1)*.15)<1e-5 for i,z in enumerate(wood)),wood
    assert all(abs(a-b)<1e-5 for a,b in zip(wood[1::2],stone))
    assert abs(wood[-1]-2.4)<1e-5
    treads=[obj for obj in paired.objects if obj.name.startswith("Overlay wooden tread")]
    tread_vertices=[obj.matrix_world@v.co for obj in treads for v in obj.data.vertices]
    run=max(v.y for v in tread_vertices)-min(v.y for v in tread_vertices)
    width=max(v.x for v in tread_vertices)-min(v.x for v in tread_vertices)
    assert abs(run-4.48)<1e-5 and abs(width-.8)<1e-5
    # A walking lane above each wooden tread is unobstructed; the recessed
    # support must not conceal the small treads inside the coarse stone steps.
    for i,z in enumerate(wood):
        ray_clear(paired.objects,(-.8,(i+.5)*.28,z+.025),(0,0,1),.5)
    report["paired_stairs"]={"wood_surface_heights_m":wood,"stone_surface_heights_m":stone,"wood_total_run_m":run,"wood_total_width_m":width,"matched_top_m":wood[-1]}
    report["body_heights_m"]={}
    for key,height in {"scale_rat_small":1.2,"scale_rat_medium":1.8,"scale_rat_adult":2.8,"scale_rat_high":4}.items():
        objects=[obj for obj in bpy.data.collections[key].objects if obj.type=="MESH" and obj.get("export_group")=="body"]
        observed=check_meshes(objects)
        actual=observed["bounds_max_m"][2]-observed["bounds_min_m"][2]
        assert abs(actual-height)<1e-5 and abs(observed["bounds_min_m"][2])<1e-5,(key,actual)
        report["body_heights_m"][key]=actual
    report["handrail_heights_m"]={}
    for key,height in (("railing_iron",1),("railing_high",1.5)):
        rails=[obj for obj in bpy.data.collections[key].objects if obj.name.startswith("Handrail datum")]
        assert len(rails)==1
        actual=max((rails[0].matrix_world@v.co).z for v in rails[0].data.vertices)
        assert abs(actual-height)<1e-5,(key,actual)
        report["handrail_heights_m"][key]=actual
    frame=[obj for obj in bpy.data.collections["door_standard"].objects if obj.type=="MESH" and obj.get("export_group")=="frame"]
    for z in (.12,1.0,1.75):
        ray_clear(frame,(-.799,0,z),(1,0,0),1.598)
    ray_clear(frame,(0,0,.001),(0,0,1),2.998)
    arch_vertices=[obj.matrix_world@v.co for obj in frame if obj.name.startswith("Pointed archivolt") for v in obj.data.vertices]
    opening_top=min(v.z for v in arch_vertices if abs(v.x)<1e-5)
    spring_height=min(v.z for v in arch_vertices)
    spring_width=2*min(abs(v.x) for v in arch_vertices if abs(v.z-spring_height)<1e-5)
    assert abs(opening_top-3)<1e-5 and abs(spring_width-1.6)<1e-5,(opening_top,spring_width)
    report["door_clear_aperture_m"]={"spring_width":spring_width,"center_height":opening_top,"clearance_samples_height_m":[.12,1,1.75]}
    leaf=[obj for obj in bpy.data.collections["door_standard"].objects if obj.type=="MESH" and obj.get("export_group")=="leaf"]
    report["source_door_sweep"]=door_sweep(scene,frame,leaf)
    body=[obj for obj in bpy.data.collections["scale_rat_adult"].objects if obj.type=="MESH" and obj.get("export_group")=="body"]
    report["ordinary_adult_door_passage"]=resident_passage(frame,body)
    for key in sorted(PILOTS):
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.context.scene.render.fps=30
        bpy.ops.import_scene.fbx(filepath=str(root/"models"/(key+".fbx")))
        bpy.context.view_layer.update()
        observed=check_meshes(list(bpy.context.scene.objects))
        material_paths(root)
        for bound in ("bounds_min_m","bounds_max_m"):
            assert all(abs(a-b)<.006 for a,b in zip(observed[bound],report["source"][key][bound])),(key,bound,observed[bound],report["source"][key][bound])
        assert observed["mesh_count"]<=8,"Game meshes should be grouped: "+key
        report["fbx"][key]=observed
    for clip in ("door_open","door_close"):
        bpy.ops.wm.read_factory_settings(use_empty=True)
        scene=bpy.context.scene
        scene.render.fps=30
        bpy.ops.import_scene.fbx(filepath=str(root/"animations"/(clip+".fbx")),anim_offset=0)
        candidates=[obj for obj in scene.objects if obj.type=="EMPTY" and obj.name.startswith("door_hinge")]
        assert len(candidates)==1,"Door hinge lost in FBX"
        hinge=candidates[0]
        leaf=[obj for obj in hinge.children_recursive if obj.type=="MESH"]
        frame_objects=[obj for obj in scene.objects if obj.type=="MESH" and obj not in leaf]
        samples=[]
        for frame in (0,15,30,45,60):
            scene.frame_set(frame)
            samples.append(math.degrees(hinge.rotation_euler.z))
        expected=(0,-100) if clip=="door_open" else (-100,0)
        assert abs(samples[0]-expected[0])<.02 and abs(samples[-1]-expected[1])<.02,(clip,samples)
        diffs=[b-a for a,b in zip(samples,samples[1:])]
        assert all(d<0 for d in diffs) if clip=="door_open" else all(d>0 for d in diffs)
        scene.frame_set(0 if clip=="door_open" else 60)
        check_meshes(scene.objects)
        report["animations"][clip]={"fps":30,"frames":[0,60],"duration_seconds":2,"sample_frames":[0,15,30,45,60],"local_hinge_angles_deg":samples}
        report["animations"][clip]["masonry_sweep"]=door_sweep(scene,frame_objects,leaf)
    materials=json.loads((root/"materials.json").read_text())
    for material in materials.values():
        assert {"basecolor","normal","roughness","metallic"}<=set(material["textures"])
        for path in material["textures"].values():
            assert (root/path).is_file()
    report["file_sha256"]={str(path.relative_to(root)).replace("\\","/"):hashlib.sha256(path.read_bytes()).hexdigest() for path in sorted(root.rglob("*")) if path.is_file() and path.suffix in {".blend",".fbx",".png",".ttf",".txt"}}
    report["status"]="passed"
    report["qualification_limit"]="Saved Blender sources and Blender FBX reimport only; no Stride editor/runtime/ZIP or visual-art approval implied."
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    if args.record_catalog:
        for asset in manifest["assets"]:
            if asset["id"] in PILOTS:
                asset["source_status"]="validated_blender_fbx"
        manifest["source_validation_report"]=catalog_report
        (root/"catalog.json").write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf-8")
    print("CANAL_CITY_VALIDATION_PASSED",args.report)


if __name__=="__main__":
    main()
