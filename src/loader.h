#pragma once

#include <string>

#include "Graph.h"

// Reads the three preprocessed files in `dir` into a Dataset:
//   actors.tsv  idx  nconst  name   birthYear
//   movies.tsv  idx  tconst  title  year
//   edges.tsv   actorIdx  movieIdx
// Every file is tab-separated with one header row. Actor and movie indices
// are separate contiguous ranges starting at 0, and row `idx` is stored at
// position `idx` in Dataset::actors / Dataset::movies. A birthYear or year of
// "\N" (IMDb's missing-value marker) becomes std::nullopt.
// Each edges.tsv row becomes one Credit.
//
// Throws std::runtime_error if a file is missing or a row cannot be parsed.
// Pass the result to the Graph constructor to build the adjacency lists.
Dataset loadDataset(const std::string& dir);
