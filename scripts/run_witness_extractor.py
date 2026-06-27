#!/usr/bin/env python3
# run_witness_extractor.py — batch-runs bdd_witness_extractor over a set of instances.
#
# Expected layout (one subfolder per SAT instance):
#   <root>/<instance_name>/<instance_name>.cnf
#   <root>/<instance_name>/<instance_name>.cnf.xz   (original compressed download, ignored)
#
# For each instance, runs the extractor once per mode (destructive,
# constructive). The extractor writes its augmented CNF next to the input
# file (e.g. "<instance>.destructive.cnf"); this script additionally records
# the witness-clause counts and timing for each run in a CSV.

import argparse
import csv
import re
import subprocess
import sys
import time
from pathlib import Path

MODES = ["destructive", "constructive"]

WITNESS_RE = re.compile(r"number of total witness clauses:\s*(\d+)")
INSPECTED_RE = re.compile(r"number of inspected clauses:\s*(\d+)")
CLAUSES_RE = re.compile(r"^clauses:\s*(\d+)", re.MULTILINE)


def read_dimacs_header(cnf_path: Path):
    with open(cnf_path, "r", errors="ignore") as f:
        for line in f:
            if line.startswith("p cnf"):
                _, _, num_vars, num_clauses = line.split()
                return int(num_vars), int(num_clauses)
    raise ValueError(f"no 'p cnf' header found in {cnf_path}")


def find_instance_cnf(instance_dir: Path):
    # Prefer the .cnf whose name matches an .xz in the same folder, so that
    # augmented outputs from previous runs (*.destructive.cnf etc.) are not
    # picked up as the input instance.
    for xz in instance_dir.glob("*.xz"):
        candidate = instance_dir / xz.stem
        if candidate.suffix == ".cnf" and candidate.exists():
            return candidate

    # Fallback: a single plain .cnf file with no mode suffix.
    candidates = [
        f for f in instance_dir.glob("*.cnf")
        if ".destructive." not in f.name and ".constructive." not in f.name
    ]
    if len(candidates) == 1:
        return candidates[0]
    return None


def run_extractor(extractor: Path, cnf_path: Path, mode: str, max_nodes: int):
    cmd = [str(extractor), "-i", str(cnf_path), "-m", mode]
    if max_nodes is not None:
        cmd += ["-n", str(max_nodes)]

    start = time.monotonic()
    result = subprocess.run(cmd, capture_output=True, text=True)
    elapsed = time.monotonic() - start
    return result.returncode, result.stdout, result.stderr, elapsed


def main():
    parser = argparse.ArgumentParser(description="Run bdd_witness_extractor over a benchmark set.")
    parser.add_argument("root", type=Path,
                         help="benchmark root folder containing one subfolder per instance")
    parser.add_argument("--extractor", type=Path,
                         default=Path(__file__).resolve().parent.parent / "bdd_witness_extractor",
                         help="path to the bdd_witness_extractor binary")
    parser.add_argument("--output-csv", type=Path, default=None,
                         help="where to write results (default: <root>/results.csv)")
    parser.add_argument("--max-nodes", type=int, default=None,
                         help="override the extractor's -n max_nodes (default: extractor default)")
    args = parser.parse_args()

    root = args.root.resolve()
    if not root.is_dir():
        sys.exit(f"error: {root} is not a directory")

    if not args.extractor.exists():
        sys.exit(f"error: extractor binary not found at {args.extractor}")

    output_csv = args.output_csv or (root / "results.csv")

    instance_dirs = sorted(d for d in root.iterdir() if d.is_dir())

    with open(output_csv, "w", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow([
            "instance", "mode", "num_vars", "num_clauses",
            "num_inspected_clauses", "num_witness_clauses",
            "runtime_seconds", "status",
        ])

        for instance_dir in instance_dirs:
            cnf_path = find_instance_cnf(instance_dir)
            if cnf_path is None:
                print(f"[skip] {instance_dir.name}: no matching .cnf file found")
                continue

            num_vars, num_clauses = read_dimacs_header(cnf_path)

            for mode in MODES:
                print(f"[run] {instance_dir.name} [{mode}] ...", end=" ", flush=True)
                returncode, stdout, stderr, elapsed = run_extractor(
                    args.extractor, cnf_path, mode, args.max_nodes)

                if returncode != 0:
                    print(f"FAILED (exit {returncode}): {stderr.strip()}")
                    writer.writerow([instance_dir.name, mode, num_vars, num_clauses,
                                      "", "", f"{elapsed:.3f}", "error"])
                else:
                    witness_match = WITNESS_RE.search(stdout)
                    inspected_match = INSPECTED_RE.search(stdout)
                    num_witness = int(witness_match.group(1)) if witness_match else ""
                    num_inspected = int(inspected_match.group(1)) if inspected_match else ""
                    print(f"{num_witness} witness clauses ({elapsed:.1f}s)")
                    writer.writerow([instance_dir.name, mode, num_vars, num_clauses,
                                      num_inspected, num_witness, f"{elapsed:.3f}", "ok"])

                csvfile.flush()

    print(f"\nresults written to {output_csv}")


if __name__ == "__main__":
    main()
