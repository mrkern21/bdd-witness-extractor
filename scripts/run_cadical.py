#!/usr/bin/env python3
# run_cadical.py — batch-runs cadical (under runlim) over a benchmark set.
#
# Expected layout (one subfolder per SAT instance):
#   <root>/<instance_name>/<instance_name>.cnf
#   <root>/<instance_name>/<instance_name>.destructive.cnf
#   <root>/<instance_name>/<instance_name>.constructive.cnf
#
# For each instance, runs "runlim cadical --stats <file>" once per file
# (original, destructive, constructive). Cadical's stdout (including the
# --stats report) is saved next to the input as "<file>.cadical.log"; the
# runlim resource report, solver result, and the most important cadical
# stats (conflicts, decisions, propagations, restarts, reduced, fixed) are
# recorded in a CSV.

import argparse
import csv
import re
import subprocess
import sys
import time
from pathlib import Path

KINDS = [("original", ""), ("destructive", ".destructive"), ("constructive", ".constructive")]

RESULT_RE = re.compile(r"^s (SATISFIABLE|UNSATISFIABLE|UNKNOWN)", re.MULTILINE)
RUNLIM_STATUS_RE = re.compile(r"^\[runlim\] status:\s*(.+)$", re.MULTILINE)
RUNLIM_TIME_RE = re.compile(r"^\[runlim\] time:\s*([0-9.]+) seconds$", re.MULTILINE)
RUNLIM_REAL_RE = re.compile(r"^\[runlim\] real:\s*([0-9.]+) seconds$", re.MULTILINE)
RUNLIM_SPACE_RE = re.compile(r"^\[runlim\] space:\s*([0-9.]+) MB$", re.MULTILINE)

# Most important cadical --stats counters (top-level "c <label>:" lines).
CADICAL_STATS = {
    "conflicts": re.compile(r"^c conflicts:\s*(\d+)", re.MULTILINE),
    "decisions": re.compile(r"^c decisions:\s*(\d+)", re.MULTILINE),
    "propagations": re.compile(r"^c propagations:\s*(\d+)", re.MULTILINE),
    "restarts": re.compile(r"^c restarts:\s*(\d+)", re.MULTILINE),
    "reduced": re.compile(r"^c reduced:\s*(\d+)", re.MULTILINE),
    "fixed": re.compile(r"^c fixed:\s*(\d+)", re.MULTILINE),
}


def find_instance_cnf(instance_dir: Path):
    candidates = [
        f for f in instance_dir.glob("*.cnf")
        if ".destructive." not in f.name and ".constructive." not in f.name
    ]
    if len(candidates) == 1:
        return candidates[0]
    return None


def run_cadical(runlim: Path, cadical: Path, cnf_path: Path, time_limit, space_limit):
    cmd = [str(runlim)]
    if time_limit is not None:
        cmd += [f"--time-limit={time_limit}"]
    if space_limit is not None:
        cmd += [f"--space-limit={space_limit}"]
    cmd += [str(cadical), "--stats", str(cnf_path)]

    start = time.monotonic()
    result = subprocess.run(cmd, capture_output=True, text=True)
    elapsed = time.monotonic() - start
    return result.returncode, result.stdout, result.stderr, elapsed


def main():
    parser = argparse.ArgumentParser(description="Run cadical under runlim over a benchmark set.")
    parser.add_argument("root", type=Path,
                         help="benchmark root folder containing one subfolder per instance")
    parser.add_argument("--cadical", type=Path,
                         default=Path(__file__).resolve().parent.parent
                         / "external" / "cadical" / "build" / "cadical",
                         help="path to the cadical binary")
    parser.add_argument("--runlim", type=Path, default=Path("runlim"),
                         help="path to the runlim binary (default: look up on PATH)")
    parser.add_argument("--time-limit", type=int, default=None,
                         help="runlim time limit in seconds (default: runlim's own default)")
    parser.add_argument("--space-limit", type=int, default=None,
                         help="runlim space limit in MB (default: runlim's own default)")
    parser.add_argument("--output-csv", type=Path, default=None,
                         help="where to write results (default: <root>/cadical_results.csv)")
    args = parser.parse_args()

    root = args.root.resolve()
    if not root.is_dir():
        sys.exit(f"error: {root} is not a directory")

    if not args.cadical.exists():
        sys.exit(f"error: cadical binary not found at {args.cadical}")

    output_csv = args.output_csv or (root / "cadical_results.csv")

    instance_dirs = sorted(d for d in root.iterdir() if d.is_dir())

    with open(output_csv, "w", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow([
            "instance", "kind", "file", "result", "runlim_status",
            "time_seconds", "real_seconds", "space_mb", "exit_code",
            *CADICAL_STATS.keys(),
        ])

        for instance_dir in instance_dirs:
            cnf_path = find_instance_cnf(instance_dir)
            if cnf_path is None:
                print(f"[skip] {instance_dir.name}: no matching .cnf file found")
                continue

            stem = cnf_path.name[:-len(".cnf")]

            for kind, suffix in KINDS:
                file_path = instance_dir / f"{stem}{suffix}.cnf"
                if not file_path.exists():
                    print(f"[skip] {instance_dir.name} [{kind}]: {file_path.name} not found")
                    writer.writerow([instance_dir.name, kind, file_path.name,
                                      "", "", "", "", "", "missing",
                                      *([""] * len(CADICAL_STATS))])
                    continue

                print(f"[run] {instance_dir.name} [{kind}] ...", end=" ", flush=True)
                returncode, stdout, stderr, elapsed = run_cadical(
                    args.runlim, args.cadical, file_path, args.time_limit, args.space_limit)

                log_path = file_path.with_suffix(file_path.suffix + ".cadical.log")
                log_path.write_text(stdout)

                result_match = RESULT_RE.search(stdout)
                status_match = RUNLIM_STATUS_RE.search(stderr)
                time_match = RUNLIM_TIME_RE.search(stderr)
                real_match = RUNLIM_REAL_RE.search(stderr)
                space_match = RUNLIM_SPACE_RE.search(stderr)

                result = result_match.group(1) if result_match else "UNKNOWN"
                runlim_status = status_match.group(1) if status_match else ""
                time_seconds = time_match.group(1) if time_match else f"{elapsed:.3f}"
                real_seconds = real_match.group(1) if real_match else ""
                space_mb = space_match.group(1) if space_match else ""
                stats = [
                    (m.group(1) if (m := pattern.search(stdout)) else "")
                    for pattern in CADICAL_STATS.values()
                ]

                print(f"{result} ({time_seconds}s, {space_mb} MB)")
                writer.writerow([instance_dir.name, kind, file_path.name, result,
                                  runlim_status, time_seconds, real_seconds, space_mb,
                                  returncode, *stats])
                csvfile.flush()

    print(f"\nresults written to {output_csv}")


if __name__ == "__main__":
    main()
