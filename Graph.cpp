#include <stdexcept>
#include <string>
#include "Graph.h"

Graph::Graph(const Dataset& source)
    : newactorToMovies(source.actors.size()), newmovieToActors(source.movies.size()) {
    std::vector<std::size_t> actorDegrees(source.actors.size(), 0);
    std::vector<std::size_t> movieDegrees(source.movies.size(), 0);

    // Finding size of each AdjList, and making sure we do not go out of bounds
    for (std::size_t i = 0; i < source.credits.size(); i++) {
        const Credit& rel = source.credits[i];
        if (rel.actor >= actorDegrees.size())
            throw std::out_of_range("Invalid actor index in credit " + std::to_string(i+1));

        if (rel.movie >= movieDegrees.size())
            throw std::out_of_range("Invalid movie index in credit " + std::to_string(i+1));
        ++actorDegrees[rel.actor];
        ++movieDegrees[rel.movie];
    }

    // Create one reservation per node
    for (ActorIndex i = 0; i < actorDegrees.size(); i++) {
        newactorToMovies[i].reserve(actorDegrees[i]);
    }
    for (MovieIndex i = 0; i < movieDegrees.size(); i++) {
        newmovieToActors[i].reserve(movieDegrees[i]);
    }
    // creating one entry in each direction
    for (const Credit& i : source.credits) {
        newactorToMovies[i.actor].push_back(i.movie);
        newmovieToActors[i.movie].push_back(i.actor);
    }
    mycredits = source.credits.size();
}

// Returns the index of the movie in the graph
const std::vector<MovieIndex>& Graph::moviesForActor(ActorIndex actor) const {
    return newactorToMovies.at(actor);
}
// Returns the amount of movies
std::size_t Graph::movieCount() const noexcept {
    return newmovieToActors.size();
}
// Returns the amount of credits
std::size_t Graph::creditCount() const noexcept {
    return mycredits;
}
// Returns the index of the actor in the graph
const std::vector<ActorIndex>& Graph::actorsForMovie(MovieIndex movie) const {
    return newmovieToActors.at(movie);
}
// Returns the amount of actors
std::size_t Graph::actorCount() const noexcept {
    return newactorToMovies.size();
}
