#include "lookup.h"

#include <cctype>

namespace {

std::string normalize(const std::string& name) {
    std::string key;
    bool pendingSpace = false;
    for (unsigned char c : name) {
        if (std::isspace(c)) {
            pendingSpace = !key.empty();
            continue;
        }
        if (pendingSpace)
            key += ' ';
        pendingSpace = false;
        key += static_cast<char>(std::tolower(c));
    }
    return key;
}

bool isNewer(const Movie& a, const Movie& b) {
    if (!a.year)
        return false;
    return !b.year || *a.year > *b.year;
}

}  // namespace

ActorLookup::ActorLookup(const Dataset& dataset)
    : dataset_(dataset), knownMovie_(dataset.actors.size(), dataset.movies.size()) {
    for (ActorIndex i = 0; i < dataset.actors.size(); ++i)
        byName_[normalize(dataset.actors[i].name)].push_back(i);

    for (const Credit& credit : dataset.credits) {
        MovieIndex& known = knownMovie_[credit.actor];
        if (known == dataset.movies.size() ||
            isNewer(dataset.movies[credit.movie], dataset.movies[known]))
            known = credit.movie;
    }
}

std::vector<ActorIndex> ActorLookup::findActor(const std::string& name) const {
    const auto it = byName_.find(normalize(name));
    if (it == byName_.end())
        return {};
    return it->second;
}

MovieIndex ActorLookup::knownMovie(ActorIndex actor) const {
    return actor < knownMovie_.size() ? knownMovie_[actor] : dataset_.movies.size();
}

std::string ActorLookup::describe(ActorIndex actor) const {
    if (actor >= dataset_.actors.size())
        return "unknown actor";

    const Actor& a = dataset_.actors[actor];
    std::string text = a.name + " (";
    text += a.birthYear ? "b. " + std::to_string(*a.birthYear) : "birth year unknown";

    const MovieIndex movie = knownMovie_[actor];
    if (movie == dataset_.movies.size()) {
        text += ", no credits";
    } else {
        const Movie& m = dataset_.movies[movie];
        text += ", " + m.title;
        if (m.year)
            text += " (" + std::to_string(*m.year) + ")";
    }
    return text + ")";
}
