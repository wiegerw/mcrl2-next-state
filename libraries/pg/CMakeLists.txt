add_mcrl2_library(pg
  INSTALL_HEADERS TRUE
  SOURCES
	Abortable.cpp
	ComponentSolver.cpp
	DecycleSolver.cpp
	DeloopSolver.cpp
	FocusListLiftingStrategy.cpp
	Graph.cpp
	LiftingStrategy.cpp
	LinearLiftingStrategy.cpp
	MaxMeasureLiftingStrategy.cpp
	OldMaxMeasureLiftingStrategy.cpp
	ParityGame.cpp
	ParityGame_IO.cpp
	ParityGameSolver.cpp
	ParityGame_verify.cpp
	PredecessorLiftingStrategy.cpp
	PriorityPromotionSolver.cpp
	RecursiveSolver.cpp
	SmallProgressMeasures.cpp
  DEPENDS
    mcrl2_pbes
    mcrl2_bes
)
