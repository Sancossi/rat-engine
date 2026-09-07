"""Run the headless benchmark and attach reproducible machine/source metadata."""
import argparse
import datetime
import json
import os
from pathlib import Path
import platform
import subprocess


def cpu_model():
    if os.name == "nt":
        import winreg
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                            r"HARDWARE\DESCRIPTION\System\CentralProcessor\0") as key:
            return winreg.QueryValueEx(key, "ProcessorNameString")[0].strip()
    cpuinfo = Path("/proc/cpuinfo")
    if cpuinfo.is_file():
        for line in cpuinfo.read_text().splitlines():
            if line.startswith("model name"):
                return line.split(":", 1)[1].strip()
    return platform.processor()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--map", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source = Path(__file__).resolve().parent.parent
    result = subprocess.run([str(args.executable.resolve()), str(args.map.resolve())],
                            capture_output=True, text=True, encoding="utf-8", timeout=300, check=True)
    report = json.loads(result.stdout)
    report["machine"] = {"os": platform.platform(), "architecture": platform.machine(),
                         "processor": cpu_model(), "logical_cpus": os.cpu_count()}
    report["source"] = {
        "commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=source, text=True).strip(),
        "measured_source_changed_paths": subprocess.check_output(
            ["git", "diff", "--name-only", "HEAD", "--", "src", "apps/editor",
             "benchmarks", "CMakeLists.txt", "cmake"], cwd=source, text=True,
            stderr=subprocess.PIPE).splitlines(),
        "tracked_changes_present": bool(subprocess.check_output(
            ["git", "diff", "--name-only", "HEAD"], cwd=source, text=True, stderr=subprocess.PIPE).strip()),
    }
    report["captured_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(args.output)


if __name__ == "__main__":
    main()
