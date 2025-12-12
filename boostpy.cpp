#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <vector>
#include <tuple>
#include <map>
#include <omp.h>

namespace py = pybind11;


// 1. Define a custom exception for early exit
struct distance_limit_reached {};

// 2. Define the visitor
template <typename DistanceMap>
struct distance_limit_visitor : public boost::default_dijkstra_visitor {
    distance_limit_visitor(DistanceMap d, double limit) 
        : m_dist(d), m_limit(limit) {}

    template <typename Vertex, typename Graph>
    void examine_vertex(Vertex u, const Graph& g) {
        // Since Dijkstra processes vertices in increasing order of distance,
        // we can stop as soon as we see a vertex beyond our limit.
        if (m_dist[u] > m_limit) {
            throw distance_limit_reached();
        }
    }

private:
    DistanceMap m_dist;
    double m_limit;
};

double INF = std::numeric_limits<double>::infinity();

void finalize_predecessors(int* pred_row, int num_nodes) {
    for (int i = 0; i < num_nodes; ++i) {
        if (pred_row[i] == i){
            pred_row[i] = -9999;
        }
    }
}

std::tuple<py::array_t<double>, py::array_t<int>>
directed_dijkstra(const py::object &csr_matrix,
                  const std::vector<int> &sources,
                  int num_threads = 1)
{

    using namespace boost;
    typedef adjacency_list<vecS, vecS, directedS,
                           no_property,
                           property<edge_weight_t, double>> Graph;
    if (num_threads > 0)
        omp_set_num_threads(num_threads);

    // Extract CSR arrays from scipy.sparse.csr_matrix
    py::object indptr_obj = csr_matrix.attr("indptr");
    py::object indices_obj = csr_matrix.attr("indices");
    py::object data_obj = csr_matrix.attr("data");

    std::vector<int> indptr = indptr_obj.cast<std::vector<int>>();
    std::vector<int> indices = indices_obj.cast<std::vector<int>>();
    std::vector<double> data = data_obj.cast<std::vector<double>>();

    int num_nodes = indptr.size() - 1;

    Graph g(num_nodes);

    // Build graph from CSR
    #pragma omp parallel for
    for (int u = 0; u < num_nodes; ++u) {
        for (int idx = indptr[u]; idx < indptr[u + 1]; ++idx) {
            int v = indices[idx];
            double w = data[idx];
            add_edge(u, v, w, g);  // still slow for adjacency_list
        }
    }

    int num_sources = sources.size();

    // Allocate output arrays
    py::array_t<double> distances_out({num_sources, num_nodes});
    py::array_t<int> predecessors_out({num_sources, num_nodes});

    auto distances_ptr = distances_out.mutable_data();
    auto predecessors_ptr = predecessors_out.mutable_data();

    #pragma omp parallel for
    for (int si = 0; si < num_sources; ++si){
        int source = sources[si];

        // Directly point to the row in the output arrays
        double *dist_row = distances_ptr + si * num_nodes;
        int *pred_row = predecessors_ptr + si * num_nodes;

        std::vector<double> distances(num_nodes);

        dijkstra_shortest_paths(
            g, source,
            predecessor_map(pred_row)
                .distance_map(dist_row));

        finalize_predecessors(pred_row, num_nodes);
    }

    return std::make_tuple(distances_out, predecessors_out);
}





