// Tests against the hand-written dataset in data/sample/.
// No framework: each CHECK prints a message on failure, and the program exits
// with 1 if any CHECK failed. (CHECK is used instead of assert() so the tests
// still run in Release builds, where NDEBUG removes asserts.)
//
// Until loadDataset() and bidirectionalBfs() are implemented these tests are
// expected to fail.

#include <iostream>
#include <string>

#include "Graph.h"
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

// Degrees of separation = number of movies on the path; -1 if not found.
int degreesOf(const PathResult& result) {
    return result.status == PathStatus::Found ? static_cast<int>(result.offset.size()) : -1;
}

// True if `result` is a real path from src to dst: each step's two actors
// both appear in the step's movie, and consecutive steps share an actor.
bool isValidPath(const Graph& graph, const PathResult& result, ActorIndex src, ActorIndex dst) {
    if (result.status != PathStatus::Found)
        return true;  // nothing to check
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

// Same table as data/sample/README.md.
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

void testExpectedDegrees(const Graph& graph) {
    for (const Expected& e : kExpected) {
        const PathResult result = bfs(graph, e.src, e.dst);
        if (degreesOf(result) != e.degrees || !isValidPath(graph, result, e.src, e.dst)) {
            std::cerr << "  bfs " << e.src << " -> " << e.dst << ": got "
                      << degreesOf(result) << ", expected " << e.degrees << "\n";
            ++failures;
        }
    }
}

// The main cross-check: on every pair of sample actors, bfs and
// bidirectionalBfs must agree on the number of degrees, and both paths must
// be valid. The paths themselves may differ when there is a tie.
void testBfsMatchesBidirectional(const Graph& graph) {
    for (ActorIndex src = 0; src < graph.actorCount(); ++src) {
        for (ActorIndex dst = 0; dst < graph.actorCount(); ++dst) {
            const PathResult a = bfs(graph, src, dst);
            const PathResult b = bidirectionalBfs(graph, src, dst);
            if (degreesOf(a) != degreesOf(b) || !isValidPath(graph, b, src, dst)) {
                std::cerr << "  " << src << " -> " << dst << ": bfs " << degreesOf(a)
                          << ", bidirectional " << degreesOf(b) << "\n";
                ++failures;
            }
        }
    }
}

}  // namespace

int main() {
    const Dataset dataset = loadDataset(SAMPLE_DATA_DIR);
    testSampleLoads(dataset);

    if (dataset.actors.empty()) {
        std::cerr << "loadDataset(\"" << SAMPLE_DATA_DIR
                  << "\") returned no actors; skipping graph tests\n";
    } else {
        const Graph graph(dataset);
        testLookupDuplicateName(dataset);
        testExpectedDegrees(graph);
        testBfsMatchesBidirectional(graph);
    }

    if (failures > 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All tests passed\n";
    return 0;
}
