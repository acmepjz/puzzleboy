#include "PuzzleBoyLevelData.h"
#include "PuzzleBoyApp.h"

#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

PuzzleBoyLevelData::PuzzleBoyLevelData()
: m_nWidth(0)
, m_nHeight(0)
{
}

PuzzleBoyLevelData::PuzzleBoyLevelData(const PuzzleBoyLevelData& obj)
: m_sLevelName(obj.m_sLevelName)
, m_nWidth(obj.m_nWidth)
, m_nHeight(obj.m_nHeight)
{
	m_bMapData=obj.m_bMapData;
	for(unsigned int i=0;i<obj.m_objBlocks.size();i++){
		m_objBlocks.push_back(new PushableBlock(*(obj.m_objBlocks[i])));
	}
}

PuzzleBoyLevelData::~PuzzleBoyLevelData()
{
	for(unsigned int i=0;i<m_objBlocks.size();i++) delete m_objBlocks[i];
}

static void DoMapDataRLE(const std::vector<unsigned char>& src,std::vector<unsigned char>& dst){
	dst.clear();
	if(src.empty()) return;

	int lastIndex=0;
	int m=src.size();
	unsigned char lastValue=src[0];

	for(int i=1;i<=m;i++){
		int count=i-lastIndex;
		if(count>=259 || i>=m || src[i]!=lastValue){
			if(count>=4){
				dst.push_back(RLE_TILE);
				dst.push_back(count-4);
				dst.push_back(lastValue);
			}else{
				do dst.push_back(lastValue);
				while((--count)>0);
			}

			if(i>=m) break;

			lastValue=src[i];
			lastIndex=i;
		}
	}
}

static bool DoMapDataUnRLE(const std::vector<unsigned char>& src,std::vector<unsigned char>& dst){
	dst.clear();
	if(src.empty()) return true;

	int m=src.size();
	for(int i=0;i<m;i++){
		unsigned char c=src[i];
		if(c==RLE_TILE){
			if(i+2>=m) return false;
			int count=src[i+1]+4;
			c=src[i+2];
			do dst.push_back(c);
			while((--count)>0);
			i+=2;
		}else{
			dst.push_back(c);
		}
	}

	return true;
}

bool PuzzleBoyLevelData::MFCSerialize(MFCSerializer& ar)
{
	if (ar.IsStoring())
	{
		// saving
		ar<<m_sLevelName<<m_nWidth<<m_nHeight;

		std::vector<unsigned char> filteredMapData;
		std::vector<PushableBlock*> filteredBlocks;

		filteredMapData=m_bMapData;
		for(int i=0;i<(int)m_objBlocks.size();i++){
			PushableBlock* objBlock=m_objBlocks[i];
			bool bFiltered=false;

			int type=objBlock->m_nType;
			if((type==NORMAL_BLOCK || type==TARGET_BLOCK)
				&& objBlock->m_w==1 && objBlock->m_h==1 && objBlock->m_bData[0]!=0)
			{
				int x=objBlock->m_x;
				int y=objBlock->m_y;
				if(x>=0 && y>=0 && x<m_nWidth && y<m_nHeight){
					int idx=y*m_nWidth+x;
					int md=filteredMapData[idx];
					if(md==0){
						filteredMapData[idx]=(type==NORMAL_BLOCK)?NORMAL_BLOCK_TILE:TARGET_BLOCK_TILE;
						bFiltered=true;
					}else if(md==TARGET_TILE && type==TARGET_BLOCK){
						filteredMapData[idx]=TARGET_BLOCK_ON_TARGET_TILE;
						bFiltered=true;
					}
				}
			}

			if(!bFiltered) filteredBlocks.push_back(objBlock);
		}

		std::vector<unsigned char> d;
		DoMapDataRLE(filteredMapData,d);
		ar<<d;

		ar.PutObjectArray(filteredBlocks,"CPushableBlock",2);
	}
	else
	{
		// loading
		for(unsigned int i=0;i<m_objBlocks.size();i++) delete m_objBlocks[i];
		m_objBlocks.clear();

		//check version
		if(ar.version>2) return false;

		ar>>m_sLevelName>>m_nWidth>>m_nHeight;
		if(ar.IsError()) return false;

		std::vector<unsigned char> d;
		ar>>d;
		if(!DoMapDataUnRLE(d,m_bMapData) || m_bMapData.size()!=m_nWidth*m_nHeight) return false;

		if(!ar.GetObjectArray(m_objBlocks,"CPushableBlock")) return false;

		//unpack data
		for(int y=0;y<m_nHeight;y++){
			for(int x=0;x<m_nWidth;x++){
				int idx=y*m_nWidth+x;
				unsigned char md=m_bMapData[idx];
				switch(md){
				case NORMAL_BLOCK_TILE:
				case TARGET_BLOCK_TILE:
				case TARGET_BLOCK_ON_TARGET_TILE:
					{
						PushableBlock* objBlock=new PushableBlock;
						objBlock->CreateSingle((md==NORMAL_BLOCK_TILE)?NORMAL_BLOCK:TARGET_BLOCK,x,y);
						m_objBlocks.push_back(objBlock);
					}
					m_bMapData[idx]=(md==TARGET_BLOCK_ON_TARGET_TILE)?TARGET_TILE:0;
					break;
				}
			}
		}
	}

	return true;
}

