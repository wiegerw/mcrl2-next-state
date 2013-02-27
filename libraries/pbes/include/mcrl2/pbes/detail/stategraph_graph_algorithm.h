// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/pbes/detail/stategraph_graph_algorithm.h
/// \brief add your file description here.

#ifndef MCRL2_PBES_DETAIL_STATEGRAPH_GRAPH_ALGORITHM_H
#define MCRL2_PBES_DETAIL_STATEGRAPH_GRAPH_ALGORITHM_H

#include <sstream>
#include "mcrl2/pbes/detail/stategraph_graph.h"

namespace mcrl2 {

namespace pbes_system {

namespace detail {

//struct local_vertex;
//
//struct local_edge
//{
//  local_vertex* source;
//  local_vertex* target;
//
//  local_edge(local_vertex* source_, local_vertex* target_)
//   : source(source_),
//     target(target_)
//   {}
//
//  bool operator<(const local_edge& other) const
//  {
//    if (source != other.source)
//    {
//      return source < other.source;
//    }
//    return target < other.target;
//  }
//};

struct local_vertex
{
  core::identifier_string X;
  std::size_t p;
  std::set<local_vertex*> outgoing_edges;

  local_vertex(const core::identifier_string& X_, std::size_t p_)
    : X(X_), p(p_)
  {}

  std::string print() const
  {
    std::ostringstream out;
    out << "(" << std::string(X) << ", " << p << ")";
    return out.str();
  }
};

struct local_graph
{
  typedef std::map<std::size_t, local_vertex*> vertex_map;

  std::vector<local_vertex> m_vertices;
  std::map<core::identifier_string, vertex_map> m_graph;

  // @pre: (X, p) is in the graph
  local_vertex& find_vertex(const core::identifier_string& X, std::size_t p)
  {
    vertex_map& m = m_graph[X];
    vertex_map::iterator i = m.find(p);
    if (i == m.end())
    {
      std::cout << "(X, p) = (" << std::string(X) << ", " << p << ")" << std::endl;
    }
    assert (i != m.end());
    return *(i->second);
  }

  // @pre: (X, p) is not in the graph
  local_vertex& insert_vertex(const local_vertex& v)
  {
    m_vertices.push_back(v);
    m_graph[v.X][v.p] = &m_vertices.back();
    return m_vertices.back();
  }

  // insert edge between (X, i) and (Y, j)
  void insert_edge(const core::identifier_string& X, std::size_t i, const core::identifier_string& Y, std::size_t j)
  {
    mCRL2log(log::debug, "stategraph") << "insert edge [must] (" << std::string(X) << ", " << i << ") (" << std::string(Y) << ", " << j << ")" << std::endl;
    local_vertex& u = find_vertex(X, i);
    local_vertex& v = find_vertex(Y, j);
    u.outgoing_edges.insert(&v);
  }

  std::string print()
  {
    std::ostringstream out;
    for (std::vector<local_vertex>::const_iterator i = m_vertices.begin(); i != m_vertices.end(); ++i)
    {
      const local_vertex& u = *i;
      out << u.print() << " connected with";
      for (std::set<local_vertex*>::const_iterator j = u.outgoing_edges.begin(); j != u.outgoing_edges.end(); ++j)
      {
        const local_vertex& v = **j;
        out << " " << v.print();
      }
      out << std::endl;
    }
    return out.str();
  }
};

/// \brief Algorithm class for the computation of the stategraph graph
class stategraph_graph_algorithm
{
  public:
    // simplify and rewrite the expression x
    pbes_expression simplify(const pbes_expression& x) const
    {
      data::detail::simplify_rewriter r;
      stategraph_simplifying_rewriter<pbes_expression, data::detail::simplify_rewriter> R(r);
      return R(x);
    }

    // simplify and rewrite the guards of the pbes p
    void simplify(stategraph_pbes& p) const
    {
      std::vector<stategraph_equation>& equations = p.equations();
      for (std::size_t k = 0; k < equations.size(); k++)
      {
        stategraph_equation& eqn = equations[k];
        predicate_variable_vector& predvars = eqn.predicate_variables();
        for (std::size_t i = 0; i < predvars.size(); i++)
        {
          std::pair<propositional_variable_instantiation, pbes_expression>& pvar = predvars[i];
          pvar.second = simplify(pvar.second);
        }
      }
    }

    typedef control_flow_graph::vertex_iterator vertex_iterator;

  protected:
    // the pbes that is considered
    stategraph_pbes m_pbes;

    // a data rewriter
    data::rewriter m_datar;

    // the control flow parameters
    std::map<core::identifier_string, std::vector<bool> > m_is_control_flow;

