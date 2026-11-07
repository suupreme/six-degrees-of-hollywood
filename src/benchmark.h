#pragma once

#include <string>
#include <utility>
#include <vector>

#include "bfs.h"

using ActorPair = std::pair<ActorIndex, ActorIndex>;

// One row of the results CSV: one algorithm run on one actor pair.
struct BenchmarkRow {
    std::string dataset;        // e.g. "sample", "small", "medium", "full"
    std::string algorithm;      // "bfs" or "bidirectional"
    ActorIndex src = 0;
    ActorIndex dst = 0;
    int degrees = -1;           // offset.size() if Found, otherwise -1
    std::size_t nodesVisited = 0;  // actorcount + moviecount from the PathResult
    double ms = 0.0;            // wall-clock time of the search call only
};

// Runs bfs() and bidirectionalBfs() on every pair in `pairs`, on the same
// graph, and returns two rows per pair (one per algorithm). Only the search
// call is timed; loading and graph construction are not.
std::vector<BenchmarkRow> runBenchmark(const Graph& graph,
                                       const std::string& datasetName,
                                       const std::vector<ActorPair>& pairs);

// Writes `rows` to `path` as CSV with the header
//   dataset,algorithm,src,dst,degrees,nodesVisited,ms
// Returns false if the file could not be written.
bool writeResultsCsv(const std::string& path, const std::vector<BenchmarkRow>& rows);
