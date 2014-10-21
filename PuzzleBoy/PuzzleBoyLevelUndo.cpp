#include "PuzzleBoyLevelUndo.h"
#include "PushableBlock.h"

PuzzleBoyLevelUndo::PuzzleBoyLevelUndo()
: m_nType(0)
, m_x(0)
, m_y(0)
, m_dx(0)
, m_dy(0)
, m_nMovedBlockIndex(-1)
, m_objDeletedBlock(NULL)
, m_nRecordIndex(-1)
{
}

PuzzleBoyLevelUndo::~PuzzleBoyLevelUndo()
{
	if(m_objDeletedBlock) delete m_objDeletedBlock;
}

bool PuzzleBoyLevelUndo::MFCSerialize(MFCSerializer& ar)
{
	if (ar.IsStoring())
	{
		// saving
		switch(m_nType){
		case 1:
			{
				int n=m_objDeletedBlock?2:1;
				ar<<n<<m_x<<m_y<<m_dx<<m_dy<<m_nMovedBlockIndex;
				if(m_objDeletedBlock) m_objDeletedBlock->MFCSerialize(ar);
			}
			break;
		default:
			ar<<m_nType<<m_x<<m_y<<m_dx<<m_dy;
			break;
		}
	}
	else
	{
		// loading
		int n;
		ar>>n;
		switch(n){
		case 1:
		case 2:
			m_nType=1;
			ar>>m_x>>m_y>>m_dx>>m_dy>>m_nMovedBlockIndex;
			if(n==2){
				m_objDeletedBlock=new PushableBlock;
				if(!m_objDeletedBlock->MFCSerialize(ar)){
					delete m_objDeletedBlock;
					return false;
				}
			}
			break;
		default:
			m_nType=n;
			ar>>m_x>>m_y>>m_dx>>m_dy;
			break;
		}
	}

	return true;
}

void PuzzleBoyLevelUndo::MySerialize(MySerializer& ar)
{
	if (ar.IsStoring())
	{
		// saving
		switch(m_nType){
		case 1:
			ar.PutInt8(m_objDeletedBlock?2:1);
			ar.PutVUInt32(m_x);
			ar.PutVUInt32(m_y);
			ar.PutVSInt32(m_dx);
			ar.PutVSInt32(m_dy);
			ar.PutVSInt32(m_nMovedBlockIndex);
			if(m_objDeletedBlock) m_objDeletedBlock->MySerialize(ar);
			break;
		default:
			ar.PutInt8(m_nType);
			ar.PutVUInt32(m_x);
			ar.PutVUInt32(m_y);
			ar.PutVSInt32(m_dx);
			ar.PutVSInt32(m_dy);
			break;
		}
	}
	else
	{
		// loading
		int n=ar.GetInt8();
		switch(n){
		case 1:
		case 2:
			m_nType=1;
			m_x=ar.GetVUInt32();
			m_y=ar.GetVUInt32();
			m_dx=ar.GetVSInt32();
			m_dy=ar.GetVSInt32();
			m_nMovedBlockIndex=ar.GetVSInt32();
			if(n==2){
				m_objDeletedBlock=new PushableBlock;
				m_objDeletedBlock->MySerialize(ar);
			}
			break;
		default:
			m_nType=n;
			m_x=ar.GetVUInt32();
			m_y=ar.GetVUInt32();
			m_dx=ar.GetVSInt32();
			m_dy=ar.GetVSInt32();
			break;
		}
	}
}
