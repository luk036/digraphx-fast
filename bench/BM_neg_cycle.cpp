#include <chrono>
#include <cstdint>
#include <cstdio>
#include <digraphx_fast/csr_graph.hpp>
#include <digraphx_fast/neg_cycle.hpp>
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
    std::printf("=== digraphx-fast: NegCycleFinder (Howard, CSR) ===\n");
    std::printf("%-12s %-10s %-6s %-8s %-12s %-8s\n", "Nodes", "Edges", "Found", "Weight",
                "Avg(ms)", "Rel");
    const size_t sizes[] = {20000, 50000, 100000, 200000, 500000, 1000000};
    const int n_runs = 5;
    double ref_ms = 0.0;
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
        double total_ms = 0.0;
        for (int run = 0; run < n_runs; ++run) {
            vector<double> d(g.num_nodes, 0.0);
            NegCycleFinder f2(g);
            auto start = std::chrono::high_resolution_clock::now();
            f2.howard(d, weights, [](const auto&) {}, 1);
            auto end = std::chrono::high_resolution_clock::now();
            total_ms += std::chrono::duration<double, std::milli>(end - start).count();
        }
        double avg = total_ms / n_runs;
        if (ref_ms == 0.0) ref_ms = avg;
        std::printf("%-12zu %-10zu %-6s %-8.0f %-12.2f %-8.1f\n", n, g.num_edges,
                    found ? "yes" : "no", total_weight, avg, avg / ref_ms);
    }
    return 0;
}
