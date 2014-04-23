#pragma once

class PuzzleBoyLevelData;

//Generate a random level with rotate block only.
//Note: width and height must <=8
//return value is the best step of generated level
int RandomTest(int width,int height,PuzzleBoyLevelData*& outputLevel);