    propositional_variable find_propvar(const pbes<>& p, const core::identifier_string& X) const
    {
      const std::vector<pbes_equation>& equations = p.equations();
      for (std::vector<pbes_equation>::const_iterator i = equations.begin(); i != equations.end(); ++i)
      {
        if (i->variable().name() == X)
        {
          return i->variable();
        }
      }
      throw mcrl2::runtime_error("find_propvar failed!");
      return propositional_variable();
    }

    std::string print_control_flow_parameters()
    {
      std::ostringstream out;
      out << "--- control flow parameters ---" << std::endl;
      const std::vector<stategraph_equation>& equations = m_pbes.equations();
      for (std::vector<stategraph_equation>::const_iterator k = equations.begin(); k != equations.end(); ++k)
      {
        propositional_variable X = k->variable();
        const std::vector<data::variable>& d_X = k->parameters();
        const std::vector<bool>& cf = m_is_control_flow[X.name()];

        out << core::pp(X.name()) << " ";
        for (std::size_t i = 0; i < cf.size(); ++i)
        {
          if (cf[i])
          {
            out << data::pp(d_X[i]) << " ";
          }
        }
        out << std::endl;
      }
      return out.str();
    }

    std::string print_propvar_parameter(const core::identifier_string& X, std::size_t index) const
    {
      return "(" + core::pp(X) + ", " + data::pp(find_equation(m_pbes, X)->parameters()[index]) + ")";
    }

    std::string print_stategraph_assignment(bool stategraph_value,
                                              std::size_t index,
                                              const pbes_system::propositional_variable& X,
                                              const pbes_system::propositional_variable_instantiation& Y,
                                              const std::string& message,
                                              const data::variable& previous_value = data::variable()
                                             ) const
    {
      std::ostringstream out;
      out << message << ": " << print_propvar_parameter(Y.name(), index) << " -> " << std::boolalpha << stategraph_value;
      out << " because of equation " << core::pp(X.name());
      data::variable_list v = X.parameters();
      if (v.size() > 0)
      {
        out << "(";
        for (data::variable_list::iterator i = v.begin(); i != v.end(); ++i)
        {
          if (i != v.begin())
          {
            out << ", ";
          }
          out << core::pp(i->name());
        }
        out << ")";
      }
      out << " = ... " << pbes_system::pp(Y) << " index = " << index << " " << data::pp(previous_value) << std::endl;
      return out.str();
    }

