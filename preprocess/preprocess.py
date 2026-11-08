"""Make actor/movie TSV files from IMDb dumps."""

import argparse
import csv
from pathlib import Path
from tempfile import TemporaryDirectory
from time import perf_counter
import pandas as pd

minv = {"small":100000, "medium": 10000, "full":None}
emp = ("", r"\N")
outs = ("actors.tsv", "movies.tsv", "edges.tsv")


def cleanText(v):
    return v.replace("\t", " ").replace("\r", " ").replace("\n", " ")


def ratedMovies(raw, s, sz):
    cut = minv[s]
    if cut is None :
        return None
    def pick(r):
        votes =pd.to_numeric(r["numVotes"], errors="coerce")
        r = r[votes >=cut]
        return r
    curr = readData(raw / "title.ratings.tsv.gz", ["tconst", "numVotes"], sz, pick)
    sn =set()
    for i in curr["tconst"]:
        sn.add(i)
    return sn


def cleanYear(v):
    if v in emp:
        return r"\N"
    else:
        return str(int(v))


def main():
    a = getArgs()
    t0 = perf_counter()
    try:
        out, (n,m, col), drp =preprocess(a)
    except (OSError, ValueError, pd.errors.ParserError) as error:
        raise SystemExit(f"Preprocessing failed: {error}") from None
    print(f"Wrote {n} actors, {m} movies, and {col} unique credits to {out}")
    if drp:
        print(f"Dropped {drp} credits with missing actor metadata")
    print(f"Finished in {perf_counter() - t0:.2f} seconds")


def getArgs():
    ps = argparse.ArgumentParser(description=__doc__)
    ps.add_argument("--size", choices=minv, default="small")
    ps.add_argument("--raw-dir", type=Path, default=Path("data/raw"))
    ps.add_argument("--out-dir", type=Path, default=Path("data"))
    ps.add_argument("--chunksize", type=int, default=100000)
    a = ps.parse_args()
    if a.chunksize <1:
        ps.error("--chunksize must be positive")
    return a


def preprocess(a):
    nd = ["name.basics.tsv.gz", "title.basics.tsv.gz", "title.principals.tsv.gz"]
    if minv[a.size] is not None:
        nd.append("title.ratings.tsv.gz")
    ms =[]
    for i in nd:
        if not (a.raw_dir / i).is_file():
            ms.append(str(a.raw_dir / i))
    if ms :
        raise FileNotFoundError("Missing raw files: " + ", ".join(ms))

    kp= ratedMovies(a.raw_dir,a.size, a.chunksize)
    cs = ["tconst", "titleType", "primaryTitle", "startYear"]
    def getMov(r):
        r = r[r["titleType"] == "movie"]
        r =r[~r["tconst"].isin(emp)]
        r = r[~r["primaryTitle"].isin(emp)]
        if kp is not None:
            r = r[r["tconst"].isin(kp)]
        return r
    mov = readData(a.raw_dir / "title.basics.tsv.gz", cs, a.chunksize, getMov)
    mov = mov.drop_duplicates("tconst")
    mov =mov.rename(columns={"primaryTitle":"title", "startYear":"year"})
    mov["year"] = mov["year"].map(cleanYear)
    mov["title"] = mov["title"].map(cleanText)
    sn =set(mov["tconst"])

    cs = ["nconst", "tconst", "category"]
    def getEdg(r):
        r = r[r["category"].isin(("actor", "actress"))]
        r =r[r["tconst"].isin(sn)]
        r = r[~r["nconst"].isin(emp)]
        return r
    edg = readData(a.raw_dir / "title.principals.tsv.gz", cs, a.chunksize, getEdg)
    edg = edg[["nconst", "tconst"]]
    edg =edg.drop_duplicates(ignore_index=True)
    b4 = len(edg)
    sn = set(edg["nconst"])

    cs =["nconst", "primaryName", "birthYear"]
    def getAct(r):
        r = r[r["nconst"].isin(sn)]
        r =r[~r["primaryName"].isin(emp)]
        return r
    act = readData(a.raw_dir / "name.basics.tsv.gz", cs, a.chunksize, getAct)
    act = act.drop_duplicates("nconst")
    act =act.rename(columns={"primaryName":"name"})
    act["birthYear"] = act["birthYear"].map(cleanYear)
    act["name"] = act["name"].map(cleanText)
    edg = edg[edg["nconst"].isin(act["nconst"])]
    edg = edg.copy()
    drp = b4-len(edg)
    act = act[act["nconst"].isin(edg["nconst"])]
    act = act.sort_values("nconst")
    act =act.reset_index(drop=True)
    mov = mov[mov["tconst"].isin(edg["tconst"])]
    mov = mov.sort_values("tconst")
    mov =mov.reset_index(drop=True)

    # indexes start at 0
    amap = {}
    i = 0
    for v in act["nconst"]:
        amap[v] = i
        i+=1
    mmap ={}
    i = 0
    for v in mov["tconst"]:
        mmap[v] = i
        i+=1
    act.insert(0, "idx", act["nconst"].map(amap))
    mov.insert(0, "idx", mov["tconst"].map(mmap))
    edg["actorIdx"] = edg["nconst"].map(amap)
    edg["movieIdx"] = edg["tconst"].map(mmap)
    edg = edg[["actorIdx", "movieIdx"]]
    edg =edg.sort_values(["actorIdx", "movieIdx"])
    act = act[["idx", "nconst", "name", "birthYear"]]
    mov = mov[["idx", "tconst", "title", "year"]]

    a.out_dir.mkdir(parents=True, exist_ok=True)
    out = a.out_dir / a.size
    with TemporaryDirectory(prefix=".preprocess-", dir=a.out_dir) as fd:
        tmp= Path(fd)
        writeFiles(tmp, [act,mov, edg])
        out.mkdir(parents=True, exist_ok=True)
        for i in outs:
            (tmp / i).replace(out / i)
    return out, (len(act),len(mov), len(edg)), drp


def writeFiles(fd, sol):
    for i in range(len(outs)):
        curr = sol[i]
        with (fd / outs[i]).open("w", encoding="utf-8", newline="") as f:
            f.write("\t".join(curr.columns) + "\n")
            for r in curr.itertuples(index=False, name=None):
                vs =[]
                for col in r:
                    vs.append(str(col))
                f.write("\t".join(vs) + "\n")


def readData(p, cs, sz, fn):
    sol =[]
    with pd.read_csv(p, sep="\t", usecols=cs, dtype=str, keep_default_na=False,
                     quoting=csv.QUOTE_NONE, encoding="utf-8", chunksize=sz) as rs:
        for curr in rs:
            curr =fn(curr)
            curr =curr[cs]
            if not curr.empty:
                sol.append(curr)
    if sol:
        return pd.concat(sol, ignore_index=True)
    return pd.DataFrame(columns=cs)

if __name__ == "__main__":
    main()
