set(MCRL2_TOOLS
  besconvert
  complps2pbes
  lpsrealelm
  lpsstategraph
  lpssymbolicbisim
  pbesabsinthe
  pbespareqelm
  pbessymbolicbisim
  symbolic_exploration
)

if(MCRL2_ENABLE_EXPERIMENTAL)
  foreach(TOOL ${MCRL2_TOOLS})
    add_subdirectory(${TOOL})
  endforeach()
endif()
