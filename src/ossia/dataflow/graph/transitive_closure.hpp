// Copyright (C) 2001 Vladimir Prus <ghost@cs.msu.su>
// Copyright (C) 2001 Jeremy Siek <jsiek@cs.indiana.edu>
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// NOTE: this final is generated by libs/graph/doc/transitive_closure.w

#pragma once
#include <ossia/detail/config.hpp>

#include <boost/concept/assert.hpp>
#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/named_function_params.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/topological_sort.hpp>

#include <algorithm> // for std::min and std::max
#include <functional>
#include <vector>

namespace ossia
{
namespace detail
{
inline void union_successor_sets(
    const std::vector<std::size_t>& s1, const std::vector<std::size_t>& s2,
    std::vector<std::size_t>& s3)
{
  BOOST_USING_STD_MIN();
  for(std::size_t k = 0; k < s1.size(); ++k)
    s3[k] = min BOOST_PREVENT_MACRO_SUBSTITUTION(s1[k], s2[k]);
}

template <
    typename TheContainer, typename ST = std::size_t,
    typename VT = typename TheContainer::value_type>
struct subscript_t
{
  typedef VT& result_type;

  subscript_t(TheContainer& c)
      : container(&c)
  {
  }
  VT& operator()(const ST& i) const
  {
    return (*container)[i];
  }

protected:
  TheContainer* container;
};
template <typename TheContainer>
subscript_t<TheContainer> subscript(TheContainer& c)
{
  return subscript_t<TheContainer>(c);
}
} // namespace detail

template <typename Graph, typename GraphTC>
struct TransitiveClosureState
{
  using VertexIndexMap =
      typename boost::property_map<Graph, boost::vertex_index_t>::const_type;

  using vertex = typename boost::graph_traits<Graph>::vertex_descriptor;
  using size_type = typename boost::property_traits<VertexIndexMap>::value_type;
  using tc_vertex = typename boost::graph_traits<GraphTC>::vertex_descriptor;
  using cg_vertex = size_type;
  using CG_t = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS>;

  std::vector<tc_vertex> to_tc_vec;

  std::vector<cg_vertex> component_number_vec;
  std::vector<std::vector<vertex>> components;

  CG_t CG;

  std::vector<cg_vertex> adj;

  std::vector<cg_vertex> topo_order;
  std::vector<cg_vertex> topo_number;

  std::vector<std::vector<cg_vertex>> CG_vec;
  std::vector<std::vector<cg_vertex>> chains;
  std::vector<cg_vertex> in_a_chain;

  std::vector<size_type> chain_number;
  std::vector<size_type> pos_in_chain;

