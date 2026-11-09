#include "bidirectional.h"

#include <algorithm>
#include <limits>

namespace {

constexpr std::size_t kUnseen = std::numeric_limits<std::size_t>::max();

struct Side {
    std::vector<std::size_t> dist;
    std::vector<ActorIndex> prev;
    std::vector<MovieIndex> prevMovie;
    std::vector<bool> movieSeen;
    std::vector<ActorIndex> frontier;
    std::size_t depth = 0;

    Side(std::size_t actors, std::size_t movies, ActorIndex start)
        : dist(actors, kUnseen), prev(actors, actors), prevMovie(actors, movies),
          movieSeen(movies, false), frontier{start} {
        dist[start] = 0;
        prev[start] = start;
    }
};

ActorIndex expandLevel(const Graph& graph, Side& self, const Side& other,
                       PathResult& result, std::size_t& best) {
    const std::size_t n = graph.actorCount();
    ActorIndex meet = n;
    std::vector<ActorIndex> next;

    for (ActorIndex curr : self.frontier) {
        for (MovieIndex movie : graph.moviesForActor(curr)) {
            if (self.movieSeen[movie])
                continue;
            self.movieSeen[movie] = true;
            if (!other.movieSeen[movie])
                ++result.moviecount;

            for (ActorIndex actor : graph.actorsForMovie(movie)) {
                if (self.dist[actor] != kUnseen)
                    continue;
                self.dist[actor] = self.depth + 1;
                self.prev[actor] = curr;
                self.prevMovie[actor] = movie;
                next.push_back(actor);

                if (other.dist[actor] == kUnseen) {
                    ++result.actorcount;
                    continue;
                }
                const std::size_t total = self.dist[actor] + other.dist[actor];
                if (total < best) {
                    best = total;
                    meet = actor;
                }
            }
        }
    }

    self.frontier = std::move(next);
    ++self.depth;
    return meet;
}

std::vector<PathStep> joinPaths(ActorIndex src, ActorIndex dst, ActorIndex meet,
                                const Side& forward, const Side& backward) {
    std::vector<PathStep> steps;
    for (ActorIndex curr = meet; curr != src; curr = forward.prev[curr])
        steps.push_back({forward.prev[curr], forward.prevMovie[curr], curr});
    std::reverse(steps.begin(), steps.end());

    for (ActorIndex curr = meet; curr != dst; curr = backward.prev[curr])
        steps.push_back({curr, backward.prevMovie[curr], backward.prev[curr]});
    return steps;
}

}  // namespace

PathResult bidirectionalBfs(const Graph& graph, ActorIndex src, ActorIndex dst) {
    PathResult result;
    const std::size_t n = graph.actorCount();
    const std::size_t m = graph.movieCount();

    if (src >= n || dst >= n) {
        result.status = PathStatus::InvalidActor;
        return result;
    }
    result.actorcount = 1;
    if (src == dst) {
        result.status = PathStatus::Found;
        return result;
    }

    Side forward(n, m, src);
    Side backward(n, m, dst);
    result.actorcount = 2;

    std::size_t best = kUnseen;
    ActorIndex meet = n;

    while (!forward.frontier.empty() && !backward.frontier.empty()) {
        const bool goForward = forward.frontier.size() <= backward.frontier.size();
        Side& self = goForward ? forward : backward;
        const Side& other = goForward ? backward : forward;

        const ActorIndex found = expandLevel(graph, self, other, result, best);
        if (found != n)
            meet = found;
        if (meet != n)
            break;
    }

    if (meet == n)
        return result;

    result.status = PathStatus::Found;
    result.offset = joinPaths(src, dst, meet, forward, backward);
    return result;
}
