#pragma once

// CLevelUndo ÃüÁîÄ¿±ê

class CPushableBlock;

class CLevelUndo : public CObject
{
public:
	DECLARE_SERIAL(CLevelUndo);

	CLevelUndo();
	virtual ~CLevelUndo();

	void Serialize(CArchive& ar);

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
	CPushableBlock* m_objDeletedBlock;
};


