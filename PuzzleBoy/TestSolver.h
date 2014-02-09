#pragma once

#include "PuzzleBoyLevel.h"

//test solver for level with rotate blocks only
int TestSolver_SolveIt(const PuzzleBoyLevel& level,u8string& rec,void* userData,LevelSolverCallback callback);

enum SolverType{
	TEST_SOLVER,
	SOLVER_MAX,
};

//highly experimental function.
//rec: the output record
//return value: 1=success 0=no solution -1=abort -2=error
//note: call level.StartGame() before calling this function
int RunSolver(SolverType type,const PuzzleBoyLevel& level,u8string& rec,void* userData,LevelSolverCallback callback);
