// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file complps2pbes.cpp
/// \brief Add your file description here.

#include "boost.hpp" // precompiled headers

#include "mcrl2/atermpp/aterm_init.h"
#include "mcrl2/pbes/tools.h"
#include "mcrl2/utilities/text_utility.h"
#include "mcrl2/utilities/input_output_tool.h"
#include "mcrl2/utilities/pbes_output_tool.h"
#include "mcrl2/utilities/mcrl2_gui_tool.h"

using namespace mcrl2;
using namespace mcrl2::pbes_system;
using namespace mcrl2::utilities;
using namespace mcrl2::utilities::tools;
using namespace mcrl2::log;

class complps2pbes_tool : public input_output_tool
{
    typedef input_output_tool super;

  protected:
    std::string formfilename;

    std::string synopsis() const
    {
      return "[OPTION]... --formula=FILE [INFILE [OUTFILE]]\n";
    }

    void add_options(interface_description& desc)
    {
      super::add_options(desc);
      desc.add_option("formula", make_file_argument("FILE"),
                      "use the state formula from FILE", 'f');
    }

    void parse_options(const command_line_parser& parser)
    {
      super::parse_options(parser);

      //check for presence of -f
      if (parser.options.count("formula"))
      {
        formfilename = parser.option_argument("formula");
      }
    }

  public:
    complps2pbes_tool() : super(
        "complps2pbes",
        "Wieger Wesselink",
        "generate a PBES from an LPS and a state formula",
        "Convert the state formula in FILE and the LPS in INFILE to a parameterised "
        "boolean equation system (PBES) and save it to OUTFILE. If OUTFILE is not "
        "present, stdout is used. If INFILE is not present, stdin is used."
      )
    {}

    bool run()
    {
      complps2pbes(input_filename(),
               output_filename(),
               formfilename
             );
      return true;
    }

};

class complps2pbes_gui_tool: public mcrl2_gui_tool<complps2pbes_tool>
{
  public:
    complps2pbes_gui_tool()
    {
      m_gui_options["formula"] = create_filepicker_widget("modal mu-calculus files (*.mcf)|*.mcf|Text files(*.txt)|*.txt|All Files (*.*)|*.*");
    }
};


int main(int argc, char** argv)
{
  MCRL2_ATERMPP_INIT(argc, argv)

  return complps2pbes_gui_tool().execute(argc, argv);
}