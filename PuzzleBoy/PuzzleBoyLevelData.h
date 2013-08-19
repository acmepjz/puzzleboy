#pragma once

#include "PushableBlock.h"
#include "MFCSerializer.h"
#include "MySerializer.h"
#include "UTF8-16.h"
#include <vector>

//----------------(MFC) file ver 1

#define FLOOR_TILE 0
#define WALL_TILE 1
#define EMPTY_TILE 2
#define TARGET_TILE 3
#define EXIT_TILE 4
#define PLAYER_TILE 5

//----------------(MFC) file ver 2

//begin edit mode only
#define PLAYER_AND_TARGET_TILE 6

//begin serialized form only

//all these blocks are 1x1
#define NORMAL_BLOCK_TILE 7
#define TARGET_BLOCK_TILE 8
#define TARGET_BLOCK_ON_TARGET_TILE 9
//format: RLE_TILE n T, n+4=tile count
//additional format in 4-bit file: RLE_TILE 0xF n n n T, n+37=tile count
#define RLE_TILE 15

//----------------

class PuzzleBoyLevelData
{
public:
	PuzzleBoyLevelData();
	PuzzleBoyLevelData(const PuzzleBoyLevelData& obj);
	virtual ~PuzzleBoyLevelData();

	bool MFCSerialize(MFCSerializer& ar);

	void MySerialize(MySerializer& ar);

	void Create(int nWidth,int nHeight,int bPreserve=0,int nXOffset=0,int nYOffset=0);
	void CreateDefault();

	int HitTestForPlacingBlocks(int x,int y,int nEditingBlockIndex=-1) const;

	unsigned char operator()(int x,int y) const {return m_bMapData[y*m_nWidth+x];}
	unsigned char& operator()(int x,int y) {return m_bMapData[y*m_nWidth+x];}
	unsigned char operator[](int idx) const {return m_bMapData[idx];}
	unsigned char& operator[](int idx) {return m_bMapData[idx];}

public:
	u16string m_sLevelName;
	static const int ChecksumSize=160/8;
	int m_nWidth;
	int m_nHeight;
	std::vector<unsigned char> m_bMapData;
	std::vector<PushableBlock*> m_objBlocks;
};