    void compute_control_flow_parameters()
    {
      const std::vector<stategraph_equation>& equations = m_pbes.equations();
      std::map<core::identifier_string, std::vector<data::variable> > V;

      // initialize all control flow parameters to true
      // initalize V_km to the empty set
      for (std::vector<stategraph_equation>::const_iterator k = equations.begin(); k != equations.end(); ++k)
      {
        propositional_variable X = k->variable();
        const std::vector<data::variable>& d_X = k->parameters();
        m_is_control_flow[X.name()] = std::vector<bool>(d_X.size(), true);
        V[X.name()] = std::vector<data::variable>(d_X.size(), data::variable());
      }

      // pass 1
      for (std::vector<stategraph_equation>::const_iterator k = equations.begin(); k != equations.end(); ++k)
      {
        propositional_variable X = k->variable();
        const std::vector<data::variable>& d_X = k->parameters();
        const predicate_variable_vector& predvars = k->predicate_variables();
        for (predicate_variable_vector::const_iterator i = predvars.begin(); i != predvars.end(); ++i)
        {
          const propositional_variable_instantiation& Y = i->first;
          data::data_expression_list e = Y.parameters();
          std::size_t index = 0;
          for (data::data_expression_list::const_iterator q = e.begin(); q != e.end(); ++q, ++index)
          {
            if (data::is_variable(*q))
            {
              std::vector<data::variable>::const_iterator found = std::find(d_X.begin(), d_X.end(), *q);
              if (found != d_X.end())
              {
                if (V[Y.name()][index] == data::variable())
                {
                  V[Y.name()][index] = *q;
                  mCRL2log(log::debug, "stategraph") << print_stategraph_assignment(true, index, X, Y, "pass 1");
                }
                else
                {
                  bool is_same_value = (V[Y.name()][index] == *q);
                  m_is_control_flow[Y.name()][index] = is_same_value;
                  mCRL2log(log::debug, "stategraph") << print_stategraph_assignment(is_same_value, index, X, Y, "pass 1", V[Y.name()][index]);
                }
              }
            }
          }
        }
      }

      // pass 2
      std::set<core::identifier_string> todo;
      for (std::vector<stategraph_equation>::const_iterator k = equations.begin(); k != equations.end(); ++k)
      {
        todo.insert(k->variable().name());
      }

      while (!todo.empty())
      {
        core::identifier_string name = *todo.begin();
        todo.erase(todo.begin());
        const stategraph_equation& eqn = *find_equation(m_pbes, name);
        propositional_variable X = eqn.variable();
        const std::vector<data::variable>& d_X = eqn.parameters();
        const predicate_variable_vector& predvars = eqn.predicate_variables();
        for (predicate_variable_vector::const_iterator i = predvars.begin(); i != predvars.end(); ++i)
        {
          const propositional_variable_instantiation& Y = i->first;
          data::data_expression_list e = Y.parameters();
          std::size_t index = 0;
          for (data::data_expression_list::const_iterator q = e.begin(); q != e.end(); ++q, ++index)
          {
            if (is_constant(*q))
            {
              continue;
            }
            else if (data::is_variable(*q))
            {
              std::vector<data::variable>::const_iterator found = std::find(d_X.begin(), d_X.end(), *q);
              if (found == d_X.end())
              {
                if (m_is_control_flow[Y.name()][index] != false)
                {
                  m_is_control_flow[Y.name()][index] = false;
                  todo.insert(Y.name());
                  mCRL2log(log::debug, "stategraph") << print_stategraph_assignment(false, index, X, Y, "pass 2");
                }
              }
              else
              {
                if (X.name() == Y.name() && (found != d_X.begin() + index))
                {
                  if (m_is_control_flow[Y.name()][index] != false)
                  {
                    m_is_control_flow[Y.name()][index] = false;
                    todo.insert(Y.name());
                    mCRL2log(log::debug, "stategraph") << print_stategraph_assignment(false, index, X, Y, "pass 2");
                  }
                }
                else
                {
                  if (!m_is_control_flow[X.name()][found - d_X.begin()])
                  {
                    if (m_is_control_flow[Y.name()][index] != false)
                    {
                      m_is_control_flow[Y.name()][index] = false;
                      todo.insert(Y.name());
                      mCRL2log(log::debug, "stategraph") << print_stategraph_assignment(false, index, X, Y, "pass 2");
                    }
                  }
                }
              }
            }
            else
            {
              if (m_is_control_flow[Y.name()][index] != false)
              {
                m_is_control_flow[Y.name()][index] = false;
                todo.insert(Y.name());
                mCRL2log(log::debug, "stategraph") << print_stategraph_assignment(false, index, X, Y, "pass 2");
              }
            }
          }
        }
      }
      m_pbes.compute_source_dest_copy(m_is_control_flow);
      mCRL2log(log::debug) << print_control_flow_parameters();
    }

    const std::vector<bool>& stategraph_values(const core::identifier_string& X) const
    {
      std::map<core::identifier_string, std::vector<bool> >::const_iterator i = m_is_control_flow.find(X);
      assert (i != m_is_control_flow.end());
      return i->second;
    }

    // returns the control flow parameters of the propositional variable with name X
    std::set<data::variable> control_flow_parameters(const core::identifier_string& X) const
    {
      std::set<data::variable> result;
      const std::vector<bool>& b = stategraph_values(X);
      const stategraph_equation& eqn = *find_equation(m_pbes, X);
      const std::vector<data::variable>& d = eqn.parameters();
      std::size_t index = 0;
      for (std::vector<data::variable>::const_iterator i = d.begin(); i != d.end(); ++i, index++)
      {
        if (b[index])
        {
          result.insert(*i);
        }
      }
      return result;
    }

    // returns true if the i-th parameter of X is a control flow parameter
    bool is_control_flow_parameter(const core::identifier_string& X, std::size_t i) const
    {
      return stategraph_values(X)[i];
    }

    // returns the parameters of the propositional variable with name X
    std::set<data::variable> propvar_parameters(const core::identifier_string& X) const
    {
      const stategraph_equation& eqn = *find_equation(m_pbes, X);
      const std::vector<data::variable>& d = eqn.parameters();
      return std::set<data::variable>(d.begin(), d.end());
    }

    // removes parameter values that do not correspond to a control flow parameter
    propositional_variable_instantiation project(const propositional_variable_instantiation& x) const
    {
      core::identifier_string X = x.name();
      data::data_expression_list d_X = x.parameters();
      const std::vector<bool>& b = stategraph_values(X);
      std::size_t index = 0;
      std::vector<data::data_expression> d;
      for (data::data_expression_list::iterator i = d_X.begin(); i != d_X.end(); ++i, index++)
      {
        if (b[index])
        {
          d.push_back(*i);
        }
      }
      return propositional_variable_instantiation(X, data::data_expression_list(d.begin(), d.end()));
    }

