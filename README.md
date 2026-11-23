# BDD Witness Extractor

A tool that builds Binary Decision Diagrams (BDDs) over CNF formula communities and extracts *witness clauses* from infeasible BDD nodes. The extracted clauses are appended to the original CNF to steer a SAT solver away from unsatisfiable regions.

The tool uses [CaDiCaL](https://github.com/arminbiere/cadical) to parse and pre-process the input formula, and [igraph](https://github.com/igraph/igraph) for community detection via the Louvain algorithm on a Variable Incidence Graph (VIG).

---

## Table of Contents

1. [Dependencies](#1-dependencies)
2. [System Prerequisites](#2-system-prerequisites)
3. [Building the Dependencies](#3-building-the-dependencies)
4. [Building the Extractor](#4-building-the-extractor)
5. [Usage](#5-usage)
6. [Output](#6-output)
7. [Verification](#7-verification)
8. [UP-Checker](#8-up-checker)

---

## 1. Dependencies

Both dependencies are included as git submodules under `external/`:

| Submodule | Path | Purpose |
|---|---|---|
| CaDiCaL | `external/cadical` | CNF parsing, clause harvesting |
| igraph | `external/igraph` | VIG construction, Louvain community detection |

---

## 2. System Prerequisites

### Compiler and build tools

- `g++` with C++17 support (GCC 7+ or Clang 6+)
- `make`
- `cmake` >= 3.18 (required by igraph)

### igraph build dependencies

igraph's build system requires several additional tools. Install them via your package manager:

**Debian / Ubuntu:**
```sh
sudo apt-get install cmake bison flex libxml2-dev pkg-config
```

**Fedora / RHEL:**
```sh
sudo dnf install cmake bison flex libxml2-devel pkgconfig
```

**macOS (Homebrew):**
```sh
brew install cmake bison flex libxml2 pkg-config
```

> `bison` and `flex` are required to generate igraph's internal parsers. The build will fail with a cryptic error if either is missing.

---

## 3. Building the Dependencies

### 3.1 Clone the repository with submodules

```sh
git clone --recurse-submodules <repo-url>
cd bachelor_thesis_bdd_cadical
```

If you already cloned without `--recurse-submodules`:

```sh
git submodule update --init --recursive
```

### 3.2 Build CaDiCaL

```sh
cd external/cadical
./configure
make
cd ../..
```

This produces `external/cadical/build/libcadical.a`, which the project's Makefile links against.

### 3.3 Build igraph

igraph is built with CMake and installed into a local prefix so the project can find its headers and library without a system-wide install.

```sh
cd external/igraph
mkdir -p igraph_install
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=../igraph_install
cmake --build .
cmake --install .
cd ../../..
```

After installation the directory `external/igraph/igraph_install/` will contain:

```
igraph_install/
  include/igraph/   ← headers referenced by the project
  lib/              ← libigraph.a / libigraph.so
```

---

## 4. Building the Extractor

With both dependencies built, compile the project from the repository root:

```sh
make
```

This produces the binary `bdd_witness_extractor` in the repository root.

To remove all build artifacts:

```sh
make clean
```

---

## 5. Usage

```
./bdd_witness_extractor [-i <input>] [-n <max_nodes>] [-m constructive|destructive]
```

| Flag | Description | Default |
|---|---|---|
| `-i <path>` | Path to the input CNF file in DIMACS format | `instance.cnf` |
| `-n <max_nodes>` | Maximum number of BDD nodes before a chunk is abandoned | `10000000` |
| `-m <mode>` | Extraction mode: `destructive` or `constructive` | `destructive` |

### Extraction modes

**Destructive** — splits each community into fixed-size chunks and recursively halves the chunk size when the BDD node cap is exceeded.

**Constructive** — splits each community into small baseline chunks, runs a BDD build on each, then greedily merges adjacent chunks by Jaccard variable-set similarity.

### Examples

Run with defaults on a benchmark instance:
```sh
./bdd_witness_extractor -i test/kell_example.cnf
```

Use the constructive mode with a tighter node budget:
```sh
./bdd_witness_extractor -i test/kell_example.cnf -m constructive -n 500000
```

---

## 6. Output

The augmented CNF is written to a new file next to the input, with the mode suffix inserted before the extension:

| Input | Mode | Output |
|---|---|---|
| `foo.cnf` | destructive | `foo.destructive.cnf` |
| `foo.cnf` | constructive | `foo.constructive.cnf` |

The output file is a valid DIMACS CNF with:
- The original header rewritten to reflect the additional clauses (`p cnf` line).
- All original clauses copied verbatim.
- A comment line indicating how many witness clauses were appended (e.g. `c 42 witness clauses appended`).
- A `c witness clauses:` marker line preceding the appended clauses.
- The witness clauses appended at the end in standard DIMACS format.

The augmented file can be passed directly to any DIMACS-compatible SAT solver.

---

## 7. Verification

`verification/verify_witnesses.cpp` checks the correctness of an augmented CNF produced by `bdd_witness_extractor` against its original instance.

> **Note:** The verifier is only a sanity check that the augmented CNF does not change the solver result. It is not part of the thesis evaluation itself: the main goal of this thesis is to show the impact of the witness clauses on the SAT solver (see `scripts/run_cadical.py`, which compares CaDiCaL's runtime and statistics on the original and augmented instances).

Build it from the repository root:

```sh
cd verification
make
cd ..
```

This produces the binary `verify_witnesses` in the repository root.

Usage:

```
./verify_witnesses <original.cnf> <augmented.cnf> <sat|unsat>
```

The verifier first checks that `<augmented.cnf>` starts with exactly the clauses of `<original.cnf>` (everything after them is a witness clause). It then solves both files with CaDiCaL and confirms that both give the expected result (`sat` or `unsat`), i.e. that the witness clauses did not change the solver result.

Note that this does not explicitly check entailment (`original ⊨ w` for each witness clause `w`): in the SAT case, a satisfiable augmented formula only shows that *some* model of the original satisfies the witness clauses, not that every model does.

Exit code is `0` if all checks pass, `1` if at least one check fails, and `2` on a usage or parse error.

---

## 8. UP-Checker

`up-checker/up_check.cpp` measures how many of the witness clauses appended by `bdd_witness_extractor` are "trivial," i.e. already derivable from the original formula by unit propagation (UP) alone, as opposed to clauses that encode genuinely new information.

Build it from the repository root:

```sh
cd up-checker
make
cd ..
```

This produces the binary `up_check` in the repository root.

Usage:

```
./up_check <original.cnf> <augmented.cnf>
```

For each witness clause `w` in `<augmented.cnf>`, `up_check` assumes the negated literals of `w` against the original formula and runs CaDiCaL with all decisions disabled (`decisions` limit `0`), so only unit propagation can close the search:

- If propagation alone derives a conflict, `w` is **UP-implied (trivial)** — it adds no information beyond what propagation already gives.
- Otherwise, `w` is **not UP-implied (useful)** — it encodes a non-trivial fact about the formula.

The tool reports the count of UP-implied vs. not-UP-implied witness clauses and the resulting "useful fraction," followed by a machine-readable `SUMMARY,...` line for scripting.
