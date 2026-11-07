"""Turn the raw IMDb dumps into the three TSV files the C++ program loads.

Inputs (download from https://datasets.imdbws.com/ into --raw-dir):
    name.basics.tsv.gz       nconst, primaryName, birthYear, ... (people)
    title.basics.tsv.gz      tconst, titleType, primaryTitle, startYear, ... (titles)
    title.principals.tsv.gz  tconst, nconst, category, ... (who appeared in what)
    title.ratings.tsv.gz     tconst, averageRating, numVotes (used to keep
                             only well-known movies in the smaller sizes)

IMDb writes missing values as "\\N".

Outputs (written to <out-dir>/<size>/, tab-separated, one header row each):
    actors.tsv  idx  nconst  name   birthYear
    movies.tsv  idx  tconst  title  year
    edges.tsv   actorIdx  movieIdx

Actor indices and movie indices are two separate contiguous ranges starting
at 0, matching ActorIndex and MovieIndex in src/Graph.h. Each edges.tsv row
says that actor actorIdx appeared in movie movieIdx.

Usage:
    python preprocess/preprocess.py --size small
    python preprocess/preprocess.py --size full --raw-dir data/raw --out-dir data
"""

import argparse
from pathlib import Path

# TODO(Raami): decide what each size keeps (for example a minimum numVotes
# per movie) and document it in the README.
SIZES = ("small", "medium", "full")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--size", choices=SIZES, default="small",
                        help="how much of IMDb to keep (default: small)")
    parser.add_argument("--raw-dir", type=Path, default=Path("data/raw"),
                        help="folder holding the IMDb .tsv.gz files (default: data/raw)")
    parser.add_argument("--out-dir", type=Path, default=Path("data"),
                        help="output goes to <out-dir>/<size>/ (default: data)")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    out_dir = args.out_dir / args.size

    # TODO(Raami): with pandas:
    #   1. read the four .tsv.gz files from args.raw_dir (sep="\t", na_values="\\N")
    #   2. keep movies (titleType == "movie"), filtered by args.size
    #   3. keep actor/actress credits on those movies from title.principals
    #   4. assign contiguous actor and movie indices
    #   5. write actors.tsv, movies.tsv and edges.tsv to out_dir
    raise NotImplementedError(f"preprocessing not implemented yet (would write to {out_dir})")


if __name__ == "__main__":
    main()
