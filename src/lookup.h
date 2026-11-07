#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Graph.h"

// Maps an actor's name to every actor index with that name. Names are not
// unique in IMDb (two actors called "Alex Morgan" with different birth
// years), so callers get a list and decide how to choose, for example by
// showing each match with its birth year.
class ActorLookup {
public:
    // Builds the name index from the dataset's actors.
    explicit ActorLookup(const Dataset& dataset);

    // Returns the indices of all actors whose name matches `name`, in
    // ascending index order. Returns an empty list when there is no match.
    // TODO(Daniel): decide on and document matching rules (exact match vs.
    // case/whitespace-insensitive).
    std::vector<ActorIndex> findActor(const std::string& name) const;

private:
    std::unordered_map<std::string, std::vector<ActorIndex>> byName_;
};
