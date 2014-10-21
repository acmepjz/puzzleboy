#pragma once

#include "MFCSerializer.h"
#include "MySerializer.h"

class PushableBlock;

class PuzzleBoyLevelUndo
{
public:
	PuzzleBoyLevelUndo();
	virtual ~PuzzleBoyLevelUndo();

	bool MFCSerialize(MFCSerializer& ar);
	void MySerialize(MySerializer& ar);

public:
	//event type. 0=switch player, only m_x, m_y, m_dx, m_dy are used.
	//1=move. 2=move then delete block (only in serialized data)
	int m_nType;

	//position before move
	int m_x,m_y;

	//move direction or switched position
	int m_dx,m_dy;

	//moved block index or deleted block index. -1 means none
	int m_nMovedBlockIndex;

	//backup of deleted block after moved
	PushableBlock* m_objDeletedBlock;

	//only used in play from record, unreliable
	int m_nRecordIndex;
};


