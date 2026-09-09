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

PILOTS={"canal_wall","bridge_arch","stairs_medium","railing_iron","lantern_amber","door_standard"}


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


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input",type=Path,required=True)
    parser.add_argument("--report",type=Path,required=True)
    parser.add_argument("--record-catalog",action="store_true",help="After all checks pass, record this source qualification in catalog.json")
    args=parser.parse_args(sys.argv[sys.argv.index("--")+1:])
    root=args.input.resolve()
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
    report={"blender":bpy.app.version_string,"catalogue_entries":len(ids),"source_pilots":len(PILOTS),"source":{},"fbx":{},"animations":{},"failures":[]}
    report["render_configuration"]={s.name:{"engine":s.render.engine,"device":s.cycles.device,"samples":s.cycles.samples,"resolution":[s.render.resolution_x,s.render.resolution_y]} for s in bpy.data.scenes if s.name.startswith(("PREVIEW","DETAIL","DIORAMA"))}
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
    assert len(steps)==9 and all(abs(z-(i+1)*.2)<1e-5 for i,z in enumerate(steps)),steps
    report["stair_surface_heights_m"]=steps
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
    materials=json.loads((root/"materials.json").read_text())
    for material in materials.values():
        assert {"basecolor","normal","roughness","metallic"}<=set(material["textures"])
        for path in material["textures"].values():
            assert (root/path).is_file()
    report["file_sha256"]={str(path.relative_to(root)).replace("\\","/"):hashlib.sha256(path.read_bytes()).hexdigest() for path in sorted(root.rglob("*")) if path.is_file() and path.suffix in {".blend",".fbx",".png"}}
    report["status"]="passed"
    report["qualification_limit"]="Saved Blender sources and Blender FBX reimport only; no Stride editor/runtime/ZIP or visual-art approval implied."
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    if args.record_catalog:
        for asset in manifest["assets"]:
            if asset["id"] in PILOTS:
                asset["source_status"]="validated_blender_fbx"
        manifest["source_validation_report"]=str(args.report.resolve().relative_to(root)).replace("\\","/")
        (root/"catalog.json").write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf-8")
    print("CANAL_CITY_VALIDATION_PASSED",args.report)


if __name__=="__main__":
    main()
