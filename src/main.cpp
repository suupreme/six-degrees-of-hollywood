// Six Degrees of Hollywood
//
// Usage: six_degrees [dataDir] [options]
//   --from NAME --to NAME       run one search instead of asking
//   --algo bfs|bidir|both       which search to run (default both)
//   --export FILE               save each search as JSON for viz/index.html
//   --benchmark PAIRS OUT.csv   PAIRS is a number of random pairs, or a file with two nm ids per line

#include <cctype>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include "Graph.h"
#include "benchmark.h"
#include "bfs.h"
#include "bidirectional.h"
#include "loader.h"
#include "lookup.h"
#include "../ui/console.h"
#include "../ui/path_export.h"

struct Options {
    std::string dataDir = "data/sample";
    std::string algorithm = "both";
    std::string from;
    std::string to;
    std::string exportFile;
    std::string benchmarkPairs;
    std::string benchmarkOut;
};

void printUsage() {
    std::cout << "Usage: six_degrees [dataDir] [options]\n";
    std::cout << "  --from NAME --to NAME       run one search instead of asking\n";
    std::cout << "  --algo bfs|bidir|both       which search to run (default both)\n";
    std::cout << "  --export FILE               save each search as JSON for viz/index.html\n";
    std::cout << "  --benchmark PAIRS OUT.csv   PAIRS is a number of random pairs, or a file of nm id pairs\n";
}

// Returns false if the arguments are wrong.
bool parseArgs(int argc, char* argv[], Options& options) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        bool hasValue = i + 1 < argc;

        if (arg == "--help") {
            return false;
        } else if (arg == "--from" && hasValue) {
            options.from = argv[++i];
        } else if (arg == "--to" && hasValue) {
            options.to = argv[++i];
        } else if (arg == "--algo" && hasValue) {
            options.algorithm = argv[++i];
        } else if (arg == "--export" && hasValue) {
            options.exportFile = argv[++i];
        } else if (arg == "--benchmark" && i + 2 < argc) {
            options.benchmarkPairs = argv[++i];
            options.benchmarkOut = argv[++i];
        } else if (!arg.empty() && arg[0] != '-') {
            options.dataDir = arg;
        } else {
            std::cout << "Unknown or incomplete option: " << arg << "\n";
            return false;
        }
    }

    if (options.algorithm == "bidirectional") {
        options.algorithm = "bidir";
    }
    if (options.algorithm != "bfs" && options.algorithm != "bidir" && options.algorithm != "both") {
        std::cout << "--algo must be bfs, bidir or both\n";
        return false;
    }
    if (options.from.empty() != options.to.empty()) {
        std::cout << "--from and --to must be used together\n";
        return false;
    }
    return true;
}

// Runs the chosen algorithm(s), prints the results and exports the JSON if asked.
void search(const Dataset& dataset, const Graph& graph, const Options& options, ActorIndex src, ActorIndex dst) {
    std::vector<std::string> algorithms;
    if (options.algorithm != "bidir") {
        algorithms.push_back("bfs");
    }
    if (options.algorithm != "bfs") {
        algorithms.push_back("bidirectional");
    }

    std::vector<PathResult> results;
    std::vector<double> times;
    for (const std::string& algorithm : algorithms) {
        double ms = 0;
        results.push_back(runSearch(graph, src, dst, algorithm, ms));
        times.push_back(ms);
    }

    std::cout << "\n";
    printPath(dataset, src, dst, results[0]);
    std::cout << "\n";
    printStatsHeader();
    for (std::size_t i = 0; i < algorithms.size(); i++) {
        printStats(algorithms[i], results[i], times[i]);
    }
    if (results.size() == 2 && degreesOf(results[0]) != degreesOf(results[1])) {
        std::cout << "WARNING: the two algorithms disagree on the degrees.\n";
    }

    if (!options.exportFile.empty()) {
        if (exportPath(options.exportFile, dataset, graph, src, dst, algorithms[0], results[0], times[0])) {
            std::cout << "Saved to " << options.exportFile << "\n";
        } else {
            std::cout << "Could not write " << options.exportFile << "\n";
        }
    }
}

// Asks for an actor until one is chosen. Returns false if the user leaves it blank to quit.
bool askForActor(const Dataset& dataset, const Graph& graph, const ActorLookup& lookup, const std::string& prompt, ActorIndex& actor) {
    while (true) {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line) || line.empty()) {
            return false;
        }
        if (chooseActor(dataset, graph, lookup, line, actor)) {
            std::cout << "  " << actorInfo(dataset, graph, actor) << "\n";
            return true;
        }
    }
}

