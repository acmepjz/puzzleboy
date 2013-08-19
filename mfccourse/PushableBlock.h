#pragma once

#include "MySerializer.h"

// CPushableBlock ÃüÁîÄ¿±ê

#define NORMAL_BLOCK 0
#define ROTATE_BLOCK 1
#define TARGET_BLOCK 2

//----------------(MFC) file ver 2

//serialized form only

//format: T+(h-1)*4+(w-1) (1<=w,h<=4)
#define NORMAL_BLOCK_WITH_FIXED_SIZE 0x10
#define TARGET_BLOCK_WITH_FIXED_SIZE 0x30
//format: xxxx3210  number means m_bData[number] (0 or 1)
#define ROTATE_BLOCK_WITH_FIXED_SIZE 0x20
//format: m_bData omitted
#define NORMAL_BLOCK_SOLID 0x40
#define ROTATE_BLOCK_WITH_EQUAL_ARMS 0x41
#define TARGET_BLOCK_SOLID 0x42

//----------------

class CPushableBlock : public CObject
{
public:
	DECLARE_SERIAL(CPushableBlock);

	CPushableBlock();
	CPushableBlock(const CPushableBlock& obj);
	virtual ~CPushableBlock();

	void Serialize(CArchive& ar);

	void MySerialize(MySerializer& ar);

	void CreateSingle(int nType,int x,int y);
	void Draw(CDC *pDC,RECT r,int nBlockSize,int nEdgeSize,int nAnimation) const;

	void UnoptimizeSize(int nWidth,int nHeight);
	bool OptimizeSize();
	void MoveBlockInEditMode(int dx,int dy);

	bool HitTest(int x,int y) const;
	bool Rotate(int bIsClockwise);

	unsigned char operator()(int x,int y) const {return m_bData[y*m_w+x];}
	unsigned char& operator()(int x,int y) {return m_bData[y*m_w+x];}
	unsigned char operator[](int idx) const {return m_bData[idx];}
	unsigned char& operator[](int idx) {return m_bData[idx];}
public:
	int m_nType,m_x,m_y,m_w,m_h;
	//if ROTATE_BLOCK then is length of up,left,down,right arms
	CArray<unsigned char> m_bData;
};