struct PushableBlockAndIndex{
	int index;
	PushableBlock* block;
};

static int PushableBlockAndIndexCompare(const void* lp1,const void* lp2){
	PushableBlockAndIndex* block1=(PushableBlockAndIndex*)lp1;
	PushableBlockAndIndex* block2=(PushableBlockAndIndex*)lp2;

	int i1,i2;

	i1=block1->block->m_y;
	i2=block2->block->m_y;

	if(i1<i2) return -1;
	else if(i1>i2) return 1;

	i1=block1->block->m_x;
	i2=block2->block->m_x;

	if(i1<i2) return -1;
	else if(i1>i2) return 1;

	i1=block1->block->m_nType==ROTATE_BLOCK?1:block1->block->m_h;
	i2=block2->block->m_nType==ROTATE_BLOCK?1:block2->block->m_h;

	if(i1<i2) return -1;
	else if(i1>i2) return 1;

	i1=block1->block->m_nType==ROTATE_BLOCK?1:block1->block->m_w;
	i2=block2->block->m_nType==ROTATE_BLOCK?1:block2->block->m_w;

	if(i1<i2) return -1;
	else if(i1>i2) return 1;

	i1=block1->block->m_nType;
	i2=block2->block->m_nType;

	if(i1<i2) return -1;
	else if(i1>i2) return 1;

	i1=block1->index;
	i2=block2->index;

	if(i1<i2) return -1;
	else if(i1>i2) return 1;

	return 0;
}

void PuzzleBoyLevelData::MySerialize(MySerializer& ar){
	ar.PutVUInt32(m_nWidth);
	ar.PutVUInt32(m_nHeight);

	assert(m_nWidth*m_nHeight==m_bMapData.size());
	for(unsigned int i=0;i<m_bMapData.size();i++){
		ar.PutIntN(m_bMapData[i],4);
	}
	ar.PutFlush();

	ar.PutVUInt32(m_objBlocks.size());

	int m=m_objBlocks.size();
	if(m>0){
		PushableBlockAndIndex* arr=(PushableBlockAndIndex*)malloc(m*sizeof(PushableBlockAndIndex));
		for(int i=0;i<m;i++){
			PushableBlockAndIndex t={i,m_objBlocks[i]};
			arr[i]=t;
		}
		qsort(arr,m,sizeof(PushableBlockAndIndex),PushableBlockAndIndexCompare);
		for(int i=0;i<m;i++){
			arr[i].block->MySerialize(ar);
		}
		free(arr);
	}
}

void PuzzleBoyLevelData::Create(int nWidth,int nHeight,int bPreserve,int nXOffset,int nYOffset)
{
	if(nWidth>0 && nHeight>0){
		int m=nWidth*nHeight;

		if(bPreserve){
			std::vector<unsigned char> bNewData;
			bNewData.resize(m);

			for(int j=0;j<nHeight;j++){
				for(int i=0;i<nWidth;i++){
					int x=i-nXOffset,y=j-nYOffset,idx=j*nWidth+i;
					if(x>=0 && y>=0 && x<m_nWidth && y<m_nHeight){
						bNewData[idx]=m_bMapData[y*m_nWidth+x];
					}else{
						bNewData[idx]=WALL_TILE;
					}
				}
			}

			m_bMapData=bNewData;

			for(unsigned int i=0;i<m_objBlocks.size();i++){
				PushableBlock *obj=m_objBlocks[i];
				obj->m_x+=nXOffset;
				obj->m_y+=nYOffset;
			}
		}else{
			m_bMapData.resize(m);
			for(int i=0;i<m;i++){
				m_bMapData[i]=(i==0)?PLAYER_TILE:(i==m-1?EXIT_TILE:FLOOR_TILE);
			}

			for(unsigned int i=0;i<m_objBlocks.size();i++) delete m_objBlocks[i];
			m_objBlocks.clear();
		}

		m_nWidth=nWidth;
		m_nHeight=nHeight;
	}
}

void PuzzleBoyLevelData::CreateDefault()
{
	m_nWidth=8;
	m_nHeight=8;

	m_bMapData.resize(64);

	m_bMapData[0]=PLAYER_TILE;
	for(int i=1;i<63;i++) m_bMapData[i]=FLOOR_TILE;
	m_bMapData[63]=EXIT_TILE;

	for(unsigned int i=0;i<m_objBlocks.size();i++) delete m_objBlocks[i];
	m_objBlocks.clear();

	m_sLevelName=toUTF16(_("Unnamed level"));
}

int PuzzleBoyLevelData::HitTestForPlacingBlocks(int x,int y,int nEditingBlockIndex) const
{
	for(int i=0;i<(int)m_objBlocks.size();i++){
		if(i!=nEditingBlockIndex && (m_objBlocks[i]->HitTest(x,y))) return i;
	}

	if(x>=0 && y>=0 && x<m_nWidth && y<m_nHeight){
		switch(m_bMapData[y*m_nWidth+x]){
		case FLOOR_TILE:
		case EMPTY_TILE:
		case TARGET_TILE:
			break;
		default:
			return -2;
		}
	}else{
		return -2;
	}

	return -1;
}
