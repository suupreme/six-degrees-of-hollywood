#include <string>
#include <cstddef>
#pragma once
#include <vector>
#include <optional>

using ActorIndex = std::size_t;
using MovieIndex = std::size_t;
// Init'ing each object and their necessary id's
class Actor {
public:
    std::string id;
    std::string name;
    std::optional<int> birthYear;
};

class Movie {
public:
    std::string id;
    std::string title;
    std::optional<int> year;
};

class Credit {
public:
    ActorIndex actor;
    MovieIndex movie;
};

class Dataset {
public:
    std::vector<Actor> actors;
        std::vector<Movie> movies;
    
    std::vector<Credit> credits;
};

// Set up  the Graph class
struct Graph {
public:
    explicit Graph(const Dataset& source);

    std::size_t actorCount() const noexcept;
    std::size_t movieCount() const noexcept;
    std::size_t creditCount() const noexcept;

    // movies and actors are bidirectional
    const std::vector<MovieIndex>& moviesForActor(ActorIndex actor) const;
    const std::vector<ActorIndex>& actorsForMovie(MovieIndex movie) const;

private:
    
    std::vector<std::vector<MovieIndex>> newactorToMovies;
    std::vector<std::vector<ActorIndex>> newmovieToActors;
    std::size_t mycredits = 0;
};