void runInteractive(const Dataset& dataset, const Graph& graph, const ActorLookup& lookup, const Options& options) {
    std::cout << "Enter two actors (name or nm id). Leave blank to quit.\n";
    while (true) {
        ActorIndex src;
        ActorIndex dst;
        std::cout << "\n";
        if (!askForActor(dataset, graph, lookup, "First actor: ", src)) {
            break;
        }
        if (!askForActor(dataset, graph, lookup, "Second actor: ", dst)) {
            break;
        }
        search(dataset, graph, options, src, dst);
    }
}

std::vector<ActorPair> randomPairs(const Graph& graph, int count) {
    std::vector<ActorIndex> actors;
    for (ActorIndex a = 0; a < graph.actorCount(); a++) {
        if (!graph.moviesForActor(a).empty()) {
            actors.push_back(a);
        }
    }

    std::vector<ActorPair> pairs;
    if (actors.size() < 2) {
        return pairs;
    }
    std::mt19937 rng(42);  // fixed seed so every run uses the same pairs
    std::uniform_int_distribution<std::size_t> pick(0, actors.size() - 1);
    while (static_cast<int>(pairs.size()) < count) {
        ActorIndex a = actors[pick(rng)];
        ActorIndex b = actors[pick(rng)];
        if (a != b) {
            pairs.push_back(ActorPair(a, b));
        }
    }
    return pairs;
}

// Reads lines like "nm0000158  nm0000102". Ids not in this dataset are skipped.
std::vector<ActorPair> pairsFromFile(const std::string& file, const Dataset& dataset) {
    std::unordered_map<std::string, ActorIndex> indexOfId;
    for (ActorIndex i = 0; i < dataset.actors.size(); i++) {
        indexOfId[dataset.actors[i].id] = i;
    }

    std::vector<ActorPair> pairs;
    std::ifstream in(file);
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream words(line);
        std::string idA;
        std::string idB;
        words >> idA >> idB;
        if (indexOfId.count(idA) > 0 && indexOfId.count(idB) > 0) {
            pairs.push_back(ActorPair(indexOfId[idA], indexOfId[idB]));
        }
    }
    return pairs;
}

int runBenchmarkMode(const Dataset& dataset, const Graph& graph, const Options& options) {
    bool isNumber = true;
    for (char c : options.benchmarkPairs) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            isNumber = false;
        }
    }

    std::vector<ActorPair> pairs;
    if (isNumber) {
        pairs = randomPairs(graph, std::stoi(options.benchmarkPairs));
    } else {
        pairs = pairsFromFile(options.benchmarkPairs, dataset);
    }
    if (pairs.empty()) {
        std::cout << "No actor pairs to benchmark.\n";
        return 1;
    }

    // Name the dataset after its folder, e.g. "data/small" -> "small".
    std::string name = options.dataDir;
    while (name.size() > 1 && (name.back() == '/' || name.back() == '\\')) {
        name.pop_back();
    }
    std::size_t slash = name.find_last_of("/\\");
    if (slash != std::string::npos) {
        name = name.substr(slash + 1);
    }

    std::cout << "Benchmarking " << pairs.size() << " pairs on " << name << "...\n";
    std::vector<BenchmarkRow> rows = runBenchmark(graph, name, pairs);
    if (!writeResultsCsv(options.benchmarkOut, rows)) {
        std::cout << "Could not write " << options.benchmarkOut << "\n";
        return 1;
    }
    std::cout << "Wrote " << rows.size() << " rows to " << options.benchmarkOut << "\n";
    return 0;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // So names with accents show up correctly in the Windows console.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    Options options;
    if (!parseArgs(argc, argv, options)) {
        printUsage();
        return 1;
    }

    std::cout << "Loading " << options.dataDir << "...\n";
    Dataset dataset;
    try {
        dataset = loadDataset(options.dataDir);
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
        return 1;
    }
    Graph graph(dataset);
    ActorLookup lookup(dataset);
    std::cout << graph.actorCount() << " actors, " << graph.movieCount() << " movies, " << graph.creditCount() << " credits\n";

    if (graph.actorCount() == 0) {
        std::cout << "No actors loaded. Check that the folder has actors.tsv, movies.tsv and edges.tsv.\n";
        return 1;
    }

    if (!options.benchmarkPairs.empty()) {
        return runBenchmarkMode(dataset, graph, options);
    }

    if (options.from.empty()) {
        runInteractive(dataset, graph, lookup, options);
        return 0;
    }

    ActorIndex src;
    ActorIndex dst;
    if (!chooseActor(dataset, graph, lookup, options.from, src)) {
        return 1;
    }
    if (!chooseActor(dataset, graph, lookup, options.to, dst)) {
        return 1;
    }
    search(dataset, graph, options, src, dst);
    return 0;
}
