#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/visitors.hpp>
#include <limits>
#include <vector>
#include <tuple>
#include <map>

#include <omp.h>
namespace py = pybind11;
using namespace boost;

using Edge = std::pair<int, int>;

typedef compressed_sparse_row_graph<directedS,
                    no_property,
                    property<edge_weight_t, double>> Graph;



int get_max_vertex(const std::vector<Edge>& edges) {
    int max_v = 0;
    for (const auto &e : edges) {
        if (e.first > max_v) max_v = e.first;
        if (e.second > max_v) max_v = e.second;
    }
    return max_v + 1; // zero-indexed
}

std::tuple<py::array_t<double>, py::array_t<int>>
directed_dijkstra(const std::vector<Edge> &edges,
                  const std::vector<double> &weights,
                  const std::vector<int> &sources,
                  int num_threads = 1)
{


    if (num_threads > 0)
        omp_set_num_threads(num_threads);

    int num_nodes = get_max_vertex(edges);

    // ---- THIS CONSTRUCTOR IS STABLE ----
    Graph g(
        edges_are_unsorted_multi_pass,
        edges.begin(),
        edges.end(),
        weights.begin(),
        num_nodes
    );


    // ---- Output arrays ----
    const int num_sources = sources.size();

    py::array_t<double> distances_out({num_sources, num_nodes});
    py::array_t<int> predecessors_out({num_sources, num_nodes});

    auto distances_ptr = distances_out.mutable_data();
    auto predecessors_ptr = predecessors_out.mutable_data();
    auto index_map = get(vertex_index, g);

    #pragma omp parallel for
    for (int si = 0; si < num_sources; ++si){
        int source = sources[si];
        double *dist_row = distances_ptr + si * num_nodes;
        int *pred_row = predecessors_ptr + si * num_nodes;

        auto dist_map = make_iterator_property_map(dist_row, index_map);
        auto pred_map = make_iterator_property_map(pred_row, index_map);
        dijkstra_shortest_paths(
            g,
            source,
            predecessor_map(pred_map)
            .distance_map(dist_map)
        );
        

    }

    return {distances_out, predecessors_out};
}

template <typename DistanceMap>
class distance_cutoff_visitor : public default_dijkstra_visitor {
public:
    distance_cutoff_visitor(DistanceMap dist, double cutoff)
        : dist_map(dist), cutoff_distance(cutoff) {}

    template <typename Vertex, typename Graph>
    void examine_vertex(Vertex u, const Graph& g) const {
        if (get(dist_map, u) > cutoff_distance) {
            throw std::runtime_error("cutoff reached"); // stop Dijkstra
        }
    }

private:
    DistanceMap dist_map;
    double cutoff_distance;
};


std::tuple<py::array_t<double>, py::array_t<int>>
limited_directed_dijkstra(const std::vector<Edge> &edges,
                  const std::vector<double> &weights,
                  const std::vector<int> &sources,
                  double limit,
                  int num_threads = 1)
{


    if (num_threads > 0)
        omp_set_num_threads(num_threads);

    int num_nodes = get_max_vertex(edges);

    // ---- THIS CONSTRUCTOR IS STABLE ----
    Graph g(
        edges_are_unsorted_multi_pass,
        edges.begin(),
        edges.end(),
        weights.begin(),
        num_nodes
    );


    // ---- Output arrays ----
    const int num_sources = sources.size();

    py::array_t<double> distances_out({num_sources, num_nodes});
    py::array_t<int> predecessors_out({num_sources, num_nodes});

    auto distances_ptr = distances_out.mutable_data();
    auto predecessors_ptr = predecessors_out.mutable_data();
    auto index_map = get(vertex_index, g);

    #pragma omp parallel for
    for (int si = 0; si < num_sources; ++si){
        int source = sources[si];
        double *dist_row = distances_ptr + si * num_nodes;
        int *pred_row = predecessors_ptr + si * num_nodes;

        auto dist_map = make_iterator_property_map(dist_row, index_map);
        auto pred_map = make_iterator_property_map(pred_row, index_map);
        try {
            dijkstra_shortest_paths(
                g,
                source,
                predecessor_map(pred_map)
                .distance_map(dist_map)
                .visitor(distance_cutoff_visitor<decltype(dist_map)>(dist_map, limit))
            );
        } catch (const std::runtime_error&) {
            // cutoff reached, Dijkstra stopped early
        }
    
    }

    return {distances_out, predecessors_out};
}


PYBIND11_MODULE(fast_dijkstra, m)
{
    m.doc() = "Boost Graph Library Dijkstra wrapper with multi-source support";
    m.def("directed_dijkstra", &directed_dijkstra,
          py::arg("edges"),
          py::arg("weights"),
          py::arg("sources"),
          py::arg("num_threads")=1);
    m.def("limited_directed_dijkstra", &limited_directed_dijkstra,
          py::arg("edges"),
          py::arg("weights"),
          py::arg("sources"),
          py::arg("limit"),
          py::arg("num_threads")=1);
}


