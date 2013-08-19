// LevelUndo.cpp : 实现文件
//

#include "stdafx.h"
#include "mfccourse.h"
#include "LevelUndo.h"
#include "PushableBlock.h"

IMPLEMENT_SERIAL(CLevelUndo,CObject,0x1);


// CLevelUndo

CLevelUndo::CLevelUndo()
: m_nType(0)
, m_x(0)
, m_y(0)
, m_dx(0)
, m_dy(0)
, m_nMovedBlockIndex(-1)
, m_objDeletedBlock(NULL)
{
}

CLevelUndo::~CLevelUndo()
{
	if(m_objDeletedBlock) delete m_objDeletedBlock;
}


// CLevelUndo 成员函数

void CLevelUndo::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// 在此添加存储代码
		switch(m_nType){
		case 1:
			{
				int n=m_objDeletedBlock?2:1;
				ar<<n<<m_x<<m_y<<m_dx<<m_dy<<m_nMovedBlockIndex;
				if(m_objDeletedBlock) m_objDeletedBlock->Serialize(ar);
			}
			break;
		default:
			ar<<m_nType<<m_x<<m_y<<m_dx<<m_dy;
			break;
		}
	}
	else
	{
		// 在此添加加载代码
		int n;
		ar>>n;
		switch(n){
		case 1:
		case 2:
			m_nType=1;
			ar>>m_x>>m_y>>m_dx>>m_dy>>m_nMovedBlockIndex;
			if(n==2){
				m_objDeletedBlock=new CPushableBlock;
				m_objDeletedBlock->Serialize(ar);
			}
			break;
		default:
			m_nType=n;
			ar>>m_x>>m_y>>m_dx>>m_dy;
			break;
		}
	}
}
