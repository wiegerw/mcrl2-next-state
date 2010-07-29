// Author(s): Muck van Weerdenburg
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file lts.cpp

#include <cstring>
#include "mcrl2/lps/specification.h"
#include "aterm2.h"
#include "mcrl2/core/detail/struct_core.h"
#include "mcrl2/core/print.h"
#include "mcrl2/lts/lps2lts_lts.h"
#include "mcrl2/lts/lts_io.h"

#include "mcrl2/core/messaging.h"

using namespace std;
using namespace mcrl2::core;
using namespace mcrl2::core::detail;
using namespace mcrl2::lts;
using namespace mcrl2::lts::detail;

namespace mcrl2
{
  namespace lts
  {
    #define NAME "lps2lts"

    void lps2lts_lts::open_lts(const char *filename, lps2lts_lts_options &opts)
    {
      lts_filename = std::string(filename);
      if ( term_nil == atermpp::aterm_appl() )
      {
        term_nil = gsMakeNil();
        term_nil.protect();
        afun_pair = atermpp::function_symbol("pair", 2, false);
        afun_pair.protect();
      }
      lts_opts = opts;
      switch ( lts_opts.outformat )
      {
        case lts_aut:
          gsVerboseMsg("writing state space in AUT format to '%s'.\n",filename);
          lts_opts.outinfo = false;
          aut.open(filename);
          if ( !aut.is_open() )
          {
            gsErrorMsg("cannot open '%s' for writing\n",filename);
            exit(1);
          }
          break;
        case lts_mcrl2:
          gsVerboseMsg("writing state space in mCRL2 format to '%s'.\n",filename);
          {
            SVCbool b;

            b = lts_opts.outinfo?SVCfalse:SVCtrue;
            SVCopen(svc,const_cast< char* > (filename),SVCwrite,&b); // XXX check result

            SVCsetCreator(svc,const_cast < char* > (NAME));
            if (lts_opts.outinfo)
              SVCsetType(svc, const_cast < char* > ("mCRL2+info"));
            else
              SVCsetType(svc, const_cast < char* > ("mCRL2"));
            svcparam = SVCnewParameter(svc,(ATerm) ATmakeList0(),&b);
          }
          break;
        case lts_none:
          gsVerboseMsg("not saving state space.\n");
          break;
        default:
          gsVerboseMsg("writing state space in %s format to '%s'.\n",
                      mcrl2::lts::detail::string_for_type(lts_opts.outformat).c_str(),filename);
          generic_lts = new lts();
          aterm2state = ATtableCreate(10000,50);
          aterm2label = ATtableCreate(100,50);
          break;
      }
    }

    void lps2lts_lts::save_initial_state(boost::uint64_t idx, ATerm state)
    {
      initial_state = idx;
      switch ( lts_opts.outformat )
      {
        case lts_aut:
          aut << "des (0,0,0)                                      " << endl;
          break;
        case lts_mcrl2:
          {
            SVCbool b;
            if ( lts_opts.outinfo )
            {
              SVCsetInitialState(svc,SVCnewState(svc,(ATerm) lts_opts.nstate->makeStateVector(state),&b));
            } else {
              SVCsetInitialState(svc,SVCnewState(svc,(ATerm) ATmakeInt(initial_state),&b));
            }
          }
          break;
        case lts_none:
          break;
        default:
          {
            ATerm t = ATtableGet(aterm2state,state);
            if ( t == NULL )
            {
              t = (ATerm) ATmakeInt(generic_lts->add_state((ATerm) lts_opts.nstate->makeStateVector(state)));
              ATtablePut(aterm2state,state,t);
            }
            generic_lts->set_initial_state(ATgetInt((ATermInt) t));
          }
          break;
      }
    }

