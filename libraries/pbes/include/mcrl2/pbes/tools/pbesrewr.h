// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/pbes/tools/pbesrewr.h
/// \brief add your file description here.

#ifndef MCRL2_PBES_TOOLS_PBESREWR_H
#define MCRL2_PBES_TOOLS_PBESREWR_H

#include <cassert>
#include "mcrl2/data/enumerator.h"
#include "mcrl2/data/detail/one_point_rule_preprocessor.h"
#include "mcrl2/pbes/io.h"
#include "mcrl2/pbes/one_point_rule_rewriter.h"
#include "mcrl2/pbes/rewrite.h"
#include "mcrl2/pbes/rewriter.h"
#include "mcrl2/pbes/tools.h"
#include "mcrl2/pbes/detail/bqnf_traverser.h"
#include "mcrl2/pbes/detail/ppg_traverser.h"
#include "mcrl2/pbes/detail/ppg_rewriter.h"
#include "mcrl2/utilities/logger.h"

namespace mcrl2 {

namespace pbes_system {

void pbesrewr(const std::string& input_filename,
              const std::string& output_filename,
              const data::rewrite_strategy rewrite_strategy,
              pbes_rewriter_type rewriter_type,
              bool skip_data
             )
{
  // load the pbes
  pbes<> p;
  load_pbes(p, input_filename);

  // data rewriter
  data::rewriter datar(p.data(), rewrite_strategy);

  // pbes rewriter
  switch (rewriter_type)
  {
    case simplify:
    {
      simplifying_rewriter<pbes_expression, data::rewriter> pbesr(datar);
      pbes_rewrite(p, pbesr);
      break;
    }
    case quantifier_all:
    {
      data::data_enumerator datae(p.data(), datar);
      data::rewriter_with_variables datarv(datar);
      bool enumerate_infinite_sorts = true;
      enumerate_quantifiers_rewriter<pbes_expression, data::rewriter_with_variables, data::data_enumerator> pbesr(datarv, datae, enumerate_infinite_sorts, skip_data);
      pbes_rewrite(p, pbesr);
      break;
    }
    case quantifier_finite:
    {
      data::data_enumerator datae(p.data(), datar);
      data::rewriter_with_variables datarv(datar);
      bool enumerate_infinite_sorts = false;
      enumerate_quantifiers_rewriter<pbes_expression, data::rewriter_with_variables, data::data_enumerator> pbesr(datarv, datae, enumerate_infinite_sorts, skip_data);
      pbes_rewrite(p, pbesr);
      break;
    }
    case quantifier_one_point:
    {
      // first preprocess data expressions
      data::detail::one_point_rule_preprocessor one_point_processor;
      data_rewriter<pbes_expression, data::detail::one_point_rule_preprocessor> datar_onepoint(one_point_processor);
      pbes_rewrite(p, datar_onepoint);

      // apply the one point rule rewriter
      one_point_rule_rewriter pbesr;
      pbes_rewrite(p, pbesr);

      // post processing: apply the simplifying rewriter
      simplifying_rewriter<pbes_expression, data::rewriter> simp(datar);
      pbes_rewrite(p, simp);
      break;
    }
    case pfnf:
    {
      pfnf_rewriter pbesr;
      pbes_rewrite(p, pbesr);
      break;
    }
    case ppg:
    {
      //bool bqnf = detail::is_bqnf(p);
      //std::clog << "bqnf_traverser says: p is " << (bqnf ? "" : "NOT ") << "in BQNF." << std::endl;
      bool ppg = detail::is_ppg(p);
      if (ppg)
      {
        mCRL2log(log::verbose) << "PBES is already a PPG." << std::endl;
      }
      else
      {
        mCRL2log(log::verbose) << "Rewriting..." << std::endl;
        pbes<> q = detail::to_ppg(p);
        mCRL2log(log::verbose) << "Rewriting done." << std::endl;
        ppg = detail::is_ppg(q);
        if (!ppg)
        {
          throw(std::runtime_error("The result PBES if not a PPG!"));
        }
        p = q;
      }
      break;
    }
    case bqnf_quantifier:
    {
      bqnf_rewriter pbesr;
      pbes_rewrite(p, pbesr);
      break;
    }
    case prover:
    default:
    {
      // Just ignore.
      assert(0);  // The PBES rewriter cannot be activated through
      // the commandline. So, we cannot end up here.
      break;
    }
  }
  p.save(output_filename);
}

} // namespace pbes_system

} // namespace mcrl2

#endif // MCRL2_PBES_TOOLS_PBESREWR_H