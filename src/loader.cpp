#include "loader.h"
#include <fstream>
#include <stdexcept>
#include <charconv>
#include <utility>
#include <filesystem>
#include <unordered_set>
#include <algorithm>




namespace {
std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> sol;
    std::string curr;
    for (char i : s) {
        if (i == '\t') {
            sol.push_back(curr);
            curr.clear();
        }
        else {
            curr+=i;
        }
    }
    sol.push_back(curr);
    return sol;
}

struct Rows {
private:
    std::string file;
    std::ifstream f;
    std::size_t n;
    std::size_t ln = 0;
public:


    bool next(std::vector<std::string>& r) {
        std::string curr;
        while (std::getline(f, curr)) {
            ln++;
            if (!curr.empty() && curr.back() == '\r')
                curr.pop_back();
            if (curr.empty())
                continue;
            r =split(curr);
            if (r.size() != n)
                fail("Expected " + std::to_string(n) + " tab-separated fields");
            return true;
        }
        if (f.bad())
            fail("Could not read file");
        return false;
    }

    std::size_t line() const {
        return ln;
    }

    [[noreturn]] void fail(const std::string& msg, std::size_t at = 0) const {
        if (at ==0)
            at = ln;
        std::string s = file;
        if (at != 0)
            s+= ":" + std::to_string(at);
        throw std::runtime_error(s + ": " + msg);
    }

    Rows(const std::filesystem::path& p, const std::vector<std::string>& h)
        : file(p.string()), f(p), n(h.size()) {
        if (!f.is_open())
            fail("File unopenable");
        std::vector<std::string> r;
        if (!next(r))
            fail("Missing header.");
        if (r[0].compare(0, 3, "\xEF\xBB\xBF") == 0)
            r[0].erase(0,3);
        if (r !=h)
            fail("Unknown TSV heading");
    }


};

template <typename T>
T getNum(const std::string& s, const Rows& in) {
    T sol = 0;
    const char* end = s.data()+s.size();
    auto curr = std::from_chars(s.data(), end, sol);
    if (curr.ptr == end && curr.ec == std::errc{})
        return sol;
    in.fail("Invalid or out-of-range number: " + s);
}



std::optional<int> getYear(const std::string& s, const Rows& in) {
    if (s.empty() || s == "\\N")
        return std::nullopt;
    return getNum<int>(s,in);
}

template <typename T>
struct Entry {
    T val;
    std::size_t idx;
    std::size_t ln;
};

template <typename T>
std::vector<T> getData(const std::filesystem::path& p, const std::vector<std::string>& h) {
    Rows in(p,h);
    std::unordered_set<std::string> seen;
    std::vector<Entry<T>> rows;
    std::vector<std::string> r;
    while (in.next(r)) {
        std::size_t idx = getNum<std::size_t>(r[0],in);
        if (r[2].empty() || r[2] == "\\N")
            in.fail("Missing name or title");
        if (r[1].empty() || r[1] == "\\N")
            in.fail("Missing record ID");
        if (seen.count(r[1]) !=0)
            in.fail("Duplicate record ID: " + r[1]);

        T curr{r[1], r[2], getYear(r[3], in)};
        rows.push_back({std::move(curr), idx, in.line()});
        seen.insert(r[1]);
    }

    std::vector<T> sol;
    sol.reserve(rows.size());
    std::sort(rows.begin(), rows.end(), [](const Entry<T>& a, const Entry<T>& b) {
        return a.idx < b.idx;
    });
    std::size_t i = 0;
    for (Entry<T>& curr : rows) {
        if (curr.idx !=i)
            in.fail("Duplicate or non-contiguous index: " + std::to_string(curr.idx), curr.ln);
        sol.push_back(std::move(curr.val));
        i++;
    }
    return sol;
}
}



Dataset loadDataset(const std::string& dir) {
    std::filesystem::path p(dir);
    Dataset sol;

    sol.movies =getData<Movie>(p / "movies.tsv", {"idx", "tconst", "title", "year"});
    sol.actors = getData<Actor>(p / "actors.tsv", {"idx", "nconst", "name", "birthYear"});

    Rows in(p / "edges.tsv", {"actorIdx", "movieIdx"});
    std::vector<std::string> r;
    while (in.next(r)) {
        MovieIndex m = getNum<MovieIndex>(r[1],in);
        ActorIndex a = getNum<ActorIndex>(r[0], in);
        if (m >= sol.movies.size())
            in.fail("Credit references an unknown movie index");
        if (a >=sol.actors.size())
            in.fail("Credit references an unknown actor index");

        sol.credits.push_back({a,m});
    }
    return sol;
}
