"""Author deterministic native assets from real Stride importer metadata.

This creates owned preview assets; it does not exercise MCP reimport or A1.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import uuid


def uid(name):
    return str(uuid.uuid5(uuid.NAMESPACE_URL,"rat-expedition/canal-city/"+name))


def ref(name):
    return uid(name)+":CanalCity/"+name


def header(kind,name,version="2.0.0.0"):
    return f"!{kind}\nId: {uid(name)}\nSerializedVersion: {{Stride: {version}}}\nTags: []\n"


def vector(p):
    return "{X: %s, Y: %s, Z: %s}"%tuple(p)


def component(key,kind,fields):
    return f"                    {uid(key).replace('-','')}: !{kind}\n                        Id: {uid(key+'/id')}\n"+"".join("                        "+line+"\n" for line in fields.splitlines())


def entity(name,position,model=None,extras="",scale=(1,1,1),rotation=(0,0,0,1)):
    key="entity/"+name
    result=f"        -   Entity:\n                Id: {uid(key)}\n                Name: {name}\n                Components:\n"
    result+=component(key+"/transform","TransformComponent",f"Position: {vector(position)}\nRotation: {{X: {rotation[0]}, Y: {rotation[1]}, Z: {rotation[2]}, W: {rotation[3]}}}\nScale: {vector(scale)}\nChildren: {{}}")
    if model:
        result+=component(key+"/model","ModelComponent",f"Model: {ref(model)}\nMaterials: {{}}")
    return result+extras


def hierarchy(names,parts):
    return "Hierarchy:\n    RootParts:\n"+"".join(f"        - ref!! {uid('entity/'+name)}\n" for name in names)+"    Parts:\n"+"".join(parts)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--import-report",type=Path,required=True)
    parser.add_argument("--authoring",type=Path,required=True)
    parser.add_argument("--replace-owned",action="store_true")
    args=parser.parse_args()
    resources=args.authoring/"Resources/CanalCity"
    output=args.authoring/"Assets/CanalCity"
    if output.exists() and not args.replace_owned:
        parser.error("Native output exists; review source changes before --replace-owned")
    output.mkdir(parents=True,exist_ok=True)
    imported=json.loads(args.import_report.read_text())
    metadata={row["file"]:row for row in imported["files"]}
    for path,row in metadata.items():
        if hashlib.sha256((resources/path).read_bytes()).hexdigest()!=row["sha256"]:
            parser.error("Source changed since Stride import: "+path)
    materials=json.loads((resources/"materials.json").read_text())
    catalog=json.loads((resources/"catalog.json").read_text())
    ids=sorted(a["id"] for a in catalog["assets"] if a["source"])
    def save(name,extension,text):
        (output/(name+extension)).write_text(text,encoding="utf-8")
    for name,data in materials.items():
        for role,path in data["textures"].items():
            texture=Path(path).stem
            kind="NormalMapTextureType" if role=="normal" else "GrayscaleTextureType" if role in {"roughness","metallic"} else "ColorTextureType"
            save(texture,".sdtex",header("Texture",texture)+f"Source: !file ../../Resources/CanalCity/{path}\nType: !{kind} {{}}\nIsStreamable: false\nIsCompressed: false\nGenerateMipmaps: true\n")
        mat=header("MaterialAsset",name)+"Attributes:\n"
        for feature,field,role,compute in (("Surface: !MaterialNormalMapFeature","NormalMap","normal","ComputeTextureColor"),("MicroSurface: !MaterialGlossinessMapFeature","GlossinessMap","roughness","ComputeTextureScalar"),("Diffuse: !MaterialDiffuseMapFeature","DiffuseMap","basecolor","ComputeTextureColor"),("Specular: !MaterialMetalnessMapFeature","MetalnessMap","metallic","ComputeTextureScalar")):
            mat+=f"    {feature}\n        {field}: !{compute}\n            Texture: {ref(Path(data['textures'][role]).stem)}\n"
            if role=="roughness": mat+="        Invert: true\n"
        mat+="    DiffuseModel: !MaterialDiffuseLambertModelFeature {}\n    SpecularModel: !MaterialSpecularMicrofacetModelFeature\n        Fresnel: !MaterialSpecularMicrofacetFresnelSchlick {}\n        Visibility: !MaterialSpecularMicrofacetVisibilitySmithSchlickGGX {}\n        NormalDistribution: !MaterialSpecularMicrofacetNormalDistributionGGX {}\n"
        if "emission" in data["textures"]:
            mat+=f"    Emissive: !MaterialEmissiveMapFeature\n        EmissiveMap: !ComputeTextureColor\n            Texture: {ref(Path(data['textures']['emission']).stem)}\n        Intensity: !ComputeFloat\n            Value: 4\n"
        save(name,".sdmat",mat+"Layers: {}\n")
    for key in ids:
        info=metadata["models/"+key+".fbx"]
        skeleton=key+"_skeleton"
        text=header("Skeleton",skeleton)+f"Source: !file ../../Resources/CanalCity/models/{key}.fbx\nScaleImport: 1\nPivotPosition: {{X: 0, Y: 0, Z: 0}}\nNodes:\n"
        for index,node in enumerate(info["nodes"]):
            text+=f"    {uid(skeleton+'/'+str(index)).replace('-','')}: {{Name: {node['Name']}, Depth: {node['Depth']}, Preserve: true}}\n"
        save(skeleton,".sdskel",text)
        text=header("Model",key)+f"Source: !file ../../Resources/CanalCity/models/{key}.fbx\nScaleImport: 1\nPivotPosition: {{X: 0, Y: 0, Z: 0}}\nMergeMeshes: false\nDeduplicateMaterials: true\nSkeleton: {ref(skeleton)}\nMaterials:\n"
        for index,mat in enumerate(info["materials"]):
            name=mat.removeprefix("CC_")
            assert name in materials
            text+=f"    {uid(key+'/mat/'+str(index)).replace('-','')}:\n        Name: {mat}\n        MaterialInstance:\n            Material: {ref(name)}\n"
        save(key,".sdm3d",text)
        extra=""
        if key=="door_standard":
            extra=component("entity/"+key+"/animation","AnimationComponent",f"Animations:\n    {uid('open-map').replace('-','')}~open: {ref('door_open')}\n    {uid('close-map').replace('-','')}~close: {ref('door_close')}")
        save(key+"_prefab",".sdprefab",header("PrefabAsset",key+"_prefab","3.1.0.1")+hierarchy([key],[entity(key,(0,0,0),key,extra)]))
    door_nodes=[n["Name"] for n in metadata["models/door_standard.fbx"]["nodes"]]
    for clip in ("door_open","door_close"):
        assert [n["Name"] for n in metadata["animations/"+clip+".fbx"]["nodes"]]==door_nodes
        assert metadata["animations/"+clip+".fbx"]["animations"][0]["end"]==2
        save(clip,".sdanim",header("Animation",clip)+f"Source: !file ../../Resources/CanalCity/animations/{clip}.fbx\nAnimationStack: 0\nClipDuration: {{Enabled: false}}\nRepeatMode: PlayOnceHold\nType: !StandardAnimationAssetType {{}}\nSkeleton: {ref('door_standard_skeleton')}\nRootMotion: false\nPreviewModel: {ref('door_standard')}\n")
    positions={"canal_wall":(-7,0,-6),"bridge_arch":(3,0,-6),"stairs_medium":(-8,0,5),"stairs_paired":(-4,0,5),"door_standard":(1,0,3.5),"railing_iron":(5,0,3.5),"railing_high":(8,0,3.5),"lantern_amber":(11,0,3.5),"scale_rat_small":(3,0,8),"scale_rat_medium":(5,0,8),"scale_rat_adult":(7,0,8),"scale_rat_high":(10,0,8)}
    names=[];parts=[]
    for key in ids:
        extra=""
        if key=="door_standard":
            extra=component("entity/"+key+"/animation","AnimationComponent",f"Animations:\n    {uid('open-map').replace('-','')}~open: {ref('door_open')}\n    {uid('close-map').replace('-','')}~close: {ref('door_close')}")
        names.append(key);parts.append(entity(key,positions[key],key,extra))
    names.append("Camera")
    yaw=math.atan2(-21,30);pitch=math.atan2(-16,math.hypot(21,30))
    camera_rotation=(math.cos(yaw/2)*math.sin(pitch/2),math.sin(yaw/2)*math.cos(pitch/2),-math.sin(yaw/2)*math.sin(pitch/2),math.cos(yaw/2)*math.cos(pitch/2))
    parts.append(entity("Camera",(-21,18,30),rotation=camera_rotation,extras=component("entity/Camera/camera","CameraComponent","Projection: Perspective\nVerticalFieldOfView: 45\nNearClipPlane: 0.1\nFarClipPlane: 200\nSlot: bbfef2cb-8c63-4cab-9caf-6ae48f44a8ba")))
    for name,kind,intensity,color in (("Ambient","LightAmbient",.8,(.6,.68,.8)),("Key","LightDirectional",3,(1,.85,.66)),("Fill","LightDirectional",1,(.5,.65,1))):
        names.append(name)
        parts.append(entity(name,(0,8,0),extras=component("entity/"+name+"/light","LightComponent",f"Intensity: {intensity}\nType: !{kind}\n    Color: !ColorRgbProvider\n        Value: {{R: {color[0]}, G: {color[1]}, B: {color[2]}}}")))
    save("preview_ground",".sdpromodel",header("ProceduralModelAsset","preview_ground")+f"Type: !CubeProceduralModel\n    Size: {{X: 34, Y: 0.2, Z: 26}}\n    MaterialInstance:\n        Material: {ref('stone_dark')}\n")
    names.append("Ground");parts.append(entity("Ground",(0,-.11,0),"preview_ground"))
    save("PreviewScene",".sdscene",header("SceneAsset","PreviewScene","3.1.0.1")+"ChildrenIds: []\nOffset: {X: 0, Y: 0, Z: 0}\n"+hierarchy(names,parts))
    compositor=(args.authoring/"Assets/GraphicsCompositor.sdgfxcomp").read_text(encoding="utf-8")
    compositor=compositor.replace("d8864284-0fe9-47ee-825e-fb4f0a2589df",uid("PreviewCompositor"),1).replace("Color: {R: 0.40491876, G: 0.411895424, B: 0.43775, A: 1.0}","Color: {R: 0.025, G: 0.035, B: 0.045, A: 1.0}")
    save("PreviewCompositor",".sdgfxcomp",compositor)
    package="!Package\nSerializedVersion: {Assets: 3.1.0.0}\nMeta:\n    Name: Canal City Preview\n    Version: 0.1.0\n    Authors: []\n    Owners: []\n    Dependencies: null\nAssetFolders: []\nResourceFolders: []\nRootAssets:\n"
    package+="".join("    - "+ref(name)+"\n" for name in ["PreviewScene","PreviewCompositor"]+[key+"_prefab" for key in ids])
    preview=args.authoring.parent/"Rat.Expedition.CanalCity.Preview"
    preview.mkdir(exist_ok=True)
    (preview/"Rat.Expedition.CanalCity.Preview.sdpkg").write_text(package,encoding="utf-8")
    (resources/"native-import.json").write_text(json.dumps(imported,indent=2)+"\n",encoding="utf-8")
    for asset in catalog["assets"]:
        if asset["id"] in ids:
            key=asset["id"]
            asset["native_preview_status"]="authored_pending_qualification"
            asset["native_preview"]={"model":"../../Assets/CanalCity/"+key+".sdm3d","skeleton":"../../Assets/CanalCity/"+key+"_skeleton.sdskel","prefab":"../../Assets/CanalCity/"+key+"_prefab.sdprefab"}
    catalog.pop("native_preview_validation_report",None)
    (resources/"catalog.json").write_text(json.dumps(catalog,indent=2)+"\n",encoding="utf-8")
    print("Native assets authored:",output)


if __name__=="__main__":main()
