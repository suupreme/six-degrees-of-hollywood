# six-degrees-of-hollywood
COP3530 Project 2 Group 48

## Overview

Given two actors, Six Degrees of Hollywood finds the shortest chain of co-stars
that connects them, using IMDb data. Example: Actor A was in Movie 1 with
Actor B, who was in Movie 2 with Actor C, so A and C are 2 degrees apart.

The data is stored as a bipartite graph: actors and movies are both nodes, and
each edge links an actor to a movie they appeared in. A path alternates
actor → movie → actor, and the degrees of separation equal the number of
movies on the path. Actors and movies get their own contiguous integer indices
(`ActorIndex`, `MovieIndex`), mapped from the IMDb IDs (`nm…`, `tt…`), and each
side has an adjacency list (`std::vector<std::vector<…>>`).

We compare two algorithms on the same graph and the same actor pairs:
- **BFS** from the source actor (`src/bfs.cpp`)
- **Bidirectional BFS** from both actors at once (`src/bidirectional.cpp`)

and record how many nodes each one visits and how long it takes.

## Team

| Name | Responsibilities |
|---|---|
| Raami | Preprocessing, graph, data loader, BFS |
| Daniel | Bidirectional BFS, name lookup, benchmark |
| Josh | Command-line program, UI, visualization |

TODO: full names.

## Prerequisites

- A C++17 compiler (GCC 9+, Clang 10+ or MSVC 2019+)
- CMake 3.16+
- Python 3.9+ with pandas (`pip install -r preprocess/requirements.txt`), only needed to preprocess the IMDb data
- A web browser for `viz/` (optional)

## Downloading the IMDb data

Download these files from <https://datasets.imdbws.com/> into `data/raw/`.
Don't unzip them; the preprocessing script reads the `.gz` files directly.

- `name.basics.tsv.gz`
- `title.basics.tsv.gz`
- `title.principals.tsv.gz`
- `title.ratings.tsv.gz`

The IMDb datasets are for personal and non-commercial use only; see the
license on that page. These files and everything generated from them are
gitignored. Only the hand-written sample in `data/sample/` is committed.

## Preprocessing

```bash
pip install -r preprocess/requirements.txt
python preprocess/preprocess.py --size small
python preprocess/preprocess.py --size medium
python preprocess/preprocess.py --size full
```

Output goes to `data/<size>/`. Each run writes three tab-separated files, each with a header row:

| File | Columns |
|---|---|
| `actors.tsv` | `idx  nconst  name  birthYear` |
| `movies.tsv` | `idx  tconst  title  year` |
| `edges.tsv` | `actorIdx  movieIdx` |

Every size keeps only feature films (`titleType == movie`) and people credited
as `actor` or `actress`. The sizes differ in which movies they keep:

| Size | Movies kept | Actors | Movies |
|---|---|---|---|
| `small` | at least 100,000 IMDb votes | 13,619 | 2,761 |
| `medium` | at least 10,000 IMDb votes | 54,830 | 12,472 |
| `full` | every movie | 1,369,669 | 547,163 |

Each smaller size is a subset of the larger ones. `title.principals` lists only
the top-billed cast of each title (about 10 people), so minor roles are missing
and some real-world paths are longer here than they would be with full casts.
`full` takes much longer to preprocess than the other sizes.

TODO(Raami): how long each size takes to generate.

## Building

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

This builds `six_degrees` (the program), `benchmark_runner` and `tests`.
With Visual Studio the executables are in `build/Release/`.

## Running

Web UI: start the server, then open <http://localhost:8080>:

```bash
build/six_degrees data/small --serve
```

Type two actors (suggestions appear as you type) and click **Find Connection**.
The page shows the chain of movies, each step, and how BFS and bidirectional BFS
compare on that search. If several actors share a name, it asks which one you meant.

Command line:

```bash
build/six_degrees data/small
build/six_degrees data/medium --from "Kevin Bacon" --to "Meryl Streep"
```

| Option | Meaning |
|---|---|
| `--from NAME --to NAME` | run one search instead of asking |
| `--algo bfs\|bidir\|both` | which search to run (default `both`) |
| `--export FILE` | save each search as JSON (see `viz/path.json`) |
| `--benchmark PAIRS OUT.csv` | benchmark one dataset; `PAIRS` is a number of random pairs or a file of nm id pairs |
| `--serve [PORT]` | run the web UI (default port 8080) |
| `--web DIR` | folder with the web UI files (default: the repo's `viz/`) |

Names can also be given as IMDb ids (`nm0000102`).

Tests (they use `data/sample/`):

```bash
ctest --test-dir build --output-on-failure
```

## Benchmark

```bash
build/benchmark_runner results.csv 1000 data/small data/medium data/full
```

This picks 1,000 random pairs of actors from the first dataset (list the smallest
first), runs both algorithms on the same pairs in every dataset, and writes one row
per run to `results.csv`:
`dataset,algorithm,src,dst,degrees,nodesVisited,ms`. `degrees` is -1 when the two
actors are not connected. The pairs are saved to `results.csv.pairs.tsv`, so a run
can be repeated with `build/benchmark_runner results.csv results.csv.pairs.tsv ...`.

Results from one run (1,000 pairs, averages per search):

| Size | BFS time (ms) | Bidirectional time (ms) | BFS nodes visited | Bidirectional nodes visited | Pairs with no path |
|---|---|---|---|---|---|
| `small` | 0.122 | 0.013 | 7,838 | 665 | 44 |
| `medium` | 0.348 | 0.049 | 20,300 | 1,715 | 7 |
| `full` | 4.749 | 1.818 | 105,702 | 6,447 | 0 |

Bidirectional BFS visited 12 to 16 times fewer nodes and ran 2.6 to 9 times faster
at every size, and both algorithms found the same number of degrees for every pair.
Pairs that are not connected in `small` become connected as more movies are added.

## Project structure

```
preprocess/            IMDb .tsv.gz -> actors.tsv, movies.tsv, edges.tsv (Python/pandas)
src/
  Graph.h/.cpp         Dataset types and the bipartite adjacency lists
  bfs.h/.cpp           PathResult and standard BFS
  loader.h/.cpp        reads the three TSV files into a Dataset
  bidirectional.h/.cpp bidirectional BFS (same PathResult as bfs)
  lookup.h/.cpp        actor name -> list of actor indices (names can repeat)
  benchmark.h/.cpp     runs both searches on the same pairs, writes a results CSV
  main.cpp             command-line program
  benchmark_main.cpp   benchmark_runner: same pairs across several datasets
ui/                    console UI, JSON export and the local web server for --serve
viz/                   web UI (index.html, style.css, app.js) and a path.json example
data/sample/           small hand-written dataset with known answers
tests/                 test_main.cpp, plain checks with no framework
```

## File ownership

| Path | Owner |
|---|---|
| `preprocess/` | Raami |
| `src/Graph.h`, `src/Graph.cpp`, `src/bfs.h`, `src/bfs.cpp`, `src/loader.h`, `src/loader.cpp` | Raami |
| `src/bidirectional.*`, `src/lookup.*`, `src/benchmark.*` | Daniel |
| `src/main.cpp`, `ui/`, `viz/` | Josh |
| `data/sample/`, `tests/`, `CMakeLists.txt` | Shared |

Shared interfaces: `Dataset`, `Graph` (`src/Graph.h`) and `PathResult`
(`src/bfs.h`). Tell the team before changing them.
