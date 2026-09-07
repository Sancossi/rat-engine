"""Analyze only rat_core and rat_editor_logic compile-database entries."""
import argparse
import concurrent.futures
import json
from pathlib import Path
import re
import subprocess
import time

TARGETS = {"rat_core", "rat_editor_logic"}


def selected_entries(database):
    selected = {}
    for entry in database:
        output = entry.get("output", entry.get("command", " ".join(entry.get("arguments", []))))
        match = re.search(r"(?:^|[/ ]|/Fo)CMakeFiles/([^/ ]+)\.dir/", output.replace("\\", "/"))
        if not match or match[1] not in TARGETS:
            continue
        source = Path(entry["file"])
        if not source.is_absolute():
            source = Path(entry["directory"]) / source
        selected[(match[1], str(source.resolve()))] = {"target": match[1], "file": str(source.resolve())}
    if {entry["target"] for entry in selected.values()} != TARGETS:
        raise ValueError("compile database must contain both rat_core and rat_editor_logic")
    return sorted(selected.values(), key=lambda entry: (entry["target"], entry["file"]))


def analyze(entry, index, build, artifacts, executable):
    command = executable + [entry["file"], "-p", str(build),
                            "--checks=-*,clang-analyzer-*", "--warnings-as-errors=*", "--quiet"]
    start = time.monotonic()
    try:
        result = subprocess.run(command, capture_output=True, timeout=120)
        code, output = result.returncode, result.stdout + result.stderr
    except subprocess.TimeoutExpired as error:
        code, output = -1, (error.stdout or b"") + (error.stderr or b"") + b"\nTIMEOUT: 120 seconds\n"
    except OSError as error:
        code, output = -1, str(error).encode("utf-8")
    log = artifacts / f"{index:03d}-{Path(entry['file']).stem}.log"
    log.write_bytes(output)
    return {**entry, "command": command, "exit_code": code, "log": str(log),
            "elapsed_seconds": time.monotonic() - start}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--artifact-dir", type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=3)
    parser.add_argument("--executable", default="clang-tidy")
    parser.add_argument("--command-json", help="Override with a JSON argv array, e.g. uv tool run ... clang-tidy")
    args = parser.parse_args()
    executable = json.loads(args.command_json) if args.command_json else [args.executable]
    if not isinstance(executable, list) or not executable or not all(isinstance(v, str) and v for v in executable):
        parser.error("command override must be a nonempty array of strings")
    if args.jobs < 1:
        parser.error("jobs must be positive")
    build, artifacts = args.build_dir.resolve(), args.artifact_dir.resolve()
    artifacts.mkdir(parents=True, exist_ok=True)
    report = {"status": "failed", "targets": sorted(TARGETS), "files": []}
    try:
        entries = selected_entries(json.loads((build / "compile_commands.json").read_text(encoding="utf-8")))
        version = subprocess.run(executable + ["--version"], capture_output=True, text=True, timeout=120, check=True)
        report["tool_version"] = version.stdout.strip()
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
            futures = [pool.submit(analyze, entry, i, build, artifacts, executable) for i, entry in enumerate(entries)]
            for future in futures:
                result = future.result()
                report["files"].append(result)
                print(f"{'PASS' if result['exit_code'] == 0 else 'FAIL'} {result['target']} {Path(result['file']).name}", flush=True)
        report["status"] = "passed" if all(f["exit_code"] == 0 for f in report["files"]) else "failed"
    except Exception as error:
        report["error"] = str(error)
    (artifacts / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return 0 if report["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
