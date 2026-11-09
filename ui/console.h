#pragma once

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "Graph.h"
#include "bfs.h"
#include "bidirectional.h"
#include "lookup.h"

inline std::string toLower(std::string text) {
    for (char& c : text) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}

inline std::string movieText(const Dataset& dataset, MovieIndex movie) {
    const Movie& m = dataset.movies[movie];
    if (m.year) {
        return m.title + " (" + std::to_string(*m.year) + ")";
    }
    return m.title;
}

// Used to tell apart actors with the same name.
inline std::string actorInfo(const Dataset& dataset, const Graph& graph, ActorIndex actor) {
    const Actor& a = dataset.actors[actor];
    std::string text = a.name;
    if (a.birthYear) {
        text += " (b. " + std::to_string(*a.birthYear) + ")";
    }
    text += " [" + a.id + "]";

    const std::vector<MovieIndex>& movies = graph.moviesForActor(actor);
    if (movies.empty()) {
        text += ", no movies";
    } else {
        text += ", known for " + movieText(dataset, movies[0]);
    }
    return text;
}

// Turns typed text (a name or an nm... id) into one actor. Returns false if none was chosen.
inline bool chooseActor(const Dataset& dataset, const Graph& graph, const ActorLookup& lookup, const std::string& input, ActorIndex& result) {
    std::vector<ActorIndex> matches = lookup.findActor(input);

    // No exact match: try the id, or the name ignoring upper/lower case.
    if (matches.empty()) {
        std::string lowerInput = toLower(input);
        for (ActorIndex i = 0; i < dataset.actors.size(); i++) {
            if (dataset.actors[i].id == input || toLower(dataset.actors[i].name) == lowerInput) {
                matches.push_back(i);
            }
        }
    }

    if (matches.empty()) {
        std::cout << "No actor named \"" << input << "\".\n";
        std::string lowerInput = toLower(input);
        int shown = 0;
        for (ActorIndex i = 0; i < dataset.actors.size() && shown < 5; i++) {
            if (toLower(dataset.actors[i].name).find(lowerInput) != std::string::npos) {
                if (shown == 0) {
                    std::cout << "Did you mean:\n";
                }
                std::cout << "  " << actorInfo(dataset, graph, i) << "\n";
                shown++;
            }
        }
        return false;
    }

    if (matches.size() == 1) {
        result = matches[0];
        return true;
    }

    // Several actors share this name, so let the user pick one.
    std::size_t count = std::min<std::size_t>(matches.size(), 20);
    std::cout << matches.size() << " actors are named \"" << input << "\":\n";
    for (std::size_t i = 0; i < count; i++) {
        std::cout << "  " << (i + 1) << ") " << actorInfo(dataset, graph, matches[i]) << "\n";
    }

    while (true) {
        std::cout << "Pick 1-" << count << " (blank to cancel): ";
        std::string line;
        if (!std::getline(std::cin, line) || line.empty()) {
            return false;
        }
        int choice = std::atoi(line.c_str());
        if (choice >= 1 && choice <= static_cast<int>(count)) {
            result = matches[choice - 1];
            return true;
        }
        std::cout << "Please enter a number from 1 to " << count << ".\n";
    }
}

// Runs "bfs" or "bidirectional" and stores how long it took in ms.
inline PathResult runSearch(const Graph& graph, ActorIndex src, ActorIndex dst, const std::string& algorithm, double& ms) {
    auto start = std::chrono::steady_clock::now();
    PathResult result;
    if (algorithm == "bfs") {
        result = bfs(graph, src, dst);
    } else {
        result = bidirectionalBfs(graph, src, dst);
    }
    auto end = std::chrono::steady_clock::now();
    ms = std::chrono::duration<double, std::milli>(end - start).count();
    return result;
}

inline int degreesOf(const PathResult& result) {
    if (result.status != PathStatus::Found) {
        return -1;
    }
    return static_cast<int>(result.offset.size());
}

inline void printPath(const Dataset& dataset, ActorIndex src, ActorIndex dst, const PathResult& result) {
    if (src == dst) {
        std::cout << "Same actor entered twice: 0 degrees.\n";
        return;
    }
    if (result.status != PathStatus::Found) {
        std::cout << "No connection between " << dataset.actors[src].name << " and " << dataset.actors[dst].name << ".\n";
        return;
    }

    std::cout << dataset.actors[src].name;
    for (const PathStep& step : result.offset) {
        std::cout << " -> " << movieText(dataset, step.movie) << " -> " << dataset.actors[step.actor2].name;
    }
    std::cout << "\nDegrees of separation: " << result.offset.size() << "\n";
}

inline void printStatsHeader() {
    std::cout << std::left << std::setw(15) << "Algorithm" << std::setw(10) << "Degrees" << std::setw(14) << "Actors seen" << std::setw(14) << "Movies seen" << "Time (ms)\n";
}

inline void printStats(const std::string& algorithm, const PathResult& result, double ms) {
    std::cout << std::left << std::setw(15) << algorithm << std::setw(10) << degreesOf(result) << std::setw(14) << result.actorcount << std::setw(14) << result.moviecount << std::fixed << std::setprecision(3) << ms << "\n";
}
