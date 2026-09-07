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
import zlib


def png_pixels(path: Path):
    """Decode the renderer's lossless 8-bit RGB/RGBA capture without image dependencies."""
    data = path.read_bytes()
    offset, compressed = 8, bytearray()
    while offset < len(data):
        size = struct.unpack(">I", data[offset:offset + 4])[0]
        kind, chunk = data[offset + 4:offset + 8], data[offset + 8:offset + 8 + size]
        if kind == b"IHDR":
            width, height, bits, color, _, _, interlace = struct.unpack(">IIBBBBB", chunk)
            if bits != 8 or color not in (2, 6) or interlace:
                raise RuntimeError("Unsupported renderer PNG format")
            channels = 4 if color == 6 else 3
        if kind == b"IDAT":
            compressed.extend(chunk)
        offset += size + 12
    raw = zlib.decompress(compressed)
    stride, rows = width * channels, []
    for y in range(height):
        start = y * (stride + 1)
        filter_type, row = raw[start], bytearray(raw[start + 1:start + 1 + stride])
        previous = rows[-1] if rows else bytearray(stride)
        for x in range(stride):
            a, b, c = (row[x-channels] if x >= channels else 0), previous[x], (previous[x-channels] if x >= channels else 0)
            if filter_type == 0:
                predictor = 0
            elif filter_type == 1:
                predictor = a
            elif filter_type == 2:
                predictor = b
            elif filter_type == 3:
                predictor = (a+b)//2
            elif filter_type == 4:
                p = a+b-c
                predictor = min((a,b,c), key=lambda value: abs(p-value))
            else:
                raise RuntimeError("Unknown PNG row filter")
            row[x] = (row[x] + predictor) & 255
        rows.append(row)
    def luminance(point, minimum=False):
        x, y = (round(value) for value in point)
        if not (2 <= x < width-2 and 2 <= y < height-2):
            raise RuntimeError("Surface probe outside capture")
        values = []
        for dy in (-1,0,1):
            for dx in (-1,0,1):
                rgb = rows[y+dy][(x+dx)*channels:(x+dx)*channels+3]
                values.append(0.2126*rgb[0]+0.7152*rgb[1]+0.0722*rgb[2])
        return min(values) if minimum else sorted(values)[4]
    return luminance


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
                if name in ("surface-top", "surface-tilt", "surface-under"):
                    luminance = png_pixels(output / (name + ".png"))
                    values = {key: luminance(point) for key, point in child["surface_probes"].items()}
                    item["surface_luminance"] = values
                    if name == "surface-tilt":
                        if luminance(child["surface_probes"]["cube_edge"], minimum=True) > 45:
                            raise RuntimeError("Rendered cube contour is missing")
                        if not (values["cube_top"] > values["cube_z"] + 25 and values["cube_z"] > values["cube_x"] + 15):
                            raise RuntimeError("Rendered top and side lighting did not separate")
                        slopes = [values["ramp_" + yaw] for yaw in ("north", "east", "south", "west")]
                        if max(slopes) - min(slopes) < 20:
                            raise RuntimeError("Rendered ramp slopes lack directional shading")
                    elif name == "surface-top":
                        if not (2 < values["step_two"] - values["step_one"] < 10):
                            raise RuntimeError("Rendered top-down height cue is absent or excessive")
                    elif not (values["bridge_under"] < 100 and values["ground"] > 120):
                        raise RuntimeError("Rendered bridge underside is not dark against the ground")
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
