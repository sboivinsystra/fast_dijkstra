#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <vector>
#include <queue>
#include <limits>

namespace py = pybind11;

struct CSRGraph {
    int n;
    std::vector<int> indptr;
    std::vector<int> indices;
    std::vector<double> weights;
};

const double INF = std::numeric_limits<double>::infinity();

/* ---------------- Single-source Dijkstra ---------------- */

void dijkstra_csr(
    const CSRGraph& g,
    int source,
    double* dist,
    int* pred,
    double cutoff
) {

    using Item = std::pair<double, int>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> queue;

    dist[source] = 0.0;
    queue.emplace(0.0, source);

    while (!queue.empty()) {
        auto [d, u] = queue.top();
        queue.pop();

        if (d > dist[u]) continue;
        if (d > cutoff) break;

        for (int ei = g.indptr[u]; ei < g.indptr[u + 1]; ++ei) {
            int v = g.indices[ei];
            double nd = d + g.weights[ei];

            if (nd < dist[v] && nd <= cutoff) {
                dist[v] = nd;
                pred[v] = u;
                queue.emplace(nd, v);
            }
        }
    }
}

/* ---------------- Python-exposed function ---------------- */

std::tuple<py::array_t<double>, py::array_t<int>>
multi_source_dijkstra(
    py::array_t<int> indptr,
    py::array_t<int> indices,
    py::array_t<double> weights,
    py::array_t<int> sources,
    double cutoff = std::numeric_limits<double>::infinity()
) {
    // ---- Validate shapes ----
    if (indices.size() != weights.size()) {
        throw std::runtime_error("indices and weights must have same length");
    }

    int num_nodes = indptr.size() - 1;
    int num_sources = sources.size();

    // ---- Build graph (once) ----
    CSRGraph g;
    g.n = num_nodes;
    g.indptr.assign(indptr.data(), indptr.data() + indptr.size());
    g.indices.assign(indices.data(), indices.data() + indices.size());
    g.weights.assign(weights.data(), weights.data() + weights.size());

    // ---- Allocate outputs ----
    py::array_t<double> distances_out({num_sources, num_nodes});
    py::array_t<int> predecessors_out({num_sources, num_nodes});

    auto distances_ptr = distances_out.mutable_data();
    auto predecessors_ptr = predecessors_out.mutable_data();
    for (int i = 0; i < num_sources * num_nodes; ++i) {
        distances_ptr[i] = INF;
        predecessors_ptr[i] = -9999;
    }
    const int* src_ptr = sources.data();

    // ---- Run Dijkstra for each source ----
    for (int si = 0; si < num_sources; ++si) {
        double *dist_row = distances_ptr + si * num_nodes;
        int *pred_row = predecessors_ptr + si * num_nodes;
        dijkstra_csr(
            g,
            src_ptr[si],
            dist_row,
            pred_row,
            cutoff
        );
    }

    return {distances_out, predecessors_out};
}

/* ---------------- pybind11 module ---------------- */

PYBIND11_MODULE(fast_dijkstra, m) {
    m.doc() = "Fast CSR-based multi-source Dijkstra";

    m.def("dijkstra", &multi_source_dijkstra,
        py::arg("indptr"),
        py::arg("indices"),
        py::arg("weights"),
        py::arg("sources"),
        py::arg("cutoff") = INF
    );
}
