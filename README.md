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
python preprocess/preprocess.py --size small
```

`--size` is `small`, `medium` or `full`, and output goes to `data/<size>/`.
Each run writes three tab-separated files, each with a header row:

| File | Columns |
|---|---|
| `actors.tsv` | `idx  nconst  name  birthYear` |
| `movies.tsv` | `idx  tconst  title  year` |
| `edges.tsv` | `actorIdx  movieIdx` |

TODO(Raami): what each size keeps, and how long each one takes to generate.

## Building

```bash
cmake -S . -B build
cmake --build build
```

This builds two executables: `six_degrees` (the program) and `tests`.

## Running

```bash
build/six_degrees data/sample
```

TODO(Josh): document the final command-line options (interactive search,
`--benchmark`, `--export`).

Tests (they use `data/sample/`):

```bash
ctest --test-dir build --output-on-failure
```

Visualization: export a path to `viz/path.json`, then serve the folder and
open <http://localhost:8000>:

```bash
python -m http.server --directory viz 8000
```

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
ui/                    user interface (TODO)
viz/                   path.json example and a vis-network viewer
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