    void lps2lts_lts::save_transition(boost::uint64_t idx_from, ATerm from, ATermAppl action, boost::uint64_t idx_to, ATerm to)
    {
      switch ( lts_opts.outformat )
      {
        case lts_aut:
          if ( idx_from == initial_state )
            idx_from = 0;
          if ( idx_to == initial_state )
            idx_to = 0;
          aut << "(" << idx_from << ",\"";
          PrintPart_CXX(aut,(ATerm) action,ppDefault);
          aut << "\"," << idx_to << ")" << endl;
          aut.flush();
          break;
        case lts_mcrl2:
          if ( lts_opts.outinfo )
          {
            SVCbool b;
            SVCputTransition(svc,
              SVCnewState(svc,(ATerm) lts_opts.nstate->makeStateVector(from),&b),
              SVCnewLabel(svc,(ATerm) ATmakeAppl2(AFun(afun_pair),(ATerm) action,(ATerm) static_cast<ATermAppl>(term_nil)),&b),
              SVCnewState(svc,(ATerm) lts_opts.nstate->makeStateVector(to),&b),
              svcparam);
          } else {
            SVCbool b;
            SVCputTransition(svc,
              SVCnewState(svc,(ATerm) ATmakeInt(idx_from),&b),
              SVCnewLabel(svc,(ATerm) ATmakeAppl2(AFun(afun_pair),(ATerm) action,(ATerm) static_cast<ATermAppl>(term_nil)),&b),
              SVCnewState(svc,(ATerm) ATmakeInt(idx_to),&b),
              svcparam);
          }
          break;
        case lts_none:
          break;
        default:
          {
            unsigned int from_state;
            unsigned int to_state;
            unsigned int label;
            ATerm t = ATtableGet(aterm2state,from);
            if ( t == NULL )
            {
              t = (ATerm) ATmakeInt(generic_lts->add_state((ATerm) lts_opts.nstate->makeStateVector(from)));
              ATtablePut(aterm2state,from,t);
            }
            from_state = ATgetInt((ATermInt) t);
            t = ATtableGet(aterm2state,to);
            if ( t == NULL )
            {
              t = (ATerm) ATmakeInt(generic_lts->add_state((ATerm) lts_opts.nstate->makeStateVector(to)));
              ATtablePut(aterm2state,to,t);
            }
            to_state = ATgetInt((ATermInt) t);
            t = ATtableGet(aterm2label,(ATerm) action);
            if ( t == NULL )
            {
              t = (ATerm) ATmakeInt(generic_lts->add_label((ATerm) action, ATisEmpty((ATermList) ATgetArgument(action,0)) == ATtrue));
              ATtablePut(aterm2state,(ATerm) action,t);
            }
            label = ATgetInt((ATermInt) t);
            generic_lts->add_transition(transition(from_state,label,to_state));
          }
          break;
      }
    }

    void lps2lts_lts::close_lts(boost::uint64_t num_states, boost::uint64_t num_trans)
    {
      switch ( lts_opts.outformat )
      {
        case lts_aut:
          aut.seekp(0);
          aut << "des (0," << num_trans << "," << num_states << ")";
          aut.close();
          break;
        case lts_mcrl2:
          {
            int e = SVCclose(svc);
            if ( e )
            {
              gsErrorMsg("svcerror: %s\n",SVCerror(e));
            }
            mcrl2::lts::detail::add_extra_mcrl2_svc_data(lts_filename, mcrl2::data::detail::data_specification_to_aterm_data_spec(lts_opts.spec->data()),
                         lts_opts.spec->process().process_parameters(), lts_opts.spec->action_labels());
          }
          break;
        case lts_none:
          break;
        default:
          {
            lts_extra ext;
            if ( !lts_opts.outinfo )
            {
              generic_lts->remove_state_values();
            }
            if ( lts_opts.outformat == lts_fsm )
            {
              ext = lts_extra(*lts_opts.spec);
            } else if ( lts_opts.outformat == lts_dot )
            {
              string fn(lts_filename);
              lts_dot_options extdot;
              extdot.name = &fn;
              extdot.print_states = lts_opts.outinfo;
              ext = lts_extra(extdot);
            }
            generic_lts->write_to(lts_filename,lts_opts.outformat,ext);

            ATtableDestroy(aterm2label);
            ATtableDestroy(aterm2state);
            delete generic_lts;
            generic_lts = NULL;
            break;
          }
      }

      // Avoid static initialisation/descruction fiasco
      lts_opts.spec.reset();
    }

    void lps2lts_lts::remove_lts()
    {
      switch ( lts_opts.outformat )
      {
        case lts_aut:
          aut.close();
          break;
        case lts_mcrl2:
          {
            int e = SVCclose(svc);
            if ( e )
            {
              gsErrorMsg("svcerror: %s\n",SVCerror(e));
            }
          }
          break;
        case lts_none:
          break;
        default:
          ATtableDestroy(aterm2label);
          ATtableDestroy(aterm2state);
          delete generic_lts;
          generic_lts = NULL;
          break;
      }
      remove(lts_filename.c_str());
    }

  }
}


