// Six Degrees of Hollywood

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

    return 0;
}
