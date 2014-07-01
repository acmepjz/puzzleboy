#pragma once

#include "SimpleListScreen.h"
#include "TestRandomLevel.h"

class PuzzleBoyLevelFile;
class MT19937;

//ad-hoc random level TEST
class RandomMapScreen:public SimpleListScreen{
public:
	void OnDirty() override;
	int OnClick(int index) override;
	int DoModal() override;

	static int DoRandom(int type,PuzzleBoyLevelData*& outputLevel,MT19937 *rnd,void *userData=NULL,RandomLevelCallback callback=NULL);

	static PuzzleBoyLevelFile* DoRandomLevels(int type,int levelCount);
};