    // removes parameter values that do not correspond to a control flow parameter
    propositional_variable project_variable(const propositional_variable& x) const
    {
      core::identifier_string X = x.name();
      data::variable_list d_X = x.parameters();
      const std::vector<bool>& b = stategraph_values(X);
      std::size_t index = 0;
      std::vector<data::variable> d;
      for (data::variable_list::iterator i = d_X.begin(); i != d_X.end(); ++i, index++)
      {
        if (b[index])
        {
          d.push_back(*i);
        }
      }
      return propositional_variable(X, data::variable_list(d.begin(), d.end()));
    }

    template <typename Substitution>
    propositional_variable_instantiation apply_substitution(const propositional_variable_instantiation& X, Substitution sigma) const
    {
      return propositional_variable_instantiation(X.name(), data::replace_free_variables(X.parameters(), sigma));
    }

  public:

    /// \brief Computes the control flow graph
    void run(const pbes<>& p)
    {
      m_pbes = stategraph_pbes(p);
      m_datar = data::rewriter(p.data());
      simplify(m_pbes);

      stategraph_influence_graph_algorithm ialgo(m_pbes);
      ialgo.run();

      compute_control_flow_parameters();

      mCRL2log(log::debug, "stategraph") << "--- source, dest, copy ---\n" << m_pbes.print_source_dest_copy() << std::endl;
    }
};

/// \brief Algorithm class for the global variant of the stategraph algorithm
class stategraph_graph_global_algorithm: public stategraph_graph_algorithm
{
  public:
    typedef stategraph_graph_algorithm super;

  protected:
    // the control flow graph
    control_flow_graph m_control_flow_graph;

    void compute_control_flow_graph()
    {
      mCRL2log(log::debug, "stategraph") << "=== compute control flow graph ===" << std::endl;

      data::rewriter datar(m_pbes.data());
      pbes_system::simplifying_rewriter<pbes_expression, data::rewriter> pbesr(datar);

      std::set<stategraph_vertex*> todo;

      // handle the initial state
      propositional_variable_instantiation u0 = project(m_pbes.initial_state());
      vertex_iterator i = m_control_flow_graph.insert_vertex(u0);
      todo.insert(&(i->second));
      mCRL2log(log::debug, "stategraph") << "u0 = " << pbes_system::pp(u0) << std::endl;

      while (!todo.empty())
      {
        std::set<stategraph_vertex*>::iterator i = todo.begin();
        todo.erase(i);
        stategraph_vertex& u = **i;
        stategraph_vertex* source = &u;
        mCRL2log(log::debug, "stategraph") << "selected todo element u = " << pbes_system::pp(u.X) << std::endl;

        const stategraph_equation& eqn = *find_equation(m_pbes, u.X.name());
        mCRL2log(log::debug, "stategraph") << "eqn = " << eqn.print() << std::endl;
        propositional_variable X = project_variable(eqn.variable());
        data::variable_list d = X.parameters();
        data::data_expression_list e = u.X.parameters();
        data::sequence_sequence_substitution<data::variable_list, data::data_expression_list> sigma(d, e);

        const predicate_variable_vector& predvars = eqn.predicate_variables();
        if (eqn.is_simple())
        {
          mCRL2log(log::debug, "stategraph") << "insert guard " << pbes_system::pp(eqn.simple_guard()) << " in vertex u" << std::endl;
          u.guards.insert(eqn.simple_guard());
        }
        for (predicate_variable_vector::const_iterator j = predvars.begin(); j != predvars.end(); ++j)
        {
          mCRL2log(log::debug, "stategraph") << "Y(e) = " << pbes_system::pp(j->first) << std::endl;
          pbes_expression g = pbesr(j->second, sigma);
          mCRL2log(log::debug, "stategraph") << "g = " << pbes_system::pp(j->second) << data::print_substitution(sigma) << " = " << pbes_system::pp(g) << std::endl;
          if (is_false(g))
          {
            continue;
          }
          propositional_variable_instantiation Ye = apply_substitution(j->first, sigma);
          propositional_variable_instantiation Y = project(Ye);
          propositional_variable_instantiation label = Ye;

          mCRL2log(log::debug, "stategraph") << "v = " << pbes_system::pp(Y) << std::endl;

          vertex_iterator q = m_control_flow_graph.find(Y);
          bool has_vertex = q != m_control_flow_graph.end();
          if (!has_vertex)
          {
            mCRL2log(log::debug, "stategraph") << "insert vertex v" << std::endl;
            q = m_control_flow_graph.insert_vertex(Y);
            todo.insert(&(q->second));
          }
          stategraph_vertex& v = q->second;
          u.guards.insert(g);
          mCRL2log(log::debug, "stategraph") << "insert guard g in vertex u" << std::endl;
          stategraph_vertex* target = &v;
          stategraph_edge e(source, target, label);
          mCRL2log(log::debug, "stategraph") << "insert edge (u, v) with label " << pbes_system::pp(label) << std::endl;
          u.outgoing_edges.insert(e);
          v.incoming_edges.insert(e);
        }
      }

      m_control_flow_graph.create_index();
    }