std::tuple<py::array_t<double>, py::array_t<int>>
limited_directed_dijkstra(const py::object &csr_matrix,
                       const std::vector<int> &sources,
                       double limit,
                       int num_threads = 1)
{
    using namespace boost;
    typedef adjacency_list<vecS, vecS, directedS,
                           no_property,
                           property<edge_weight_t, double>> Graph;
    if (num_threads > 0)
        omp_set_num_threads(num_threads);
    // Extract CSR arrays from scipy.sparse.csr_matrix
    py::object indptr_obj = csr_matrix.attr("indptr");
    py::object indices_obj = csr_matrix.attr("indices");
    py::object data_obj = csr_matrix.attr("data");

    std::vector<int> indptr = indptr_obj.cast<std::vector<int>>();
    std::vector<int> indices = indices_obj.cast<std::vector<int>>();
    std::vector<double> data = data_obj.cast<std::vector<double>>();

    int num_nodes = indptr.size() - 1;

    Graph g(num_nodes);

    // Build graph from CSR
    #pragma omp parallel for
    for (int u = 0; u < num_nodes; ++u) {
        for (int idx = indptr[u]; idx < indptr[u + 1]; ++idx) {
            int v = indices[idx];
            double w = data[idx];
            add_edge(u, v, w, g);  // still slow for adjacency_list
        }
    }

    int num_sources = sources.size();

    // Allocate output arrays
    py::array_t<double> distances_out({num_sources, num_nodes});
    py::array_t<int> predecessors_out({num_sources, num_nodes});

    auto distances_ptr = distances_out.mutable_data();
    auto predecessors_ptr = predecessors_out.mutable_data();



    #pragma omp parallel for
    for (int si = 0; si < num_sources; ++si){
        int source = sources[si];
        // Directly point to the row in the output arrays
        double *dist_row = distances_ptr + si * num_nodes;
        int *pred_row = predecessors_ptr + si * num_nodes;

        distance_limit_visitor<decltype(dist_row)> vis(dist_row, limit);

        try {
            dijkstra_shortest_paths(
                g,
                source,
                predecessor_map(pred_row)
                    .distance_map(dist_row)
                    .visitor(vis)
            );
        } catch (const distance_limit_reached&) {}
        // put -9999
        finalize_predecessors(pred_row, num_nodes);
       
    }

    return std::make_tuple(distances_out, predecessors_out);
}



py::array_t<double>
limited_directed_dijkstra_no_pred(const py::object &csr_matrix,
                       const std::vector<int> &sources,
                       double limit,
                       int num_threads = 1)
{
    using namespace boost;
    typedef adjacency_list<vecS, vecS, directedS,
                           no_property,
                           property<edge_weight_t, double>> Graph;
    if (num_threads > 0)
        omp_set_num_threads(num_threads);
    // Extract CSR arrays from scipy.sparse.csr_matrix
    py::object indptr_obj = csr_matrix.attr("indptr");
    py::object indices_obj = csr_matrix.attr("indices");
    py::object data_obj = csr_matrix.attr("data");

    std::vector<int> indptr = indptr_obj.cast<std::vector<int>>();
    std::vector<int> indices = indices_obj.cast<std::vector<int>>();
    std::vector<double> data = data_obj.cast<std::vector<double>>();

    int num_nodes = indptr.size() - 1;

    Graph g(num_nodes);

    // Build graph from CSR
    #pragma omp parallel for
    for (int u = 0; u < num_nodes; ++u) {
        for (int idx = indptr[u]; idx < indptr[u + 1]; ++idx) {
            int v = indices[idx];
            double w = data[idx];
            add_edge(u, v, w, g);  // still slow for adjacency_list
        }
    }

    int num_sources = sources.size();

    // Allocate output arrays
    py::array_t<double> distances_out({num_sources, num_nodes});

    auto distances_ptr = distances_out.mutable_data();



    #pragma omp parallel for
    for (int si = 0; si < num_sources; ++si){
        int source = sources[si];
        // Directly point to the row in the output arrays
        double *dist_row = distances_ptr + si * num_nodes;

        distance_limit_visitor<decltype(dist_row)> vis(dist_row, limit);

        try {
            dijkstra_shortest_paths(
                g,
                source,
                distance_map(dist_row)
                    .visitor(vis)
            );
        } catch (const distance_limit_reached&) {}       
    }

    return distances_out;
}

PYBIND11_MODULE(boostpy, m)
{
    m.doc() = "Boost Graph Library Dijkstra wrapper with multi-source support";
    m.def("directed_dijkstra", &directed_dijkstra,
          py::arg("csr_matrix"),
          py::arg("sources"),
          py::arg("num_threads") = 1);
    m.def("limited_directed_dijkstra", &limited_directed_dijkstra,
          py::arg("csr_matrix"),
          py::arg("sources"),
          py::arg("limit"),
          py::arg("num_threads") = 1);

     m.def("limited_directed_dijkstra_no_pred", &limited_directed_dijkstra_no_pred,
          py::arg("csr_matrix"),
          py::arg("sources"),
          py::arg("limit"),
          py::arg("num_threads") = 1);
}


