#pragma once

#include "PuzzleBoyLevel.h"

const int TestSolver_MaxBlockCount=64;

//#define USE_SOLUTION_REACHABLE

struct TestSolverExtendedData{
	LevelSolverState state;
	int blockUsed;
	int moves;
	int pushes;

	//1=reachable, other=unreachable (blocked)
	unsigned char reachable[256];
	//0=unreachable (blocked), 1=reachable, 1 bit per direction
	unsigned char blockStateReachable[TestSolver_MaxBlockCount];

#ifdef USE_SOLUTION_REACHABLE
	unsigned char solutionReachable[256];
#endif

	int deadlockBlockCount;
};

//test solver for level with rotate blocks only
int TestSolver_SolveIt(const PuzzleBoyLevel& level,u8string* rec=0,void* userData=0,LevelSolverCallback callback=0,TestSolverExtendedData *ed=0);

enum SolverType{
	TEST_SOLVER,
	SOLVER_MAX,
};

//highly experimental function.
//rec: the output record
//return value: 1=success 0=no solution -1=abort -2=error
//note: call level.StartGame() before calling this function
int RunSolver(SolverType type,const PuzzleBoyLevel& level,u8string* rec=0,void* userData=0,LevelSolverCallback callback=0);
