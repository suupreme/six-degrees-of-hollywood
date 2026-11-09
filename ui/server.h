#pragma once

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using SocketHandle = SOCKET;
inline void closeSocket(SocketHandle s) { closesocket(s); }
inline bool validSocket(SocketHandle s) { return s != INVALID_SOCKET; }
#else
#include <arpa/inet.h>
#include <csignal>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using SocketHandle = int;
inline void closeSocket(SocketHandle s) { close(s); }
inline bool validSocket(SocketHandle s) { return s >= 0; }
#endif

#include "Graph.h"
#include "bfs.h"
#include "bidirectional.h"
#include "console.h"
#include "lookup.h"
#include "path_export.h"

// Local web server
struct HttpResponse {
    int status = 200;
    std::string type = "application/json";
    std::string body;
};

inline std::string urlDecode(const std::string& text) {
    std::string out;
    for (std::size_t i = 0; i < text.size(); i++) {
        if (text[i] == '+') {
            out += ' ';
        } else if (text[i] == '%' && i + 2 < text.size()) {
            out += static_cast<char>(std::stoi(text.substr(i + 1, 2), nullptr, 16));
            i += 2;
        } else {
            out += text[i];
        }
    }
    return out;
}

inline std::unordered_map<std::string, std::string> parseQuery(const std::string& query) {
    std::unordered_map<std::string, std::string> params;
    std::stringstream parts(query);
    std::string part;
    while (std::getline(parts, part, '&')) {
        std::size_t eq = part.find('=');
        if (eq == std::string::npos) {
            params[urlDecode(part)] = "";
        } else {
            try {
                params[urlDecode(part.substr(0, eq))] = urlDecode(part.substr(eq + 1));
            } catch (const std::exception&) {
            }
        }
    }
    return params;
}

inline HttpResponse jsonError(int status, const std::string& message) {
    return {status, "application/json", "{\"error\": " + jsonText(message) + "}"};
}

class WebServer {
public:
    WebServer(const Dataset& dataset, const Graph& graph, const ActorLookup& lookup,
              std::string webDir, std::string datasetName)
        : dataset_(dataset), graph_(graph), lookup_(lookup),
          webDir_(std::move(webDir)), datasetName_(std::move(datasetName)) {
        for (ActorIndex i = 0; i < dataset_.actors.size(); i++) {
            indexOfId_[dataset_.actors[i].id] = i;
            lowerNames_.push_back(toLower(dataset_.actors[i].name));
        }
    }

    HttpResponse handle(const std::string& method, const std::string& target) const {
        if (method != "GET") {
            return jsonError(405, "Only GET is supported");
        }
        std::size_t q = target.find('?');
        const std::string path = target.substr(0, q);
        const auto params = parseQuery(q == std::string::npos ? "" : target.substr(q + 1));

        if (path == "/api/info") {
            return info();
        }
        if (path == "/api/actors") {
            return actors(param(params, "q"));
        }
        if (path == "/api/search") {
            return search(param(params, "from"), param(params, "to"));
        }
        return staticFile(path);
    }

    int run(int port) const {
#ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            std::cout << "Could not start networking\n";
            return 1;
        }
#endif
#ifndef _WIN32
        std::signal(SIGPIPE, SIG_IGN);
#endif
        SocketHandle listener = socket(AF_INET, SOCK_STREAM, 0);
        if (!validSocket(listener)) {
            std::cout << "Could not open a socket\n";
            return 1;
        }
        int yes = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<unsigned short>(port));
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 || listen(listener, 16) != 0) {
            std::cout << "Could not listen on port " << port << " (is it already in use?)\n";
            closeSocket(listener);
            return 1;
        }

        std::cout << "Open http://localhost:" << port << " in your browser. Press Ctrl+C to stop.\n";
        while (true) {
            SocketHandle client = accept(listener, nullptr, nullptr);
            if (validSocket(client)) {
                serveClient(client);
                closeSocket(client);
            }
        }
    }

