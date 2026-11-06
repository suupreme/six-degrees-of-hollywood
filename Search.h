#include "Graph.h"
#pragma once
// Init'ing the pathstep class
class PathStep {
public:
    ActorIndex actor1;
    MovieIndex movie;
    ActorIndex actor2;
};
enum struct PathStatus {
    Found,
    NotFound,
    InvalidActor
};
struct PathResult {
public:
    PathStatus status = PathStatus::NotFound;
    std::vector<PathStep> offset;
    std::size_t actorcount = 0;
    std::size_t moviecount = 0;
};

// Searching the graph
PathResult bfs(const Graph& graph, ActorIndex src, ActorIndex dst);
