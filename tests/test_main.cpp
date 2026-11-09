// Tests

#include <cstdio>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

#include "Graph.h"
#include "benchmark.h"
#include "bfs.h"
#include "bidirectional.h"
#include "loader.h"
#include "lookup.h"

#ifndef SAMPLE_DATA_DIR
#define SAMPLE_DATA_DIR "data/sample"
#endif

namespace {

int failures = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond \
                      << "\n";                                                   \
            ++failures;                                                          \
        }                                                                        \
    } while (0)

int degreesOf(const PathResult& result) {
    return result.status == PathStatus::Found ? static_cast<int>(result.offset.size()) : -1;
}

bool isValidPath(const Graph& graph, const PathResult& result, ActorIndex src, ActorIndex dst) {
    if (result.status != PathStatus::Found)
        return true;
    ActorIndex current = src;
    for (const PathStep& step : result.offset) {
        if (step.actor1 != current || step.movie >= graph.movieCount())
            return false;
        bool hasActor1 = false, hasActor2 = false;
        for (ActorIndex a : graph.actorsForMovie(step.movie)) {
            hasActor1 = hasActor1 || a == step.actor1;
            hasActor2 = hasActor2 || a == step.actor2;
        }
        if (!hasActor1 || !hasActor2)
            return false;
        current = step.actor2;
    }
    return current == dst;
}

struct Expected {
    ActorIndex src;
    ActorIndex dst;
    int degrees;
};

const Expected kExpected[] = {
    {0, 0, 0}, {0, 1, 1}, {0, 8, 1}, {0, 4, 2}, {0, 3, 3},
    {0, 6, 3}, {3, 8, 3}, {1, 6, 4}, {9, 0, -1}, {9, 9, 0},
};

void testSampleLoads(const Dataset& dataset) {
    CHECK(dataset.actors.size() == 10);
    CHECK(dataset.movies.size() == 6);
    CHECK(dataset.credits.size() == 15);
}

void testLookupDuplicateName(const Dataset& dataset) {
    const ActorLookup lookup(dataset);
    const std::vector<ActorIndex> alexes = lookup.findActor("Alex Morgan");
    CHECK((alexes == std::vector<ActorIndex>{3, 8}));
    CHECK(lookup.findActor("Nobody Here").empty());
}

void testLookupNormalizesNames(const Dataset& dataset) {
    const ActorLookup lookup(dataset);
    CHECK((lookup.findActor("  alex   MORGAN ") == std::vector<ActorIndex>{3, 8}));
    CHECK((lookup.findActor("ava brooks") == std::vector<ActorIndex>{0}));
    CHECK(lookup.findActor("").empty());
}

void testLookupDescribe(const Dataset& dataset) {
    const ActorLookup lookup(dataset);
    CHECK(lookup.describe(3) == "Alex Morgan (b. 1975, The Long Static (2008))");
    CHECK(lookup.describe(8) == "Alex Morgan (b. 1990, Second Chorus (2020))");
    CHECK(lookup.describe(7) == "Gus Hill (birth year unknown, Second Chorus (2020))");
    CHECK(lookup.describe(9) == "Hana Ito (b. 1999, no credits)");
}

void testExpectedDegrees(const Graph& graph) {
    for (const Expected& e : kExpected) {
        const PathResult a = bfs(graph, e.src, e.dst);
        const PathResult b = bidirectionalBfs(graph, e.src, e.dst);
        if (degreesOf(a) != e.degrees || !isValidPath(graph, a, e.src, e.dst) ||
            degreesOf(b) != e.degrees || !isValidPath(graph, b, e.src, e.dst)) {
            std::cerr << "  " << e.src << " -> " << e.dst << ": bfs " << degreesOf(a)
                      << ", bidirectional " << degreesOf(b) << ", expected " << e.degrees
                      << "\n";
            ++failures;
        }
    }
}

void testInvalidActor(const Graph& graph) {
    const ActorIndex bad = graph.actorCount();
    CHECK(bfs(graph, 0, bad).status == PathStatus::InvalidActor);
    CHECK(bidirectionalBfs(graph, 0, bad).status == PathStatus::InvalidActor);
    CHECK(bidirectionalBfs(graph, bad, 0).status == PathStatus::InvalidActor);
}

