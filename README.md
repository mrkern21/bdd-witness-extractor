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
mkdir -p install
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=../install
cmake --build .
cmake --install .
cd ../../..
```

After installation the directory `external/igraph/install/` will contain:

```
install/
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
