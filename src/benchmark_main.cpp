// Benchmark runner

#include <algorithm>
#include <cctype>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "Graph.h"
#include "benchmark.h"
#include "loader.h"

namespace {

bool isNumber(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(),
                                     [](unsigned char c) { return std::isdigit(c); });
}

int usage(const char* program) {
    std::cerr << "usage: " << program << " OUT.csv PAIRS DATA_DIR [DATA_DIR...]\n"
              << "  PAIRS is a number of random pairs picked from the first DATA_DIR,\n"
              << "  or a file with two nm ids per line\n";
    return 2;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 4)
        return usage(argv[0]);

    const std::string outCsv = argv[1];
    const std::string pairsArg = argv[2];
    const std::vector<std::string> dirs(argv + 3, argv + argc);

    try {
        std::vector<ActorIdPair> ids;
        if (isNumber(pairsArg)) {
            const Dataset first = loadDataset(dirs.front());
            const Graph graph(first);
            ids = pickPairIds(first, graph, std::stoul(pairsArg));
            const std::string pairsFile = outCsv + ".pairs.tsv";
            if (writePairsFile(pairsFile, ids))
                std::cout << "Saved " << ids.size() << " pairs to " << pairsFile << "\n";
        } else {
            ids = readPairsFile(pairsArg);
        }
        if (ids.empty()) {
            std::cerr << "No actor pairs to benchmark\n";
            return 1;
        }

        std::vector<BenchmarkRow> rows;
        int mismatches = 0;
        for (const std::string& dir : dirs) {
            const Dataset dataset = loadDataset(dir);
            const Graph graph(dataset);
            const std::vector<ActorPair> pairs = resolvePairs(dataset, ids);
            const std::string name = std::filesystem::path(dir).filename().string();

            if (pairs.size() != ids.size())
                std::cerr << name << ": " << ids.size() - pairs.size()
                          << " pair(s) not in this dataset, skipped\n";
            std::cout << "Benchmarking " << pairs.size() << " pairs on " << name << " ("
                      << graph.actorCount() << " actors, " << graph.movieCount()
                      << " movies)\n";

            std::vector<BenchmarkRow> result = runBenchmark(graph, name, pairs);
            for (std::size_t i = 0; i + 1 < result.size(); i += 2) {
                if (result[i].degrees != result[i + 1].degrees) {
                    std::cerr << name << ": algorithms disagree on " << result[i].src
                              << " -> " << result[i].dst << "\n";
                    ++mismatches;
                }
            }
            rows.insert(rows.end(), result.begin(), result.end());
        }

        if (!writeResultsCsv(outCsv, rows)) {
            std::cerr << "Could not write " << outCsv << "\n";
            return 1;
        }
        std::cout << "Wrote " << rows.size() << " rows to " << outCsv << "\n";
        return mismatches == 0 ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