bool crossCheck(const Graph& graph, const std::string& label) {
    bool ok = true;
    for (ActorIndex src = 0; src < graph.actorCount(); ++src) {
        for (ActorIndex dst = 0; dst < graph.actorCount(); ++dst) {
            const PathResult a = bfs(graph, src, dst);
            const PathResult b = bidirectionalBfs(graph, src, dst);
            if (degreesOf(a) != degreesOf(b) || !isValidPath(graph, b, src, dst)) {
                std::cerr << "  " << label << " " << src << " -> " << dst << ": bfs "
                          << degreesOf(a) << ", bidirectional " << degreesOf(b) << "\n";
                ++failures;
                ok = false;
            }
        }
    }
    return ok;
}

void testCrossCheckSample(const Graph& graph) {
    crossCheck(graph, "sample");
}

Dataset randomDataset(std::mt19937_64& rng, std::size_t actors, std::size_t movies,
                      std::size_t credits) {
    Dataset dataset;
    for (std::size_t i = 0; i < actors; ++i)
        dataset.actors.push_back({"nm" + std::to_string(i), "Actor " + std::to_string(i), {}});
    for (std::size_t i = 0; i < movies; ++i)
        dataset.movies.push_back({"tt" + std::to_string(i), "Movie " + std::to_string(i), {}});
    for (std::size_t i = 0; i < credits; ++i)
        dataset.credits.push_back({rng() % actors, rng() % movies});
    return dataset;
}

void testCrossCheckRandomGraphs() {
    std::mt19937_64 rng(2026);
    for (int trial = 0; trial < 200; ++trial) {
        const std::size_t actors = 2 + rng() % 40;
        const std::size_t movies = 1 + rng() % 30;
        const std::size_t credits = rng() % (actors * 2 + 1);
        const Dataset dataset = randomDataset(rng, actors, movies, credits);
        const Graph graph(dataset);
        if (!crossCheck(graph, "random#" + std::to_string(trial)))
            return;
    }
}

void testBenchmark(const Dataset& dataset, const Graph& graph) {
    const std::vector<ActorIdPair> ids = pickPairIds(dataset, graph, 5, 7);
    CHECK(ids.size() == 5);
    CHECK(ids == pickPairIds(dataset, graph, 5, 7));

    const std::vector<ActorPair> pairs = resolvePairs(dataset, ids);
    CHECK(pairs.size() == ids.size());
    CHECK(resolvePairs(dataset, {{"nm0000000", "nm9000000"}}).empty());

    const std::vector<BenchmarkRow> rows = runBenchmark(graph, "sample", pairs);
    CHECK(rows.size() == pairs.size() * 2);
    for (std::size_t i = 0; i + 1 < rows.size(); i += 2) {
        CHECK(rows[i].algorithm == "bfs");
        CHECK(rows[i + 1].algorithm == "bidirectional");
        CHECK(rows[i].degrees == rows[i + 1].degrees);
        CHECK(rows[i].nodesVisited > 0 && rows[i + 1].nodesVisited > 0);
        CHECK(rows[i].ms >= 0.0 && rows[i + 1].ms >= 0.0);
    }

    const std::string csv = "test_results.csv";
    CHECK(writeResultsCsv(csv, rows));
    std::ifstream in(csv);
    std::string header;
    std::getline(in, header);
    CHECK(header == "dataset,algorithm,src,dst,degrees,nodesVisited,ms");
    std::size_t lines = 0;
    for (std::string line; std::getline(in, line);)
        ++lines;
    CHECK(lines == rows.size());
    in.close();
    std::remove(csv.c_str());

    const std::string pairsFile = "test_pairs.tsv";
    CHECK(writePairsFile(pairsFile, ids));
    CHECK(readPairsFile(pairsFile) == ids);
    std::remove(pairsFile.c_str());
}

}  // namespace

int main() {
    const Dataset dataset = loadDataset(SAMPLE_DATA_DIR);
    testSampleLoads(dataset);

    const Graph graph(dataset);
    testLookupDuplicateName(dataset);
    testLookupNormalizesNames(dataset);
    testLookupDescribe(dataset);
    testExpectedDegrees(graph);
    testInvalidActor(graph);
    testCrossCheckSample(graph);
    testCrossCheckRandomGraphs();
    testBenchmark(dataset, graph);

    if (failures > 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All tests passed\n";
    return 0;
}