  std::vector<std::vector<cg_vertex>> successors;
};

template <typename T>
void resize_and_fill(T& vec, const std::size_t N)
{
  vec.clear();
  vec.resize(N);
}
template <typename T>
void resize_and_fill(std::vector<std::vector<T>>& vec, const std::size_t N)
{
  const auto cur = vec.size();
  vec.resize(N);
  for(std::size_t i = 0; i < std::min(cur, N); i++)
  {
    vec[i].clear();
  }
}

template <typename T>
void resize_and_fill(
    std::vector<std::vector<T>>& vec, const std::size_t N, const std::size_t init_num,
    T init_val)
{
  vec.resize(N);
  for(std::size_t i = 0; i < N; i++)
  {
    auto cur = vec[i].size();
    vec[i].resize(init_num, init_val);
    std::fill(vec[i].begin(), vec[i].begin() + std::min(cur, init_num), init_val);
  }
}

template <
    typename Graph, typename GraphTC, typename G_to_TC_VertexMap,
    typename VertexIndexMap>
void transitive_closure(
    const Graph& g, GraphTC& tc, G_to_TC_VertexMap g_to_tc_map, VertexIndexMap index_map,
    TransitiveClosureState<Graph, GraphTC>& state)
{
  using namespace boost;
  if(num_vertices(g) == 0)
    return;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;
  typedef typename property_traits<VertexIndexMap>::value_type size_type;

  BOOST_CONCEPT_ASSERT((VertexListGraphConcept<Graph>));
  BOOST_CONCEPT_ASSERT((AdjacencyGraphConcept<Graph>));
  BOOST_CONCEPT_ASSERT((VertexMutableGraphConcept<GraphTC>));
  BOOST_CONCEPT_ASSERT((EdgeMutableGraphConcept<GraphTC>));
  BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept<VertexIndexMap, vertex>));

  typedef size_type cg_vertex;
  const auto n_vtx = num_vertices(g);
  auto& component_number_vec = state.component_number_vec;
  resize_and_fill(component_number_vec, n_vtx);
  iterator_property_map<cg_vertex*, VertexIndexMap, cg_vertex, cg_vertex&>
      component_number(&component_number_vec[0], index_map);

  const auto num_scc
      = strong_components(g, component_number, vertex_index_map(index_map));

  auto& components = state.components;
  resize_and_fill(components, num_scc);
  build_component_lists(g, num_scc, component_number, components);

  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS> CG_t;
  auto& CG = state.CG;
  CG = CG_t{num_scc}; // todo reuse memory/

  for(cg_vertex s = 0; s < components.size(); ++s)
  {
    auto& adj = state.adj;
    adj.clear();
    for(size_type i = 0; i < components[s].size(); ++i)
    {
      const auto u = components[s][i];
      for(auto [v, v_end] = adjacent_vertices(u, g); v != v_end; ++v)
      {
        const auto t = component_number[*v];
        if(s != t) // Avoid loops in the condensation graph
          adj.push_back(t);
      }
    }

    ossia::remove_duplicates(adj);
    for(auto i = adj.begin(); i != adj.end(); ++i)
    {
      add_edge(s, *i, CG);
    }
  }

  assert(num_scc == num_vertices(CG));
  auto& topo_order = state.topo_order;
  topo_order.clear();
  topo_order.reserve(num_scc);
  auto& topo_number = state.topo_number;
  topo_number.resize(num_scc);
  topological_sort(
      CG, std::back_inserter(topo_order), vertex_index_map(identity_property_map()));
  std::reverse(topo_order.begin(), topo_order.end());
  size_type n = 0;
  for(auto iter = topo_order.begin(); iter != topo_order.end(); ++iter)
    topo_number[*iter] = n++;

  auto& CG_vec = state.CG_vec;
  CG_vec.resize(num_scc);
  for(size_type i = 0; i < num_scc; ++i)
  {
    auto pr = adjacent_vertices(i, CG);
    CG_vec[i].assign(pr.first, pr.second);
    std::sort(CG_vec[i].begin(), CG_vec[i].end(), [&](cg_vertex lhs, cg_vertex rhs) {
      return topo_number[lhs] < topo_number[rhs];
    });
  }

  auto& chains = state.chains;
  chains.clear();
  chains.reserve(std::log2(n_vtx));
  {
    auto& in_a_chain = state.in_a_chain;
    resize_and_fill(in_a_chain, num_scc);
    for(cg_vertex v : topo_order)
    {
      if(!in_a_chain[v])
      {
        chains.resize(chains.size() + 1);
        auto& chain = chains.back();
        for(;;)
        {
          chain.push_back(v);
          in_a_chain[v] = true;
          auto next = std::find_if(CG_vec[v].begin(), CG_vec[v].end(), [&](auto elt) {
            return !in_a_chain[elt];
          });
          if(next != CG_vec[v].end())
            v = *next;
          else
            break; // end of chain, dead-end
        }
      }
    }
  }
  auto& chain_number = state.chain_number;
  chain_number.resize(num_scc); // resize_and_fill(chain_number, num_scc);
  auto& pos_in_chain = state.pos_in_chain;
  pos_in_chain.resize(num_scc); // resize_and_fill(pos_in_chain, num_scc);
  for(size_type i = 0; i < chains.size(); ++i)
    for(size_type j = 0; j < chains[i].size(); ++j)
    {
      const cg_vertex v = chains[i][j];
      chain_number[v] = i;
      pos_in_chain[v] = j;
    }

  static const constexpr cg_vertex inf = std::numeric_limits<cg_vertex>::max();
  auto& successors = state.successors;
  resize_and_fill(successors, num_scc, chains.size(), inf);
  for(auto i = topo_order.rbegin(); i != topo_order.rend(); ++i)
  {
    const cg_vertex u = *i;
    for(auto adj = CG_vec[u].begin(), adj_last = CG_vec[u].end(); adj != adj_last; ++adj)
    {
      const cg_vertex v = *adj;
      auto& suc_u = successors[u];
      auto& suc_v = successors[v];
      auto& suc_u_v = suc_u[chain_number[v]];
      auto& topo_v = topo_number[v];
      if(topo_v < suc_u_v)
      {
        // Succ(u) = Succ(u) U Succ(v)
        detail::union_successor_sets(suc_u, suc_v, suc_u);
        // Succ(u) = Succ(u) U {v}
        suc_u_v = topo_v;
      }
    }
  }

  for(size_type i = 0; i < CG_vec.size(); ++i)
    CG_vec[i].clear();
  for(size_type i = 0; i < CG_vec.size(); ++i)
    for(size_type j = 0; j < chains.size(); ++j)
    {
      const size_type topo_num = successors[i][j];
      if(topo_num < inf)
      {
        cg_vertex v = topo_order[topo_num];
        const auto chain_j_size = chains[j].size();
        const auto pos_v = pos_in_chain[v];
        if(chain_j_size > pos_v)
        {
          CG_vec[i].reserve(CG_vec[i].size() + chain_j_size - pos_v);
          for(size_type k = pos_v; k < chain_j_size; ++k)
            CG_vec[i].push_back(chains[j][k]);
        }
      }
    }

  // Add vertices to the transitive closure graph
  {
    vertex_iterator i, i_end;
    for(boost::tie(i, i_end) = vertices(g); i != i_end; ++i)
      g_to_tc_map[*i] = add_vertex(tc);
  }
  // Add edges between all the vertices in two adjacent SCCs
  for(auto si = CG_vec.begin(), si_end = CG_vec.end(); si != si_end; ++si)
  {
    cg_vertex s = si - CG_vec.begin();
    for(auto i = CG_vec[s].begin(), i_end = CG_vec[s].end(); i != i_end; ++i)
    {
      cg_vertex t = *i;
      for(size_type k = 0; k < components[s].size(); ++k)
        for(size_type l = 0; l < components[t].size(); ++l)
          add_edge(g_to_tc_map[components[s][k]], g_to_tc_map[components[t][l]], tc);
    }
  }
  // Add edges connecting all vertices in a SCC
  for(size_type i = 0; i < components.size(); ++i)
    if(components[i].size() > 1)
      for(size_type k = 0; k < components[i].size(); ++k)
        for(size_type l = 0; l < components[i].size(); ++l)
        {
          vertex u = components[i][k], v = components[i][l];
          add_edge(g_to_tc_map[u], g_to_tc_map[v], tc);
        }

  // Find loopbacks in the original graph.
  // Need to add it to transitive closure.
  {
    for(auto [i, i_end] = vertices(g); i != i_end; ++i)
    {
      for(auto [ab, ae] = adjacent_vertices(*i, g); ab != ae; ++ab)
      {
        if(*ab == *i)
          if(components[component_number[*i]].size() == 1)
            add_edge(g_to_tc_map[*i], g_to_tc_map[*i], tc);
      }
    }
  }
}

template <typename Graph, typename GraphTC>
void transitive_closure(
    const Graph& g, GraphTC& tc, TransitiveClosureState<Graph, GraphTC>& tc_state)
{
  using namespace boost;
  if(num_vertices(g) == 0)
    return;
  typedef typename property_map<Graph, vertex_index_t>::const_type VertexIndexMap;
  VertexIndexMap index_map = get(vertex_index, g);

  typedef typename graph_traits<GraphTC>::vertex_descriptor tc_vertex;

  resize_and_fill(tc_state.to_tc_vec, num_vertices(g));
  iterator_property_map<tc_vertex*, VertexIndexMap, tc_vertex, tc_vertex&> g_to_tc_map(
      &tc_state.to_tc_vec[0], index_map);

  transitive_closure(g, tc, g_to_tc_map, index_map, tc_state);
}
}
