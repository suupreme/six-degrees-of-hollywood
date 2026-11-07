// Six Degrees of Hollywood: command-line entry point.
//
// Planned usage (TODO(Josh): finalize flags and update the README):
//   six_degrees <dataDir>                          interactive: prompt for two names
//   six_degrees <dataDir> --benchmark <pairs> <out.csv>
//   six_degrees <dataDir> --export <path.json>     write the last path for viz/
//
// Flow: loadDataset() -> Graph -> ActorLookup to turn names into indices
// (ask the user to pick when a name matches several actors) -> bfs() and
// bidirectionalBfs() -> print the path using names and titles from the
// Dataset -> optionally export it in the JSON shape of viz/path.json.

#include <iostream>
#include <string>

#include "Graph.h"
#include "bfs.h"
#include "benchmark.h"
#include "bidirectional.h"
#include "loader.h"
#include "lookup.h"

int main(int argc, char* argv[]) {
    const std::string dataDir = argc > 1 ? argv[1] : "data/sample";

    const Dataset dataset = loadDataset(dataDir);
    const Graph graph(dataset);
    const ActorLookup lookup(dataset);

    std::cout << "Loaded " << graph.actorCount() << " actors, " << graph.movieCount()
              << " movies, " << graph.creditCount() << " credits from " << dataDir << "\n";

    // TODO(Josh): read two actor names, resolve them with lookup.findActor(),
    // run both searches, and print the path and stats.
    // TODO(Josh): --benchmark mode using runBenchmark() and writeResultsCsv().
    // TODO(Josh): --export mode writing the JSON shape in viz/path.json.
    return 0;
}
