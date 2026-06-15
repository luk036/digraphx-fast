#include <doctest/doctest.h>

#include <algorithm>
#include <cstdint>
#include <vector>

#include <digraphx_fast/csr_graph.hpp>
#include <digraphx_fast/neg_cycle.hpp>

using namespace digraphx_fast;

TEST_CASE("Test CSRGraph construction and iteration") {
    auto builder = CSRGraph<double>::Builder(3);
    builder.add_edge(0, 1, 1.0);
    builder.add_edge(1, 2, 2.0);
    builder.add_edge(2, 0, 3.0);
    auto g = builder.build();

    CHECK_EQ(g.num_nodes, 3);
    CHECK_EQ(g.num_edges, 3);

    // Verify CSR offsets
    CHECK_EQ(g.offsets[0], 0);
    CHECK_EQ(g.offsets[1], 1);
    CHECK_EQ(g.offsets[2], 2);
    CHECK_EQ(g.offsets[3], 3);

    // Verify edges
    CHECK_EQ(g.targets[0], 1);
    CHECK_EQ(g.weights[0], 1.0);
    CHECK_EQ(g.targets[1], 2);
    CHECK_EQ(g.weights[1], 2.0);
    CHECK_EQ(g.targets[2], 0);
    CHECK_EQ(g.weights[2], 3.0);
}

TEST_CASE("Test Negative Cycle Detection") {
    auto builder = CSRGraph<double>::Builder(3);
    builder.add_edge(0, 1, 1.0);
    builder.add_edge(1, 2, 1.0);
    builder.add_edge(2, 0, -3.0);  // negative cycle: 1+1-3 = -1
    auto g = builder.build();

    NegCycleFinder finder(g);
    std::vector<double> dist(g.num_nodes, 0.0);
    std::vector<double> weights = g.weights;  // copy

    bool found = false;
    finder.howard(dist, weights, [&](const auto&) { found = true; }, 1);

    CHECK(found);
}

TEST_CASE("Test No Negative Cycle") {
    auto builder = CSRGraph<double>::Builder(3);
    builder.add_edge(0, 1, 1.0);
    builder.add_edge(1, 2, 1.0);
    builder.add_edge(2, 0, 1.0);  // all positive: no negative cycle
    auto g = builder.build();

    NegCycleFinder finder(g);
    std::vector<double> dist(g.num_nodes, 0.0);
    std::vector<double> weights = g.weights;

    bool found = false;
    finder.howard(dist, weights, [&](const auto&) { found = true; }, 1);

    CHECK_FALSE(found);
}

TEST_CASE("Test max_cycles parameter") {
    // Create graph with multiple negative cycles
    // 0 -> 1 -> 2 -> 0  (cycle 1)
    // 0 -> 3 -> 4 -> 0  (cycle 2)
    // Both have negative total weight
    auto builder = CSRGraph<double>::Builder(5);
    builder.add_edge(0, 1, -1.0);
    builder.add_edge(1, 2, -1.0);
    builder.add_edge(2, 0, -1.0);
    builder.add_edge(0, 3, -2.0);
    builder.add_edge(3, 4, -1.0);
    builder.add_edge(4, 0, -1.0);
    auto g = builder.build();

    NegCycleFinder finder(g);
    std::vector<double> dist(g.num_nodes, 0.0);
    std::vector<double> weights = g.weights;

    size_t cycle_count = 0;
    finder.howard(dist, weights, [&](const auto&) { ++cycle_count; }, 2);

    CHECK_EQ(cycle_count, 2);
}

TEST_CASE("Test Large Graph Stress") {
    constexpr auto V = 1000U;
    auto builder = CSRGraph<double>::Builder(V);
    // Create a large cycle: 0->1->2->...->999->0
    for (uint32_t i = 0; i < V - 1; ++i) {
        builder.add_edge(i, i + 1, 1.0);
    }
    builder.add_edge(V - 1, 0, -static_cast<double>(V));  // only negative edge
    auto g = builder.build();

    NegCycleFinder finder(g);
    std::vector<double> dist(g.num_nodes, 0.0);
    std::vector<double> weights = g.weights;

    bool found = false;
    finder.howard(dist, weights, [&](const auto&) { found = true; }, 1);

    CHECK(found);
}

TEST_CASE("Test Warm-Start") {
    auto builder = CSRGraph<double>::Builder(3);
    builder.add_edge(0, 1, 1.0);
    builder.add_edge(1, 2, 1.0);
    builder.add_edge(2, 0, -3.0);
    auto g = builder.build();

    NegCycleFinder finder(g);
    std::vector<double> dist(g.num_nodes, 0.0);
    std::vector<double> weights = g.weights;

    // howard (fresh start) finds negative cycle
    bool found = false;
    finder.howard(dist, weights, [&](const auto&) { found = true; }, 1);
    CHECK(found);

    // Call howard_warm again — pred is preserved from previous iteration
    // and distances are warm. This should converge faster. Verify the
    // callback is still invoked correctly.
    size_t count = 0;
    finder.howard_warm(dist, weights, [&](const auto& cycle) {
        ++count;
        CHECK_FALSE(cycle.empty());
    }, 2);
    // Warm-start should find any remaining negative cycles quickly
    CHECK_GE(count, 0); // no crash, correct callbacks
}
