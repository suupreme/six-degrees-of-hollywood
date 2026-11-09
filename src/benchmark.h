#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "bfs.h"

using ActorPair = std::pair<ActorIndex, ActorIndex>;
using ActorIdPair = std::pair<std::string, std::string>;

// Benchmark harness
struct BenchmarkRow {
    std::string dataset;
    std::string algorithm;
    ActorIndex src = 0;
    ActorIndex dst = 0;
    int degrees = -1;
    std::size_t nodesVisited = 0;
    double ms = 0.0;
};

std::vector<ActorIdPair> pickPairIds(const Dataset& dataset, const Graph& graph,
                                     std::size_t count, std::uint64_t seed = 42);
std::vector<ActorPair> resolvePairs(const Dataset& dataset, const std::vector<ActorIdPair>& ids);

bool writePairsFile(const std::string& path, const std::vector<ActorIdPair>& ids);
std::vector<ActorIdPair> readPairsFile(const std::string& path);

std::vector<BenchmarkRow> runBenchmark(const Graph& graph,
                                       const std::string& datasetName,
                                       const std::vector<ActorPair>& pairs);

bool writeResultsCsv(const std::string& path, const std::vector<BenchmarkRow>& rows);
