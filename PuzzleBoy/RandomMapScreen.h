#pragma once

#include "SimpleListScreen.h"
#include "TestRandomLevel.h"

class PuzzleBoyLevelFile;
class MT19937;

//ad-hoc structure
struct RandomMapSizeData{
	char width; // should be 1-15, or 0 means choose a type from a predefined list
	char height; // should be 1-15, or a 1-based index if width=0
	char playerCount; // should be 1-3
	char boxType; // -1=no box, or (NORMAL_BLOCK, TARGET_BLOCK) | ((boxCount-1)<<4)
};

//ad-hoc random level TEST
class RandomMapScreen:public SimpleListScreen{
public:
	void OnDirty() override;
	int OnClick(int index) override;
	int DoModal() override;

	static int DoRandom(int type,PuzzleBoyLevelData*& outputLevel,MT19937 *rnd,void *userData=NULL,RandomLevelCallback callback=NULL);
	static int DoRandomIndirect(RandomMapSizeData size, PuzzleBoyLevelData*& outputLevel, MT19937 *rnd, void *userData=NULL, RandomLevelCallback callback=NULL);

	static PuzzleBoyLevelFile* DoRandomLevels(int type, int levelCount, bool headless = false);
	static PuzzleBoyLevelFile* DoRandomLevelsIndirect(const RandomMapSizeData& size, int levelCount, bool headless = false);
};
