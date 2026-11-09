#pragma once

#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "Graph.h"
#include "bfs.h"
#include "console.h"

// Adds quotes and escapes characters that would break the JSON.
inline std::string jsonText(const std::string& text) {
    std::string out = "\"";
    for (char c : text) {
        if (c == '"' || c == '\\') {
            out += '\\';
            out += c;
        } else if (c == '\n' || c == '\t' || c == '\r') {
            out += ' ';
        } else {
            out += c;
        }
    }
    return out + "\"";
}

inline std::string nodeJson(const std::string& id, const std::string& label, const std::string& type, bool onPath) {
    std::string onPathText = onPath ? "true" : "false";
    return "{\"id\": " + jsonText(id) + ", \"label\": " + jsonText(label) + ", \"type\": \"" + type + "\", \"onPath\": " + onPathText + "}";
}

inline std::string edgeJson(const std::string& from, const std::string& to, bool onPath) {
    std::string onPathText = onPath ? "true" : "false";
    return "{\"from\": " + jsonText(from) + ", \"to\": " + jsonText(to) + ", \"onPath\": " + onPathText + "}";
}

// Writes the path for viz/index.html, plus a few nearby movies and actors around it.
// Path nodes are written first, in order from source to target.
inline bool exportPath(const std::string& file, const Dataset& dataset, const Graph& graph, ActorIndex src, ActorIndex dst, const std::string& algorithm, const PathResult& result, double ms) {
    const std::size_t neighborsPerNode = 3;
    std::vector<std::string> nodes;
    std::vector<std::string> edges;
    std::unordered_set<std::string> added;

    std::vector<ActorIndex> pathActors;
    std::vector<MovieIndex> pathMovies;
    pathActors.push_back(src);
    added.insert(dataset.actors[src].id);
    nodes.push_back(nodeJson(dataset.actors[src].id, dataset.actors[src].name, "actor", true));

    if (result.status == PathStatus::Found) {
        for (const PathStep& step : result.offset) {
            const Movie& movie = dataset.movies[step.movie];
            const Actor& next = dataset.actors[step.actor2];
            nodes.push_back(nodeJson(movie.id, movieText(dataset, step.movie), "movie", true));
            nodes.push_back(nodeJson(next.id, next.name, "actor", true));
            edges.push_back(edgeJson(dataset.actors[step.actor1].id, movie.id, true));
            edges.push_back(edgeJson(movie.id, next.id, true));
            added.insert(movie.id);
            added.insert(next.id);
            pathMovies.push_back(step.movie);
            pathActors.push_back(step.actor2);
        }
    } else if (src != dst) {
        // No path, but still show both actors.
        pathActors.push_back(dst);
        added.insert(dataset.actors[dst].id);
        nodes.push_back(nodeJson(dataset.actors[dst].id, dataset.actors[dst].name, "actor", true));
    }

    // Nearby nodes: other movies of each path actor.
    for (ActorIndex actor : pathActors) {
        std::size_t count = 0;
        for (MovieIndex m : graph.moviesForActor(actor)) {
            const Movie& movie = dataset.movies[m];
            if (count == neighborsPerNode) {
                break;
            }
            if (added.count(movie.id) == 0) {
                added.insert(movie.id);
                nodes.push_back(nodeJson(movie.id, movieText(dataset, m), "movie", false));
                edges.push_back(edgeJson(dataset.actors[actor].id, movie.id, false));
                count++;
            }
        }
    }

    // Nearby nodes: other actors of each path movie.
    for (MovieIndex movie : pathMovies) {
        std::size_t count = 0;
        for (ActorIndex a : graph.actorsForMovie(movie)) {
            const Actor& actor = dataset.actors[a];
            if (count == neighborsPerNode) {
                break;
            }
            if (added.count(actor.id) == 0) {
                added.insert(actor.id);
                nodes.push_back(nodeJson(actor.id, actor.name, "actor", false));
                edges.push_back(edgeJson(dataset.movies[movie].id, actor.id, false));
                count++;
            }
        }
    }

    std::ofstream out(file);
    if (!out) {
        return false;
    }
    out << "{\n";
    out << "  \"sourceName\": " << jsonText(dataset.actors[src].name) << ",\n";
    out << "  \"targetName\": " << jsonText(dataset.actors[dst].name) << ",\n";
    out << "  \"algorithm\": \"" << algorithm << "\",\n";
    out << "  \"degrees\": " << (src == dst ? 0 : degreesOf(result)) << ",\n";
    out << "  \"actorsVisited\": " << result.actorcount << ",\n";
    out << "  \"moviesVisited\": " << result.moviecount << ",\n";
    out << "  \"ms\": " << ms << ",\n";

    out << "  \"nodes\": [\n";
    for (std::size_t i = 0; i < nodes.size(); i++) {
        out << "    " << nodes[i] << (i + 1 < nodes.size() ? ",\n" : "\n");
    }
    out << "  ],\n";

    out << "  \"edges\": [\n";
    for (std::size_t i = 0; i < edges.size(); i++) {
        out << "    " << edges[i] << (i + 1 < edges.size() ? ",\n" : "\n");
    }
    out << "  ]\n";
    out << "}\n";
    return true;
}
