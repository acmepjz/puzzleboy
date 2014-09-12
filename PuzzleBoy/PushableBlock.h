#pragma once

#include "MFCSerializer.h"
#include "MySerializer.h"
#include <vector>

//----------------(MFC) file ver 1

#define NORMAL_BLOCK 0
#define ROTATE_BLOCK 1
#define TARGET_BLOCK 2

//----------------(MFC) file ver 2

//begin serialized form only

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

class VertexList;

class PushableBlock
{
public:
	PushableBlock();
	PushableBlock(const PushableBlock& obj);
	virtual ~PushableBlock();

	bool MFCSerialize(MFCSerializer& ar);

	void MySerialize(MySerializer& ar);

	void CreateSingle(int nType,int x,int y);

	void UnoptimizeSize(int nWidth,int nHeight);
	bool OptimizeSize();
	void MoveBlockInEditMode(int dx,int dy);

	bool HitTest(int x,int y) const;
	bool Rotate(int bIsClockwise);

	unsigned char operator()(int x,int y) const {return m_bData[y*m_w+x];}
	unsigned char& operator()(int x,int y) {return m_bData[y*m_w+x];}
	unsigned char operator[](int idx) const {return m_bData[idx];}
	unsigned char& operator[](int idx) {return m_bData[idx];}

	bool IsSolid() const;

	void CreateGraphics();
public:
	int m_nType,m_x,m_y,m_w,m_h;
	//if ROTATE_BLOCK then is length of up,left,down,right arms
	std::vector<unsigned char> m_bData;

	//graphics data
	VertexList *m_faces,*m_shade,*m_lines;
};


