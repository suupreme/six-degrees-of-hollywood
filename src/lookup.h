#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Graph.h"

// Actor name lookup
class ActorLookup {
public:
    explicit ActorLookup(const Dataset& dataset);

    std::vector<ActorIndex> findActor(const std::string& name) const;
    std::string describe(ActorIndex actor) const;

private:
    const Dataset& dataset_;
    std::unordered_map<std::string, std::vector<ActorIndex>> byName_;
    std::vector<MovieIndex> knownMovie_;
};
