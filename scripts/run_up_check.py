#!/usr/bin/env python3
# run_up_check.py — batch-runs up_check over a benchmark set.
#
# Expected layout (one subfolder per SAT instance):
#   <root>/<instance_name>/<instance_name>.cnf
#   <root>/<instance_name>/<instance_name>.destructive.cnf
#   <root>/<instance_name>/<instance_name>.constructive.cnf
#
# For each instance, runs up_check once per mode (destructive, constructive),
# comparing the original .cnf against the matching augmented file. Records
# the witness-clause count and UP-derivability stats for each run in a CSV.

import argparse
import csv
import re
import subprocess
import sys
import time
from pathlib import Path

MODES = ["destructive", "constructive"]

SUMMARY_RE = re.compile(
    r"^SUMMARY,(.*),(\d+),(\d+),(\d+),([0-9.eE+-]+)\s*$", re.MULTILINE)


def find_instance_cnf(instance_dir: Path):
    candidates = [
        f for f in instance_dir.glob("*.cnf")
        if ".destructive." not in f.name and ".constructive." not in f.name
    ]
    if len(candidates) == 1:
        return candidates[0]
    return None


def run_up_check(up_check: Path, orig_path: Path, aug_path: Path):
    cmd = [str(up_check), str(orig_path), str(aug_path)]
    start = time.monotonic()
    result = subprocess.run(cmd, capture_output=True, text=True)
    elapsed = time.monotonic() - start
    return result.returncode, result.stdout, result.stderr, elapsed


def main():
    parser = argparse.ArgumentParser(description="Run up_check over a benchmark set.")
    parser.add_argument("root", type=Path,
                         help="benchmark root folder containing one subfolder per instance")
    parser.add_argument("--up-check", type=Path,
                         default=Path(__file__).resolve().parent.parent / "up_check",
                         help="path to the up_check binary")
    parser.add_argument("--output-csv", type=Path, default=None,
                         help="where to write results (default: <root>/up_check_results.csv)")
    args = parser.parse_args()

    root = args.root.resolve()
    if not root.is_dir():
        sys.exit(f"error: {root} is not a directory")

    if not args.up_check.exists():
        sys.exit(f"error: up_check binary not found at {args.up_check}")

    output_csv = args.output_csv or (root / "up_check_results.csv")

    instance_dirs = sorted(d for d in root.iterdir() if d.is_dir())

    with open(output_csv, "w", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow([
            "instance", "mode", "file",
            "num_witness_clauses", "up_implied", "not_up_implied",
            "useful_fraction", "runtime_seconds", "status",
        ])

        for instance_dir in instance_dirs:
            cnf_path = find_instance_cnf(instance_dir)
            if cnf_path is None:
                print(f"[skip] {instance_dir.name}: no matching .cnf file found")
                continue

            stem = cnf_path.name[:-len(".cnf")]

            for mode in MODES:
                aug_path = instance_dir / f"{stem}.{mode}.cnf"
                if not aug_path.exists():
                    print(f"[skip] {instance_dir.name} [{mode}]: {aug_path.name} not found")
                    writer.writerow([instance_dir.name, mode, aug_path.name,
                                      "", "", "", "", "", "missing"])
                    continue

                print(f"[run] {instance_dir.name} [{mode}] ...", end=" ", flush=True)
                returncode, stdout, stderr, elapsed = run_up_check(
                    args.up_check, cnf_path, aug_path)

                if returncode != 0:
                    print(f"FAILED (exit {returncode}): {stderr.strip()}")
                    writer.writerow([instance_dir.name, mode, aug_path.name,
                                      "", "", "", "", f"{elapsed:.3f}", "error"])
                else:
                    match = SUMMARY_RE.search(stdout)
                    if match:
                        num_witness = int(match.group(2))
                        up_implied = int(match.group(3))
                        not_up_implied = int(match.group(4))
                        useful_fraction = float(match.group(5))
                        print(f"{num_witness} witness clauses, "
                              f"{not_up_implied} useful ({elapsed:.1f}s)")
                        writer.writerow([instance_dir.name, mode, aug_path.name,
                                          num_witness, up_implied, not_up_implied,
                                          f"{useful_fraction:.6f}", f"{elapsed:.3f}", "ok"])
                    else:
                        print("FAILED: no SUMMARY line in output")
                        writer.writerow([instance_dir.name, mode, aug_path.name,
                                          "", "", "", "", f"{elapsed:.3f}", "error"])

                csvfile.flush()

    print(f"\nresults written to {output_csv}")


if __name__ == "__main__":
    main()
