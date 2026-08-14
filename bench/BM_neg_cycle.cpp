#define ANKERL_NANOBENCH_IMPLEMENT
#include <fmt/format.h>
#include <nanobench.h>

#include <cstdint>
#include <digraphx_fast/csr_graph.hpp>
#include <digraphx_fast/neg_cycle.hpp>
#include <string>
#include <utility>
#include <vector>

using namespace digraphx_fast;
using std::pair;
using std::vector;

static auto build_graph(size_t n_nodes, int k = 3) -> CSRGraph<double> {
    auto builder = CSRGraph<double>::Builder(static_cast<uint32_t>(n_nodes));
    for (size_t i = 0; i < n_nodes; ++i) {
        for (int d = 1; d <= k; ++d) {
            auto j = (i + static_cast<size_t>(d)) % n_nodes;
            double w = static_cast<double>(((i + 1) * 7 + (j + 1) * 13) % 100 + 1);
            builder.add_edge(static_cast<uint32_t>(i), static_cast<uint32_t>(j), w);
        }
    }
    if (n_nodes > 2) {
        builder.add_edge(0, 1, -5.0);
        builder.add_edge(1, 2, -5.0);
        builder.add_edge(2, 0, -5.0);
    }
    return builder.build();
}

int main() {
    fmt::print("=== digraphx-fast: NegCycleFinder (Howard, CSR) ===\n");
    const size_t sizes[] = {20000, 50000, 100000, 200000, 500000, 1000000};

    // Detect negative cycle and its total weight once per size (printed before the table)
    for (auto n : sizes) {
        auto g = build_graph(n);
        NegCycleFinder finder(g);
        vector<double> dist(g.num_nodes, 0.0);
        vector<double> weights = g.weights;
        double total_weight = 0.0;
        bool found = false;
        finder.howard(
            dist, weights,
            [&](const auto& cycle) {
                found = true;
                for (auto e : cycle) total_weight += weights[e];
            },
            1);
        fmt::print("n={:<8} edges={:<9} found={:<3} weight={:.0f}\n", n, g.num_edges,
                   found ? "yes" : "no", total_weight);
    }

    ankerl::nanobench::Bench bench;
    bench.title("digraphx-fast NegCycleFinder howard (CSR) sweep")
        .unit("op")
        .warmup(1)
        .epochs(3)
        .minEpochIterations(3);

    for (auto n : sizes) {
        auto g = build_graph(n);
        vector<double> weights = g.weights;
        bench.run("howard n=" + std::to_string(n), [&] {
            vector<double> d(g.num_nodes, 0.0);
            NegCycleFinder f(g);
            auto found = f.howard(d, weights, [](const auto&) {}, 1);
            ankerl::nanobench::doNotOptimizeAway(found);
        });
    }
    return 0;
}