private:
    const Dataset& dataset_;
    const Graph& graph_;
    const ActorLookup& lookup_;
    std::string webDir_;
    std::string datasetName_;
    std::unordered_map<std::string, ActorIndex> indexOfId_;
    std::vector<std::string> lowerNames_;

    static std::string param(const std::unordered_map<std::string, std::string>& params, const std::string& key) {
        auto it = params.find(key);
        return it == params.end() ? "" : it->second;
    }

    std::string actorJson(ActorIndex a) const {
        const Actor& actor = dataset_.actors[a];
        std::string birth = actor.birthYear ? std::to_string(*actor.birthYear) : "null";
        const MovieIndex movie = lookup_.knownMovie(a);
        std::string known = movie < dataset_.movies.size() ? jsonText(movieText(dataset_, movie)) : "null";
        return "{\"type\": \"actor\", \"id\": " + jsonText(actor.id) + ", \"name\": " + jsonText(actor.name) +
               ", \"birthYear\": " + birth + ", \"knownFor\": " + known + "}";
    }

    HttpResponse info() const {
        std::ostringstream out;
        out << "{\"dataset\": " << jsonText(datasetName_) << ", \"actors\": " << graph_.actorCount()
            << ", \"movies\": " << graph_.movieCount() << ", \"credits\": " << graph_.creditCount() << "}";
        return {200, "application/json", out.str()};
    }

    HttpResponse actors(const std::string& query) const {
        std::size_t limit = 8;
        std::vector<ActorIndex> found;
        auto byId = indexOfId_.find(query);
        if (byId != indexOfId_.end()) {
            found.push_back(byId->second);
        }
        for (ActorIndex a : lookup_.findActor(query)) {
            if (std::find(found.begin(), found.end(), a) == found.end()) {
                found.push_back(a);
            }
        }

        const std::size_t exact = std::min<std::size_t>(found.size(), 25);
        limit = std::max(limit, exact);

        const std::string lower = toLower(query);
        if (lower.size() >= 2) {
            std::vector<ActorIndex> starts;
            std::vector<ActorIndex> contains;
            for (ActorIndex a = 0; a < lowerNames_.size() && starts.size() < limit; a++) {
                std::size_t at = lowerNames_[a].find(lower);
                if (at == std::string::npos || graph_.moviesForActor(a).empty()) {
                    continue;
                }
                if (std::find(found.begin(), found.end(), a) != found.end()) {
                    continue;
                }
                (at == 0 ? starts : contains).push_back(a);
            }
            found.insert(found.end(), starts.begin(), starts.end());
            found.insert(found.end(), contains.begin(), contains.end());
        }

        std::string body = "{\"exact\": " + std::to_string(exact) + ", \"actors\": [";
        for (std::size_t i = 0; i < found.size() && i < limit; i++) {
            body += (i ? ", " : "") + actorJson(found[i]);
        }
        return {200, "application/json", body + "]}"};
    }

    bool resolve(const std::string& input, ActorIndex& actor, std::string& error) const {
        auto byId = indexOfId_.find(input);
        if (byId != indexOfId_.end()) {
            actor = byId->second;
            return true;
        }
        std::vector<ActorIndex> matches = lookup_.findActor(input);
        if (matches.size() == 1) {
            actor = matches[0];
            return true;
        }
        error = matches.empty() ? "No actor named \"" + input + "\""
                                : "Several actors are named \"" + input + "\". Pick one from the list.";
        return false;
    }

    std::string statsJson(const std::string& name, const PathResult& result, double ms) const {
        std::ostringstream out;
        out << "{\"algorithm\": \"" << name << "\", \"degrees\": " << degreesOf(result)
            << ", \"actorsVisited\": " << result.actorcount << ", \"moviesVisited\": " << result.moviecount
            << ", \"ms\": " << ms << "}";
        return out.str();
    }

    HttpResponse search(const std::string& from, const std::string& to) const {
        if (from.empty() || to.empty()) {
            return jsonError(400, "Enter two actors");
        }
        ActorIndex src = 0;
        ActorIndex dst = 0;
        std::string error;
        if (!resolve(from, src, error) || !resolve(to, dst, error)) {
            return jsonError(404, error);
        }

        double bfsMs = 0;
        double bidirMs = 0;
        const PathResult viaBfs = runSearch(graph_, src, dst, "bfs", bfsMs);
        const PathResult viaBidir = runSearch(graph_, src, dst, "bidirectional", bidirMs);

        std::string path = "[" + actorJson(src);
        if (viaBfs.status == PathStatus::Found) {
            for (const PathStep& step : viaBfs.offset) {
                const Movie& movie = dataset_.movies[step.movie];
                std::string year = movie.year ? std::to_string(*movie.year) : "null";
                path += ", {\"type\": \"movie\", \"id\": " + jsonText(movie.id) + ", \"title\": " +
                        jsonText(movie.title) + ", \"year\": " + year + "}";
                path += ", " + actorJson(step.actor2);
            }
        }
        path += "]";

        std::string body = "{\"from\": " + actorJson(src) + ", \"to\": " + actorJson(dst) +
                           ", \"degrees\": " + std::to_string(src == dst ? 0 : degreesOf(viaBfs)) +
                           ", \"path\": " + path + ", \"stats\": [" + statsJson("BFS", viaBfs, bfsMs) +
                           ", " + statsJson("Bidirectional BFS", viaBidir, bidirMs) + "]}";
        return {200, "application/json", body};
    }

    HttpResponse staticFile(std::string path) const {
        if (path == "/") {
            path = "/index.html";
        }
        if (path.find("..") != std::string::npos) {
            return jsonError(404, "Not found");
        }
        std::ifstream in(webDir_ + path, std::ios::binary);
        if (!in) {
            return jsonError(404, "Not found");
        }
        std::ostringstream body;
        body << in.rdbuf();

        std::string type = "application/octet-stream";
        const std::string ext = path.substr(path.find_last_of('.') + 1);
        if (ext == "html") type = "text/html; charset=utf-8";
        else if (ext == "css") type = "text/css";
        else if (ext == "js") type = "text/javascript";
        else if (ext == "json") type = "application/json";
        else if (ext == "svg") type = "image/svg+xml";
        return {200, type, body.str()};
    }

    void serveClient(SocketHandle client) const {
        std::string request;
        char buffer[4096];
        while (request.find("\r\n\r\n") == std::string::npos && request.size() < 65536) {
            int n = recv(client, buffer, sizeof(buffer), 0);
            if (n <= 0) {
                return;
            }
            request.append(buffer, static_cast<std::size_t>(n));
        }

        std::istringstream line(request.substr(0, request.find("\r\n")));
        std::string method;
        std::string target;
        line >> method >> target;

        HttpResponse response;
        try {
            response = handle(method, target);
        } catch (const std::exception& e) {
            response = jsonError(500, e.what());
        }

        const char* reason = response.status == 200 ? "OK" : response.status == 404 ? "Not Found" : "Error";
        std::string head = "HTTP/1.1 " + std::to_string(response.status) + " " + reason +
                           "\r\nContent-Type: " + response.type +
                           "\r\nContent-Length: " + std::to_string(response.body.size()) +
                           "\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n";
        std::string all = head + response.body;
        std::size_t sent = 0;
        while (sent < all.size()) {
            int n = send(client, all.data() + sent, static_cast<int>(all.size() - sent), 0);
            if (n <= 0) {
                return;
            }
            sent += static_cast<std::size_t>(n);
        }
    }
};
