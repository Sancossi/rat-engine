"""Package an existing self-contained native preview with project notices.

Does not build or change the engine checkout. The archive path must be new.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import xml.etree.ElementTree as ET
import zipfile


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--publish",type=Path,required=True)
    parser.add_argument("--zip",type=Path,required=True)
    args=parser.parse_args()
    game=Path(__file__).resolve().parents[2]
    repo=game.parents[1]
    publish=args.publish.resolve()
    if args.zip.exists():parser.error("Choose a new ZIP path")
    if args.zip.resolve().is_relative_to(publish):parser.error("ZIP must be outside the publish directory")
    required=["Rat.Expedition.CanalCity.Preview.exe","coreclr.dll","hostfxr.dll","data/db/bundles/default.bundle"]
    for path in required:
        if not (publish/path).is_file():parser.error("Missing publish output: "+path)
    for path in publish.rglob("*"):
        if path.suffix.lower() in {".fbx",".blend",".csproj",".sdscene",".sdm3d"}:parser.error("Raw source unexpectedly in runtime package: "+str(path))
    def copy(source,target):
        target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(source,target)
    copy(game/"Content/credits.json",publish/"Content/credits.json")
    # Include the existing referenced notices, even though the runtime preview
    # uses neither the 2D sprite nor the Blender label font.
    copy(game/"Content/fonts/OFL.txt",publish/"Content/fonts/OFL.txt")
    copy(repo/"LICENSE",publish/"licenses/Rat-Expedition-LICENSE")
    lockpath=repo/"tools/stride/engine.lock.json"
    lock=json.loads(lockpath.read_text())
    engine=(lockpath.parent/lock["defaultCheckout"]).resolve()
    for source,target in (("LICENSE.md","Stride-LICENSE.md"),("THIRD PARTY.md","Stride-THIRD-PARTY.md")):
        copy(engine/source,publish/"licenses"/target)
    package_lock=game/"Rat.Expedition.CanalCity.Preview/packages.lock.json"
    dependencies=json.loads(package_lock.read_text())["dependencies"]
    packages={}
    for group in dependencies.values():
        for name,data in group.items():
            if "contentHash" in data:packages[(name,data["resolved"])]=data["contentHash"]
    inventory=[]
    for (name,version),content_hash in sorted(packages.items()):
        cache=game/".packages"/name.lower()/version
        if json.loads((cache/".nupkg.metadata").read_text())["contentHash"]!=content_hash:
            parser.error("Package cache differs from lock: "+name)
        spec=ET.parse(next(cache.glob("*.nuspec")))
        license_node=next((node for node in spec.iter() if node.tag.split("}")[-1]=="license"),None)
        notices=[]
        for path in cache.rglob("*"):
            if path.is_file() and re.match(r"^(LICENSE|NOTICE|COPYING|THIRD.?PARTY)([._ -]|$)",path.name,re.I):
                target=Path("licenses")/name/version/path.relative_to(cache)
                copy(path,publish/target)
                notices.append(target.as_posix())
        inventory.append({"id":name,"version":version,"contentHash":content_hash,"license":license_node.text if license_node is not None else "See upstream notices","notices":notices})
    (publish/"licenses/package-inventory.json").write_text(json.dumps(inventory,indent=2)+"\n",encoding="utf-8")
    build={"qualification":"native preview only; not A1/game/editor acceptance","sourceCommit":subprocess.check_output(["git","rev-parse","HEAD"],cwd=repo,text=True).strip(),"sourceWorkingTreeDirty":bool(subprocess.check_output(["git","status","--porcelain"],cwd=repo,text=True).strip()),"engineIntegrationPin":lock["engineCommit"],"runtimePackages":"packages.lock.json contentHash cohort; not a claim that packages were built from editor integration pin","packageLockSha256":hashlib.sha256(package_lock.read_bytes()).hexdigest(),"selfContained":True,"credits":"Content/credits.json","notices":"licenses/package-inventory.json"}
    (publish/"build-manifest.json").write_text(json.dumps(build,indent=2)+"\n",encoding="utf-8")
    args.zip.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(args.zip,"w",zipfile.ZIP_DEFLATED,compresslevel=6) as archive:
        for path in sorted(publish.rglob("*")):
            if path.is_file():archive.write(path,path.relative_to(publish).as_posix())
    print(json.dumps({"zip":str(args.zip.resolve()),"bytes":args.zip.stat().st_size,"sha256":hashlib.sha256(args.zip.read_bytes()).hexdigest(),"packageNotices":len(inventory)},indent=2))


if __name__=="__main__":main()