    /// \brief Computes the control flow graph
    void run(const pbes<>& p)
    {
      super::run(p);
      compute_control_flow_graph();
      mCRL2log(log::verbose) << m_control_flow_graph.print();
    }
};

/// \brief Algorithm class for the local variant of the stategraph algorithm
class stategraph_graph_local_algorithm: public stategraph_graph_algorithm
{
  public:
    typedef stategraph_graph_algorithm super;

  protected:
    // the control flow graph
    std::vector<control_flow_graph> m_control_flow_graphs;
    local_graph must_graph;
    local_graph may_graph;

    void compute_must_graph()
    {
      // create vertices
      const std::vector<stategraph_equation>& equations = m_pbes.equations();
      for (std::vector<stategraph_equation>::const_iterator k = equations.begin(); k != equations.end(); ++k)
      {
        propositional_variable X = k->variable();
        const std::vector<data::variable>& d_X = k->parameters();
        const std::vector<bool>& cf = m_is_control_flow[X.name()];
        for (std::size_t i = 0; i < d_X.size(); ++i)
        {
          if (cf[i])
          {
            must_graph.insert_vertex(local_vertex(X.name(), i));
          }
        }
      }

      // create edges
      for (std::vector<stategraph_equation>::const_iterator k = equations.begin(); k != equations.end(); ++k)
      {
        const stategraph_equation& eqn = *k;
        core::identifier_string X = eqn.variable().name();
        const std::vector<std::map<std::size_t, std::size_t> >& copy = k->copy();
        for (std::size_t i = 0; i < copy.size(); i++)
        {
          const std::map<std::size_t, std::size_t>& m = copy[i];
          for (std::map<std::size_t, std::size_t>::const_iterator j = m.begin(); j != m.end(); ++j)
          {
            std::size_t p = j->first;
            std::size_t l = j->second;
            const core::identifier_string& Y = eqn.predicate_variables()[i].first.name();
            must_graph.insert_edge(X, p, Y, l);
          }
        }
      }
      mCRL2log(log::debug, "stategraph") << "=== must graph ===\n" << must_graph.print() << std::endl;
    }

    void compute_may_graph()
    {
      // create vertices
      const std::vector<stategraph_equation>& equations = m_pbes.equations();
      for (std::vector<stategraph_equation>::const_iterator k = equations.begin(); k != equations.end(); ++k)
      {
        propositional_variable X = k->variable();
        const std::vector<data::variable>& d_X = k->parameters();
        const std::vector<bool>& cf = m_is_control_flow[X.name()];
        for (std::size_t i = 0; i < d_X.size(); ++i)
        {
          if (cf[i])
          {
            may_graph.insert_vertex(local_vertex(X.name(), i));
          }
        }
      }

      // create edges
      for (std::vector<stategraph_equation>::const_iterator k = equations.begin(); k != equations.end(); ++k)
      {
        const stategraph_equation& eqn = *k;
        core::identifier_string X = eqn.variable().name();
        const std::vector<std::map<std::size_t, std::size_t> >& copy = k->copy();
        for (std::size_t i = 0; i < copy.size(); i++)
        {
          const std::map<std::size_t, std::size_t>& m = copy[i];
          for (std::map<std::size_t, std::size_t>::const_iterator j = m.begin(); j != m.end(); ++j)
          {
            std::size_t p = j->first;
            std::size_t l = j->second;
            const core::identifier_string& Y = eqn.predicate_variables()[i].first.name();
            must_graph.insert_edge(X, p, Y, l);
          }
        }
      }
      mCRL2log(log::debug, "stategraph") << "=== may graph ===\n" << may_graph.print() << std::endl;
    }

  public:
    /// \brief Computes the control flow graph
    void run(const pbes<>& p)
    {
      super::run(p);
      compute_must_graph();
    }
};

} // namespace detail

} // namespace pbes_system

} // namespace mcrl2

#endif // MCRL2_PBES_DETAIL_STATEGRAPH_GRAPH_ALGORITHM_H