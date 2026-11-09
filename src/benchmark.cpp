#include "benchmark.h"

#include <chrono>
#include <fstream>
#include <random>
#include <sstream>
#include <unordered_map>

#include "bidirectional.h"

namespace {

int degreesOf(const PathResult& result) {
    return result.status == PathStatus::Found ? static_cast<int>(result.offset.size()) : -1;
}

template <typename Search>
BenchmarkRow timeSearch(Search search, const Graph& graph, const std::string& datasetName,
                        const std::string& algorithm, const ActorPair& pair) {
    const auto start = std::chrono::steady_clock::now();
    const PathResult result = search(graph, pair.first, pair.second);
    const auto end = std::chrono::steady_clock::now();

    BenchmarkRow row;
    row.dataset = datasetName;
    row.algorithm = algorithm;
    row.src = pair.first;
    row.dst = pair.second;
    row.degrees = degreesOf(result);
    row.nodesVisited = result.actorcount + result.moviecount;
    row.ms = std::chrono::duration<double, std::milli>(end - start).count();
    return row;
}

}  // namespace

std::vector<ActorIdPair> pickPairIds(const Dataset& dataset, const Graph& graph,
                                     std::size_t count, std::uint64_t seed) {
    std::vector<ActorIndex> candidates;
    for (ActorIndex a = 0; a < graph.actorCount(); ++a) {
        if (!graph.moviesForActor(a).empty())
            candidates.push_back(a);
    }

    std::vector<ActorIdPair> ids;
    if (candidates.size() < 2)
        return ids;

    std::mt19937_64 rng(seed);
    while (ids.size() < count) {
        const ActorIndex a = candidates[rng() % candidates.size()];
        const ActorIndex b = candidates[rng() % candidates.size()];
        if (a != b)
            ids.emplace_back(dataset.actors[a].id, dataset.actors[b].id);
    }
    return ids;
}

std::vector<ActorPair> resolvePairs(const Dataset& dataset, const std::vector<ActorIdPair>& ids) {
    std::unordered_map<std::string, ActorIndex> indexOf;
    indexOf.reserve(dataset.actors.size());
    for (ActorIndex i = 0; i < dataset.actors.size(); ++i)
        indexOf.emplace(dataset.actors[i].id, i);

    std::vector<ActorPair> pairs;
    pairs.reserve(ids.size());
    for (const ActorIdPair& id : ids) {
        const auto a = indexOf.find(id.first);
        const auto b = indexOf.find(id.second);
        if (a != indexOf.end() && b != indexOf.end())
            pairs.emplace_back(a->second, b->second);
    }
    return pairs;
}

bool writePairsFile(const std::string& path, const std::vector<ActorIdPair>& ids) {
    std::ofstream out(path);
    if (!out)
        return false;
    for (const ActorIdPair& id : ids)
        out << id.first << '\t' << id.second << '\n';
    return static_cast<bool>(out);
}

std::vector<ActorIdPair> readPairsFile(const std::string& path) {
    std::vector<ActorIdPair> ids;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream words(line);
        ActorIdPair id;
        if (words >> id.first >> id.second)
            ids.push_back(std::move(id));
    }
    return ids;
}

std::vector<BenchmarkRow> runBenchmark(const Graph& graph,
                                       const std::string& datasetName,
                                       const std::vector<ActorPair>& pairs) {
    std::vector<BenchmarkRow> rows;
    rows.reserve(pairs.size() * 2);
    for (const ActorPair& pair : pairs) {
        rows.push_back(timeSearch(bfs, graph, datasetName, "bfs", pair));
        rows.push_back(timeSearch(bidirectionalBfs, graph, datasetName, "bidirectional", pair));
    }
    return rows;
}

bool writeResultsCsv(const std::string& path, const std::vector<BenchmarkRow>& rows) {
    std::ofstream out(path);
    if (!out)
        return false;
    out << "dataset,algorithm,src,dst,degrees,nodesVisited,ms\n";
    for (const BenchmarkRow& row : rows) {
        out << row.dataset << ',' << row.algorithm << ',' << row.src << ',' << row.dst << ','
            << row.degrees << ',' << row.nodesVisited << ',' << row.ms << '\n';
    }
    return static_cast<bool>(out);
}
