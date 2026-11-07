#pragma once

#include "bfs.h"

// Bidirectional BFS from `src` and `dst` at the same time, expanding the
// smaller frontier each round and stopping once the two searches meet.
//
// Returns a PathResult with the same meaning as bfs() in bfs.h, so the
// two can be compared directly:
//   status      Found, NotFound, or InvalidActor if src or dst is out of range
//   offset      the steps actor1 -> movie -> actor2, in order from src to dst;
//               empty when src == dst or no path exists
//   actorcount  actors discovered by either frontier (src and dst included)
//   moviecount  movies expanded by either frontier
// Degrees of separation = offset.size() when status == Found.
// Must return the same number of degrees as bfs() for every pair; the chosen
// path may differ when several shortest paths exist.
PathResult bidirectionalBfs(const Graph& graph, ActorIndex src, ActorIndex dst);
