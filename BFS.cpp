#include "Search.h"
#include <algorithm>
#include <queue>

namespace {

std::vector<PathStep> reconstructPath(
    ActorIndex src, ActorIndex dst,
    const std::vector<ActorIndex>& prev,
    const std::vector<MovieIndex>& prevm) {
    std::vector<PathStep> sol;
    ActorIndex curr = dst;
    while (curr !=src) {
        sol.push_back({prev[curr], prevm[curr], curr});
        curr = prev[curr];
    }
    std::reverse(sol.begin(), sol.end());
    return sol;
}

}

PathResult bfs(const Graph& graph, ActorIndex src, ActorIndex dst) {
    PathResult sol;
    std::size_t n = graph.actorCount();
    std::size_t m = graph.movieCount();

    if (src >= n || dst >= n) {
        sol.status = PathStatus::InvalidActor;
        return sol;
    }
    sol.actorcount = 1;
    if (src == dst) {
        sol.status = PathStatus::Found;
        return sol;
    }

    // n marks actors that do not have a parent yet.
    std::vector<ActorIndex> prev(n, n);
    std::vector<MovieIndex> prevm(n,m);
    std::vector<bool> seen(m, false);
    std::queue<ActorIndex> queue;
    prev[src]= src;
    queue.push(src);

    while (!queue.empty()) {
        ActorIndex curr = queue.front();
        queue.pop();

        for (MovieIndex i : graph.moviesForActor(curr)) {
            if (seen[i])
                continue;
            seen[i] = true;
            sol.moviecount++;

            for (ActorIndex j: graph.actorsForMovie(i)) {
                if (prev[j] != n)
                    continue;
                prev[j] = curr;
                prevm[j] = i;
                sol.actorcount++;

                if (j ==dst) {
                    sol.status = PathStatus::Found;
                    sol.offset = reconstructPath(src, dst, prev, prevm);
                    return sol;
                }
                queue.push(j);
            }
        }
    }
    return sol;
}
