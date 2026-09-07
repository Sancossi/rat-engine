"""Run real editor processes; widgets and input remain owned by the C++ runner."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import time


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary-dir", type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument("--user-data-dir", required=True, type=Path)
    parser.add_argument("--artifact-dir", required=True, type=Path)
    parser.add_argument("--renderer", choices=("software-d3d11", "software-opengl"), required=True)
    parser.add_argument("--manifest", type=Path, default=Path(__file__).with_name("gui_scenarios.json"))
    parser.add_argument("--scenario", action="append", help="Explicit subset; report is not full acceptance")
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    names = [entry["name"] for entry in manifest["scenarios"]]
    selected = args.scenario or names
    if set(selected) - set(names):
        parser.error("Unknown scenario in subset")
    # Fresh run roots prevent a crashed child from inheriting a stale passed report.
    args.user_data_dir.mkdir(parents=True, exist_ok=True)
    args.artifact_dir.mkdir(parents=True, exist_ok=True)
    user = Path(tempfile.mkdtemp(prefix="gui-run-", dir=args.user_data_dir.resolve()))
    artifacts = Path(tempfile.mkdtemp(prefix="gui-run-", dir=args.artifact_dir.resolve()))
    cwd = user / "unrelated working directory"
    cwd.mkdir()
    binary = args.binary_dir.resolve()
    suffix = ".exe" if os.name == "nt" else ""
    runner = binary / ("rat-editor-gui-tests" + suffix)
    editor = binary / ("rat-editor" + suffix)
    report = {"complete_suite": args.scenario is None, "status": "running", "scenarios": [],
              "binary_dir": str(binary), "user_root": str(user), "artifact_root": str(artifacts),
              "cwd": str(cwd), "requested_renderer": args.renderer,
              "software_opengl_environment": os.environ.get("LIBGL_ALWAYS_SOFTWARE", "")}
    start = time.monotonic()
    def package_snapshot():
        return {str(path.relative_to(binary)): hashlib.sha256(path.read_bytes()).hexdigest()
                for path in binary.rglob("*") if path.is_file()}
    try:
        initial_package = package_snapshot()
        for name in selected:
            output = artifacts / name
            output.mkdir()
            fixture = user / ("restart-project" if name.startswith("restart-") else name)
            if name == "production-malformed-start":
                fixture.mkdir()
                malformed = fixture / "malformed.json"
                malformed.write_text("{invalid", encoding="utf-8")
                command = [str(editor), "--user-data-dir", str(fixture), "--map", str(malformed)]
            else:
                command = [str(runner), "--user-data-dir", str(fixture), "--artifact-dir", str(output),
                           "--renderer", args.renderer, "--scenario", name]
            remaining = 270 - (time.monotonic() - start)
            if remaining <= 0:
                raise RuntimeError("GUI acceptance exceeded its 270 second process budget")
            result = subprocess.run(command, cwd=cwd, capture_output=True, timeout=min(90, remaining))
            (output / "stdout.log").write_bytes(result.stdout)
            (output / "stderr.log").write_bytes(result.stderr)
            item = {"name": name, "exit_code": result.returncode, "command": command}
            report["scenarios"].append(item)
            if name == "production-malformed-start":
                if result.returncode != 1:
                    raise RuntimeError(f"Production malformed startup returned {result.returncode}, expected 1")
            else:
                child = json.loads((output / "scenario-report.json").read_text(encoding="utf-8"))
                item["result"] = child
                if result.returncode != 0 or child.get("status") != "passed":
                    raise RuntimeError(f"{name} failed: exit {result.returncode}, {child}")
                if name != "malformed-start" and args.renderer == "software-d3d11":
                    if "DXGI_ADAPTER_FLAG_SOFTWARE / WARP" not in child.get("renderer", ""):
                        raise RuntimeError(f"{name} did not verify its software adapter")
                captures = []
                for path in sorted(output.glob("*.png")):
                    header = path.read_bytes()[:24]
                    if header[:8] != b"\x89PNG\r\n\x1a\n":
                        raise RuntimeError(f"Invalid capture {path}")
                    width, height = struct.unpack(">II", header[16:24])
                    captures.append({"path": str(path), "width": width, "height": height})
                item["captures"] = captures
                if name.startswith("scale-"):
                    scale = int(name.split("-")[1]) / 100
                    expected = (int(1280 * scale + 80) * 5 // 4, int(720 * scale + 60) * 5 // 4)
                    if not any("ratio-modal" in c["path"] and (c["width"], c["height"]) == expected for c in captures):
                        raise RuntimeError(f"{name} capture does not match scripted framebuffer dimensions {expected}")
            item["status"] = "passed"
            print(f"PASS {name}", flush=True)
        # All runtime paths resolve beside the installed executable without a data-root override.
        for relative in ("data/maps/grey_yard.json", "data/fonts/NotoSans-Regular.ttf", "data/audio/beep.wav"):
            if not (binary / relative).is_file():
                raise RuntimeError(f"Missing frontend resource: {relative}")
        if package_snapshot() != initial_package:
            raise RuntimeError("Acceptance wrote into the binary/package directory")
        if list(cwd.iterdir()):
            raise RuntimeError("Acceptance wrote into unrelated working directory")
        report["package_unchanged"] = True
        report["unrelated_cwd_unchanged"] = True
        report["status"] = "passed"
    except Exception as error:
        report["status"] = "failed"
        report["error"] = str(error)
        print(str(error), file=sys.stderr)
    report["elapsed_seconds"] = time.monotonic() - start
    result_path = artifacts / "gui-acceptance-report.json"
    result_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Report: {result_path}")
    return 0 if report["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
