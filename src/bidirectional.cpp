#include "bidirectional.h"

// TODO(Daniel): implement bidirectional BFS as described in bidirectional.h.
// Placeholder: reports NotFound for every pair.
PathResult bidirectionalBfs([[maybe_unused]] const Graph& graph,
                            [[maybe_unused]] ActorIndex src,
                            [[maybe_unused]] ActorIndex dst) {
    PathResult result;
    result.status = PathStatus::NotFound;
    return result;
}
