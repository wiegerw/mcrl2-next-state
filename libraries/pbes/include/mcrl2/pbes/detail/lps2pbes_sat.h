// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/pbes/detail/lps2pbes_sat.h
/// \brief add your file description here.

#ifndef MCRL2_PBES_DETAIL_LPS2PBES_SAT_H
#define MCRL2_PBES_DETAIL_LPS2PBES_SAT_H

#include "mcrl2/data/detail/data_utility.h"
#include "mcrl2/data/detail/find.h"
#include "mcrl2/modal_formula/replace.h"
#include "mcrl2/modal_formula/traverser.h"
#include "mcrl2/pbes/detail/lps2pbes_utility.h"

namespace mcrl2 {

namespace pbes_system {

namespace detail {

pbes_expression Sat(const lps::multi_action& a, const action_formulas::action_formula& x);

template <typename Derived>
struct sat_traverser: public action_formulas::action_formula_traverser<Derived>
{
  typedef action_formulas::action_formula_traverser<Derived> super;
  using super::enter;
  using super::leave;
  using super::operator();

#if BOOST_MSVC
#include "mcrl2/core/detail/traverser_msvc.inc.h"
#endif

  const lps::multi_action& a;
  atermpp::vector<pbes_expression> result_stack;

  sat_traverser(const lps::multi_action& a_)
    : a(a_)
  {}

  Derived& derived()
  {
    return static_cast<Derived&>(*this);
  }

  void push(const pbes_expression& x)
  {
    result_stack.push_back(x);
  }

  const pbes_expression& top() const
  {
    return result_stack.back();
  }

  pbes_expression pop()
  {
    pbes_expression result = top();
    result_stack.pop_back();
    return result;
  }

  void leave(const data::data_expression& x)
  {
    push(x);
  }

  void leave(const lps::multi_action& x)
  {
    push(lps::equal_multi_actions(a, x));
  }

  void leave(const action_formulas::true_& x)
  {
    push(true_());
  }

  void leave(const action_formulas::false_& x)
  {
    push(false_());
  }

  void operator()(const action_formulas::not_& x)
  {
    push(not_(Sat(a, x.operand())));
  }

  void leave(const action_formulas::and_& x)
  {
    pbes_expression right = pop();
    pbes_expression left = pop();
    push(and_(left, right));
  }

  void leave(const action_formulas::or_& x)
  {
    pbes_expression right = pop();
    pbes_expression left = pop();
    push(or_(left, right));
  }

  void leave(const action_formulas::imp& x)
  {
    pbes_expression right = pop();
    pbes_expression left = pop();
    push(imp(left, right));
  }

  void operator()(const action_formulas::forall& x)
  {
    data::set_identifier_generator id_generator;
    id_generator.add_identifiers(data::detail::variable_names(lps::find_variables(a)));
    id_generator.add_identifiers(data::detail::variable_names(action_formulas::find_variables(x)));
    data::variable_list y = pbes_system::detail::make_fresh_variables(x.variables(), id_generator, false);
    action_formulas::action_formula alpha = x.body();
    push(forall(y, Sat(a, action_formulas::replace_free_variables(alpha, data::make_sequence_sequence_substitution(x.variables(), y)))));
  }

  void operator()(const action_formulas::exists& x)
  {
    data::set_identifier_generator id_generator;
    id_generator.add_identifiers(data::detail::variable_names(lps::find_variables(a)));
    id_generator.add_identifiers(data::detail::variable_names(action_formulas::find_variables(x)));
    data::variable_list y = pbes_system::detail::make_fresh_variables(x.variables(), id_generator, false);
    action_formulas::action_formula alpha = x.body();
    push(exists(y, Sat(a, action_formulas::replace_free_variables(alpha, data::make_sequence_sequence_substitution(x.variables(), y)))));
  }

  void operator()(const action_formulas::at& x)
  {
    data::data_expression t = a.time();
    action_formulas::action_formula alpha = x.operand();
    data::data_expression u = x.time_stamp();
    push(and_(Sat(a, alpha), data::equal_to(t, u)));
  }
};

template <template <class> class Traverser>
struct apply_sat_traverser: public Traverser<apply_sat_traverser<Traverser> >
{
  typedef Traverser<apply_sat_traverser<Traverser> > super;
  using super::enter;
  using super::leave;
  using super::operator();
  using super::top;

  apply_sat_traverser(const lps::multi_action& a)
    : super(a)
  {}

#ifdef BOOST_MSVC
#include "mcrl2/core/detail/traverser_msvc.inc.h"
#endif
};

inline
pbes_expression Sat(const lps::multi_action& a, const action_formulas::action_formula& x)
{
  apply_sat_traverser<sat_traverser> f(a);
  f(x);
  return f.top();
}

} // namespace detail

} // namespace pbes_system

} // namespace mcrl2

#endif // MCRL2_PBES_DETAIL_LPS2PBES_SAT_H