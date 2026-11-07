#include "benchmark.h"

// TODO(Daniel): time bfs() and bidirectionalBfs() on each pair (for example
// with std::chrono::steady_clock) and fill one BenchmarkRow per run.
// Placeholder: no rows.
std::vector<BenchmarkRow> runBenchmark([[maybe_unused]] const Graph& graph,
                                       [[maybe_unused]] const std::string& datasetName,
                                       [[maybe_unused]] const std::vector<ActorPair>& pairs) {
    return {};
}

// TODO(Daniel): write the header and one line per row to `path`.
// Placeholder: writes nothing and reports failure.
bool writeResultsCsv([[maybe_unused]] const std::string& path,
                     [[maybe_unused]] const std::vector<BenchmarkRow>& rows) {
    return false;
}
