#include "loader.h"

// TODO(Raami): read actors.tsv, movies.tsv and edges.tsv from `dir` as
// described in loader.h. Until then this returns an empty Dataset, which
// builds a valid Graph with no nodes.
Dataset loadDataset([[maybe_unused]] const std::string& dir) {
    return Dataset{};
}
