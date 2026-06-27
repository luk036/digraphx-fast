/**
 * @file csr_graph.hpp
 * @brief Compressed Sparse Row directed graph representation
 */

#pragma once

#include <cassert>
#include <cstdint>
#include <vector>

namespace digraphx_fast {

    /**
     * @brief Compressed Sparse Row (CSR) directed graph
     *
     * Cache-friendly adjacency storage. All outgoing edges of a node
     * are stored contiguously. Supports fast linear iteration.
     *
     * @tparam Weight Type of edge weight (default: double)
     */
    template <typename Weight = double> class CSRGraph {
      public:
        using node_t = uint32_t;
        using weight_t = Weight;

        size_t num_nodes = 0;
        size_t num_edges = 0;

        // offsets[u] .. offsets[u+1]-1 = edges of node u
        std::vector<uint32_t> offsets;
        std::vector<uint32_t> targets;
        std::vector<weight_t> weights;

        CSRGraph() = default;

        /**
         * @brief Builder pattern for construction
         *
         * Accumulates edges in adjacency list, then compact to CSR.
         */
        class Builder {
            size_t _V;
            std::vector<std::vector<std::pair<uint32_t, weight_t>>> _adj;

          public:
            explicit Builder(size_t V) : _V(V), _adj(V) {}

            void add_edge(uint32_t u, uint32_t v, weight_t w = weight_t{}) {
                assert(u < _V && v < _V);
                _adj[u].emplace_back(v, w);
            }

            auto build() -> CSRGraph {
                CSRGraph g;
                g.num_nodes = _V;
                g.offsets.resize(_V + 1, 0);

                for (size_t i = 0; i < _V; ++i)
                    g.offsets[i + 1] = static_cast<uint32_t>(_adj[i].size());
                for (size_t i = 1; i <= _V; ++i) g.offsets[i] += g.offsets[i - 1];

                g.num_edges = g.offsets[_V];
                g.targets.resize(g.num_edges);
                g.weights.resize(g.num_edges);

                auto pos = g.offsets;
                for (uint32_t u = 0; u < _V; ++u) {
                    for (auto& [v, w] : _adj[u]) {
                        auto p = pos[u]++;
                        g.targets[p] = v;
                        g.weights[p] = w;
                    }
                }
                return g;
            }
        };

        auto size() const -> size_t { return num_nodes; }
    };

}  // namespace digraphx_fast
