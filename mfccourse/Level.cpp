// Level.cpp : 实现文件
//

#include "stdafx.h"
#include "mfccourse.h"
#include "Level.h"
#include "PushableBlock.h"
#include "LevelUndo.h"

#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

IMPLEMENT_SERIAL(CLevel,CObject,VERSIONABLE_SCHEMA | 2);

inline int GetAnimationTime(){
	return 1<<theApp.m_nAnimationSpeed;
}

// CLevel

CLevel::CLevel()
: m_nWidth(0)
, m_nHeight(0)
, m_nPlayerX(-1)
, m_nPlayerY(-1)
, m_objCurrentUndo(NULL)
, m_nMoves(0)
{
}

CLevel::CLevel(const CLevel& obj)
: m_sLevelName(obj.m_sLevelName)
, m_nWidth(obj.m_nWidth)
, m_nHeight(obj.m_nHeight)
, m_nPlayerX(-1)
, m_nPlayerY(-1)
, m_objCurrentUndo(NULL)
, m_nMoves(0)
{
	m_bMapData.Copy(obj.m_bMapData);
	for(int i=0;i<obj.m_objBlocks.GetSize();i++){
		m_objBlocks.Add(new CPushableBlock(*(CPushableBlock*)(obj.m_objBlocks[i])));
	}
}


CLevel::~CLevel()
{
	for(int i=0;i<m_objBlocks.GetSize();i++) delete m_objBlocks[i];
	for(int i=0;i<m_objUndo.GetSize();i++) delete m_objUndo[i];
}


// CLevel 成员函数

void CLevel::Create(int nWidth,int nHeight,int bPreserve,int nXOffset,int nYOffset)
{
	if(nWidth>0 && nHeight>0){
		int m=nWidth*nHeight;

		if(bPreserve){
			CArray<unsigned char> bNewData;
			bNewData.SetSize(m);

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

			m_bMapData.Copy(bNewData);

			for(int i=0;i<m_objBlocks.GetSize();i++){
				CPushableBlock *obj=(CPushableBlock*)(m_objBlocks[i]);
				obj->m_x+=nXOffset;
				obj->m_y+=nYOffset;
			}
		}else{
			m_bMapData.SetSize(m);
			for(int i=0;i<m;i++){
				m_bMapData[i]=(i==0)?PLAYER_TILE:(i==m-1?EXIT_TILE:FLOOR_TILE);
			}

			for(int i=0;i<m_objBlocks.GetSize();i++) delete m_objBlocks[i];
			m_objBlocks.RemoveAll();
		}

		m_nWidth=nWidth;
		m_nHeight=nHeight;
		m_nMoves=0;
	}
}

void CLevel::CreateDefault()
{
	m_nWidth=8;
	m_nHeight=8;
	m_nMoves=0;

	m_bMapData.SetSize(64);

	m_bMapData[0]=PLAYER_TILE;
	for(int i=1;i<63;i++) m_bMapData[i]=FLOOR_TILE;
	m_bMapData[63]=EXIT_TILE;

	for(int i=0;i<m_objBlocks.GetSize();i++) delete m_objBlocks[i];
	m_objBlocks.RemoveAll();

	m_sLevelName=_T("未命名关卡");
}

int CLevel::HitTestForPlacingBlocks(int x,int y,int nEditingBlockIndex) const
{
	for(int i=0;i<m_objBlocks.GetSize();i++){
		if(i!=nEditingBlockIndex && ((const CPushableBlock*)(m_objBlocks[i]))->HitTest(x,y)) return i;
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

int CLevel::GetBlockSize(int nWidth,int nHeight) const
{
	int i,nBlockSize;

	if(m_nWidth<=0 || m_nHeight<=0) return 32;

	nBlockSize=nWidth/m_nWidth;
	i=nHeight/m_nHeight;
	if(nBlockSize>i) nBlockSize=i;
	if(nBlockSize<8) nBlockSize=8;

	return nBlockSize;
}

RECT CLevel::GetDrawArea(RECT r,int nBlockSize) const
{
	if(nBlockSize<=0) nBlockSize=GetBlockSize(r.right-r.left,r.bottom-r.top);

	RECT r1;
	r1.left=(r.left+r.right-m_nWidth*nBlockSize)/2;
	r1.top=(r.top+r.bottom-m_nHeight*nBlockSize)/2;
	r1.right=r1.left+m_nWidth*nBlockSize;
	r1.bottom=r1.top+m_nHeight*nBlockSize;
	return r1;
}

#define DRAW_PLAYER(BRUSH) \
{ \
	pDC->SelectObject(BRUSH); \
	if(nBlockSize>=14){ \
		pDC->Ellipse(x0+nEdgeSize,y0+nEdgeSize,x0+nBlockSize-nEdgeSize,y0+nBlockSize-nEdgeSize); \
		pDC->SelectObject(&hbrBlack); \
		int yy1=y0+(nBlockSize-nEdgeSize*3)/2; \
		int yy2=y0+(nBlockSize-nEdgeSize)/2; \
		pDC->Ellipse(x0+(nBlockSize-nEdgeSize*3)/2,yy1,x0+(nBlockSize-nEdgeSize)/2+1,yy2); \
		pDC->Ellipse(x0+(nBlockSize+nEdgeSize)/2,yy1,x0+(nBlockSize+nEdgeSize*3)/2+1,yy2); \
		yy1=y0+nBlockSize/2+nEdgeSize; \
		pDC->MoveTo(x0+nBlockSize/2-nEdgeSize,yy1); \
		pDC->LineTo(x0+nBlockSize/2+nEdgeSize+1,yy1); \
	}else{ \
		pDC->Ellipse(x0+1,y0+1,x0+nBlockSize-1,y0+nBlockSize-1); \
	} \
}

void CLevel::Draw(CDC *pDC,RECT r,bool bEditMode,bool bCentering,int nBlockSize,int nEditingBlockIndex,bool bDrawGrid) const
{
	int i,j;
	int nEdgeSize;

	if(m_nWidth<=0 || m_nHeight<=0) return;

	if(!bEditMode) nEditingBlockIndex=-1;

	//calculate size per block
	if(nBlockSize<=0) nBlockSize=GetBlockSize(r.right-r.left,r.bottom-r.top);
	nEdgeSize=nBlockSize/8;
	if(nEdgeSize<2) nEdgeSize=2;

	if(bCentering) r=GetDrawArea(r,nBlockSize);

	//draw background
	{
		RECT r1={r.left,r.top,r.left+nBlockSize*m_nWidth,r.top+nBlockSize*m_nHeight};
		CBrush hbr(FLOOR_COLOR);
		pDC->FillRect(&r1,&hbr);
	}

	//draw blocks
	CBrush hbrBlock(BLOCK_COLOR),hbrBlockLight(BLOCK_LIGHT_COLOR),hbrBlockDark(BLOCK_DARK_COLOR);
	CBrush hbrBlack((COLORREF)0x0);
	CBrush hbrTarget(TARGET_COLOR);
	CBrush hbrExit(0x808080);
	CBrush hbrPlayer(0x00FFFF),hbrPlayerActivated(0x0080FF);

	//draw grid
	if(bDrawGrid){
		CPen hpn(0,1,(COLORREF)0x666666);
		pDC->SelectObject(&hpn);
		for(j=0;j<m_nHeight;j++){
			for(i=0;i<m_nWidth;i++){
				if(m_bMapData[j*m_nWidth+i]==EMPTY_TILE){
					int x0=r.left+i*nBlockSize;
					int y0=r.top+j*nBlockSize;
					RECT r1={x0,y0,x0+nBlockSize,y0+nBlockSize};
					pDC->FillRect(&r1,&hbrBlack);
				}
			}
		}
		for(int i=0;i<=m_nWidth;i++){
			pDC->MoveTo(r.left+nBlockSize*i,r.top);
			pDC->LineTo(r.left+nBlockSize*i,r.top+nBlockSize*m_nHeight);
		}
		for(int i=0;i<=m_nHeight;i++){
			pDC->MoveTo(r.left,r.top+nBlockSize*i);
			pDC->LineTo(r.left+nBlockSize*m_nWidth,r.top+nBlockSize*i);
		}
	}

	CPen hpn(0,1,(COLORREF)0x0);
	pDC->SelectObject(&hpn);

	for(j=0;j<m_nHeight;j++){
		for(i=0;i<m_nWidth;i++){
			switch(m_bMapData[j*m_nWidth+i]){
			case WALL_TILE:
				{
					bool bNeighborhood[8]={
						i>0 && j>0 && m_bMapData[(j-1)*m_nWidth+(i-1)]==WALL_TILE,
						j>0 && m_bMapData[(j-1)*m_nWidth+i]==WALL_TILE,
						i<m_nWidth-1 && j>0 && m_bMapData[(j-1)*m_nWidth+(i+1)]==WALL_TILE,
						i>0 && m_bMapData[j*m_nWidth+(i-1)]==WALL_TILE,
						i<m_nWidth-1 && m_bMapData[j*m_nWidth+(i+1)]==WALL_TILE,
						i>0 && j<m_nHeight-1 && m_bMapData[(j+1)*m_nWidth+(i-1)]==WALL_TILE,
						j<m_nHeight-1 && m_bMapData[(j+1)*m_nWidth+i]==WALL_TILE,
						i<m_nWidth-1 && j<m_nHeight-1 && m_bMapData[(j+1)*m_nWidth+(i+1)]==WALL_TILE
					};
					int x0=r.left+i*nBlockSize;
					int y0=r.top+j*nBlockSize;
					RECT r1={x0,y0,x0+nBlockSize,y0+nBlockSize};
					DrawEdgedBlock(pDC,r1,nEdgeSize,&hbrBlock,&hbrBlockLight,&hbrBlockDark,bNeighborhood);
				}
				break;
			case EMPTY_TILE:
				if(!bDrawGrid){
					int x0=r.left+i*nBlockSize;
					int y0=r.top+j*nBlockSize;
					RECT r1={x0,y0,x0+nBlockSize,y0+nBlockSize};
					pDC->FillRect(&r1,&hbrBlack);
				}
				break;
			case EXIT_TILE:
				{
					int x0=r.left+i*nBlockSize;
					int y0=r.top+j*nBlockSize;
					float radius=float(nBlockSize)/2.0f-float(nEdgeSize);
					POINT pt[19];
					for(int k=0;k<=16;k++){
						float a=3.14159265f/16.0f*float(k);
						pt[k].x=int(floor(float(x0)+float(nBlockSize)/2.0f-radius*cos(a)+0.5f));
						pt[k].y=int(floor(float(y0)+float(nBlockSize)/2.0f-radius*sin(a)+0.5f));
					}
					pt[17].x=pt[16].x;
					pt[18].x=pt[0].x;
					pt[17].y=pt[18].y=y0+nBlockSize-nEdgeSize;

					if(!bEditMode && m_nTargetFinished==m_nTargetCount){
						pDC->SelectObject(&hbrBlack);
						pDC->Polygon(pt,19);
					}else{
						pDC->SelectObject(&hbrExit);
						pDC->Polygon(pt,19);

						pt[9].x=pt[8].x;
						pt[9].y=pt[18].y;
						pDC->Polyline(pt+8,2);

						if(nBlockSize>=12){
							pDC->SelectObject(&hbrBlack);
							pDC->Ellipse(pt[8].x-nEdgeSize*2,y0+nBlockSize/2,pt[8].x-nEdgeSize+1,y0+nBlockSize/2+nEdgeSize+1);
							pDC->Ellipse(pt[8].x+nEdgeSize,y0+nBlockSize/2,pt[8].x+nEdgeSize*2+1,y0+nBlockSize/2+nEdgeSize+1);
						}
					}
				}
				break;
			case PLAYER_TILE:
			case PLAYER_AND_TARGET_TILE: //edit mode only
				if(bEditMode){
					int x0=r.left+i*nBlockSize;
					int y0=r.top+j*nBlockSize;

					DRAW_PLAYER(hbrPlayer);
				}
				break;
			default:
				break;
			}
		}
	}

	//boxes and animation
	for(i=0;i<m_objBlocks.GetSize();i++){
		if(i!=nEditingBlockIndex){
			CPushableBlock *objBlock=(CPushableBlock*)(m_objBlocks[i]);
			if(!bEditMode && i==m_nBlockAnimationIndex){
				if(objBlock->m_nType==ROTATE_BLOCK){
					objBlock->Draw(pDC,r,nBlockSize,nEdgeSize,m_nBlockAnimationFlags?16-m_nMoveAnimationTime:m_nMoveAnimationTime);
				}else{
					RECT r1={
						r.left-(m_nMoveAnimationX*nBlockSize*(8-m_nMoveAnimationTime))/8,
						r.top-(m_nMoveAnimationY*nBlockSize*(8-m_nMoveAnimationTime))/8,
						r.right,r.bottom
					};
					objBlock->Draw(pDC,r1,nBlockSize,nEdgeSize,0);
				}
			}else{
				objBlock->Draw(pDC,r,nBlockSize,nEdgeSize,0);
			}
		}
	}

	//draw overlay
	int nOverlaySize=nBlockSize/3;

	for(j=0;j<m_nHeight;j++){
		for(i=0;i<m_nWidth;i++){
			switch(m_bMapData[j*m_nWidth+i]){
			case EMPTY_TILE:
				{
					RECT r1={r.left+i*nBlockSize+nOverlaySize,r.top+j*nBlockSize+nOverlaySize,
						r.left+(i+1)*nBlockSize-nOverlaySize,r.top+(j+1)*nBlockSize-nOverlaySize};
					pDC->FillRect(&r1,&hbrBlack);
				}
				break;
			case TARGET_TILE:
			case PLAYER_AND_TARGET_TILE: //edit mode only
				{
					RECT r1={r.left+i*nBlockSize+nOverlaySize,r.top+j*nBlockSize+nOverlaySize,
						r.left+(i+1)*nBlockSize-nOverlaySize,r.top+(j+1)*nBlockSize-nOverlaySize};
					pDC->FillRect(&r1,&hbrTarget);
				}
				break;
			default:
				break;
			}
		}
	}

	//player and animation (in playing mode)
	if(!bEditMode){
		for(j=0;j<m_nHeight;j++){
			for(i=0;i<m_nWidth;i++){
				if(m_bMapData[j*m_nWidth+i]==PLAYER_TILE){
					bool bCurrent=(i==m_nPlayerX && j==m_nPlayerY);

					//check if we should draw animation instead
					if(bCurrent && (m_nMoveAnimationTime || m_nSwitchAnimationTime)) continue;

					int x0=r.left+i*nBlockSize;
					int y0=r.top+j*nBlockSize;

					DRAW_PLAYER(bCurrent?&hbrPlayerActivated:&hbrPlayer);
				}
			}
		}

		if(m_nPlayerX>=0 && m_nPlayerY>=0 && (m_nMoveAnimationTime || m_nSwitchAnimationTime)){
			int x0=r.left+m_nPlayerX*nBlockSize;
			int y0=r.top+m_nPlayerY*nBlockSize;

			if(m_nMoveAnimationTime>0){
				x0-=(m_nMoveAnimationX*nBlockSize*(8-m_nMoveAnimationTime))/8;
				y0-=(m_nMoveAnimationY*nBlockSize*(8-m_nMoveAnimationTime))/8;
			}

			if(m_nSwitchAnimationTime>0){
				i=m_nSwitchAnimationTime<4?m_nSwitchAnimationTime:8-m_nSwitchAnimationTime;
				y0-=nEdgeSize*i;
			}

			DRAW_PLAYER(&hbrPlayerActivated);
		}
	}

	//draw target overlay (playing mode)
	if(!bEditMode && m_bTargetData.GetSize()>=m_nWidth*m_nHeight){
		for(j=0;j<m_nHeight;j++){
			for(i=0;i<m_nWidth;i++){
				if(m_bTargetData[j*m_nWidth+i]){
					RECT r1={r.left+i*nBlockSize+nOverlaySize,r.top+j*nBlockSize+nOverlaySize,
						r.left+(i+1)*nBlockSize-nOverlaySize,r.top+(j+1)*nBlockSize-nOverlaySize};
					pDC->FillRect(&r1,&hbrTarget);
				}
			}
		}
	}

	//draw currently editing block
	if(nEditingBlockIndex>=0 && nEditingBlockIndex<m_objBlocks.GetSize()){
		//darken background
		{
			int nPattern[4]={0xAAAA5555,0xAAAA5555,0xAAAA5555,0xAAAA5555};
			CBitmap bm;
			bm.CreateBitmap(16,8,1,1,nPattern);

			CBrush hbrPattern(&bm),*hbrOld=pDC->SelectObject(&hbrPattern);
			CPen hpn0,*hpnOld;
			hpn0.CreateStockObject(NULL_PEN);
			hpnOld=pDC->SelectObject(&hpn0);

			pDC->SetROP2(R2_MASKPEN);
			pDC->Rectangle(r.left,r.top,r.right,r.bottom);
			pDC->SetROP2(R2_COPYPEN);

			pDC->SelectObject(hbrOld);
			pDC->SelectObject(hpnOld);

			hbrPattern.DeleteObject();
			bm.DeleteObject();
		}

		CPushableBlock *objBlock=(CPushableBlock*)(m_objBlocks[nEditingBlockIndex]);
		objBlock->Draw(pDC,r,nBlockSize,nEdgeSize,0);
	}
}

#define DRAW_EDGED_BLOCK_PART(FLIP_HORIZONTAL,FLIP_VERTICAL) \
if(b1){ \
	if(b2){ \
		if(!b0){ \
			hpnOld=pDC->SelectObject(&hpn0); \
			{ \
				POINT pt[]={{x0,y0},{x0,y1},{x1,y1}}; \
				pDC->SelectObject(FLIP_VERTICAL?hbrDark:hbrLight); \
				pDC->Polygon(pt,3); \
			} \
			{ \
				POINT pt[]={{x0,y0},{x1,y0},{x1,y1}}; \
				pDC->SelectObject(FLIP_HORIZONTAL?hbrDark:hbrLight); \
				pDC->Polygon(pt,3); \
			} \
			pDC->SelectObject(hpnOld); \
			POINT pt[]={ \
				{x0,y1},{x1,y1},{x1,y0-(y0<y2?1:-1)}, \
				{x0,y0},{x1,y1} \
			}; \
			DWORD m[]={3,2}; \
			pDC->PolyPolyline(pt,m,2); \
		} \
	}else{ \
		RECT r1={x0<x1?x0:x1,y0<y2?y0:y2,x0>x1?x0:x1,y0>y2?y0:y2}; \
		pDC->FillRect(&r1,FLIP_HORIZONTAL?hbrDark:hbrLight); \
		POINT pt[]={ \
			{x0,y0},{x0,y2+(y0<y2?1:-1)}, \
			{x1,y0},{x1,y2+(y0<y2?1:-1)} \
		}; \
		DWORD m[]={2,2}; \
		pDC->PolyPolyline(pt,m,2); \
	} \
}else{ \
	if(b2){ \
		RECT r1={x0<x2?x0:x2,y0<y1?y0:y1,x0>x2?x0:x2,y0>y1?y0:y1}; \
		pDC->FillRect(&r1,FLIP_VERTICAL?hbrDark:hbrLight); \
		POINT pt[]={ \
			{x0,y0},{x2+(x0<x2?1:-1),y0}, \
			{x0,y1},{x2+(x0<x2?1:-1),y1} \
		}; \
		DWORD m[]={2,2}; \
		pDC->PolyPolyline(pt,m,2); \
	}else{ \
		hpnOld=pDC->SelectObject(&hpn0); \
		{ \
			POINT pt[]={{x0,y0},{x1,y1},{x2,y1},{x2,y0}}; \
			pDC->SelectObject(FLIP_VERTICAL?hbrDark:hbrLight); \
			pDC->Polygon(pt,4); \
		} \
		{ \
			POINT pt[]={{x0,y0},{x1,y1},{x1,y2},{x0,y2}}; \
			pDC->SelectObject(FLIP_HORIZONTAL?hbrDark:hbrLight); \
			pDC->Polygon(pt,4); \
		} \
		pDC->SelectObject(hpnOld); \
		POINT pt[]={ \
			{x0,y2},{x0,y0},{x2+(x0<x2?1:-1),y0}, \
			{x1,y2},{x1,y1},{x2+(x0<x2?1:-1),y1}, \
			{x0,y0},{x1,y1} \
		}; \
		DWORD m[]={3,3,2}; \
		pDC->PolyPolyline(pt,m,3); \
	} \
}

void CLevel::DrawEdgedBlock(CDC *pDC,RECT r,int nEdgeSize,CBrush *hbr,CBrush *hbrLight,CBrush *hbrDark,const bool bNeighborhood[8])
{
	int x0,x1,x2=(r.left+r.right)/2;
	int y0,y1,y2=(r.top+r.bottom)/2;

	CPen hpn0,*hpnOld;
	hpn0.CreateStockObject(NULL_PEN);

	bool b0,b1,b2;

	//TEST ONLY
	pDC->FillRect(&r,hbr);

	//draw top-left corner
	x0=r.left;
	x1=r.left+nEdgeSize;
	y0=r.top;
	y1=r.top+nEdgeSize;
	b0=bNeighborhood[0];
	b1=bNeighborhood[1];
	b2=bNeighborhood[3];
	DRAW_EDGED_BLOCK_PART(false,false);

	//draw top-right corner
	x0=r.right;
	x1=r.right-nEdgeSize;
	b0=bNeighborhood[2];
	b2=bNeighborhood[4];
	DRAW_EDGED_BLOCK_PART(true,false);

	//draw bottom-right corner
	y0=r.bottom;
	y1=r.bottom-nEdgeSize;
	b0=bNeighborhood[7];
	b1=bNeighborhood[6];
	DRAW_EDGED_BLOCK_PART(true,true);

	//draw bottom-left corner
	x0=r.left;
	x1=r.left+nEdgeSize;
	b0=bNeighborhood[5];
	b2=bNeighborhood[3];
	DRAW_EDGED_BLOCK_PART(false,true);
}

static void DoMapDataRLE(const CArray<unsigned char>& src,CArray<unsigned char>& dst){
	dst.RemoveAll();
	if(src.IsEmpty()) return;

	int lastIndex=0;
	int m=src.GetSize();
	unsigned char lastValue=src[0];

	for(int i=1;i<=m;i++){
		int count=i-lastIndex;
		if(count>=259 || i>=m || src[i]!=lastValue){
			if(count>=4){
				dst.Add(RLE_TILE);
				dst.Add(count-4);
				dst.Add(lastValue);
			}else{
				do dst.Add(lastValue);
				while((--count)>0);
			}

			if(i>=m) break;

			lastValue=src[i];
			lastIndex=i;
		}
	}
}

static bool DoMapDataUnRLE(const CArray<unsigned char>& src,CArray<unsigned char>& dst){
	dst.RemoveAll();
	if(src.IsEmpty()) return true;

	int m=src.GetSize();
	for(int i=0;i<m;i++){
		unsigned char c=src[i];
		if(c==RLE_TILE){
			if(i+2>=m) return false;
			int count=src[i+1]+4;
			c=src[i+2];
			do dst.Add(c);
			while((--count)>0);
			i+=2;
		}else{
			dst.Add(c);
		}
	}

	return true;
}

void CLevel::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// 在此添加存储代码
		ar<<m_sLevelName<<m_nWidth<<m_nHeight;

		CArray<unsigned char> filteredMapData;
		CObArray filteredBlocks;

		filteredMapData.Copy(m_bMapData);
		for(int i=0;i<m_objBlocks.GetSize();i++){
			CPushableBlock* objBlock=(CPushableBlock*)m_objBlocks[i];
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

			if(!bFiltered) filteredBlocks.Add(objBlock);
		}

		CArray<unsigned char> d;
		DoMapDataRLE(filteredMapData,d);
		d.Serialize(ar);
		filteredBlocks.Serialize(ar);
	}
	else
	{
		for(int i=0;i<m_objBlocks.GetSize();i++) delete m_objBlocks[i];
		m_objBlocks.RemoveAll();
		m_nMoves=0;

		//check version
		if(ar.GetObjectSchema()>2){
			AfxThrowArchiveException(CArchiveException::badSchema);
		}

		ar>>m_sLevelName>>m_nWidth>>m_nHeight;

		CArray<unsigned char> d;
		d.Serialize(ar);
		if(!DoMapDataUnRLE(d,m_bMapData) || m_bMapData.GetSize()!=m_nWidth*m_nHeight){
			AfxThrowArchiveException(CArchiveException::badSchema);
		}

		m_objBlocks.Serialize(ar);

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
						CPushableBlock* objBlock=new CPushableBlock;
						objBlock->CreateSingle((md==NORMAL_BLOCK_TILE)?NORMAL_BLOCK:TARGET_BLOCK,x,y);
						m_objBlocks.Add(objBlock);
					}
					m_bMapData[idx]=(md==TARGET_BLOCK_ON_TARGET_TILE)?TARGET_TILE:0;
					break;
				}
			}
		}
	}
}

struct PushableBlockAndIndex{
	int index;
	CPushableBlock* block;
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

void CLevel::MySerialize(MySerializer& ar){
	ar.PutVUInt32(m_nWidth);
	ar.PutVUInt32(m_nHeight);

	assert(m_nWidth*m_nHeight==m_bMapData.GetSize());
	for(int i=0;i<m_bMapData.GetSize();i++){
		ar.PutIntN(m_bMapData[i],4);
	}
	ar.PutFlush();

	ar.PutVUInt32(m_objBlocks.GetSize());

	int m=m_objBlocks.GetSize();
	if(m>0){
		PushableBlockAndIndex* arr=(PushableBlockAndIndex*)malloc(m*sizeof(PushableBlockAndIndex));
		for(int i=0;i<m;i++){
			PushableBlockAndIndex t={i,(CPushableBlock*)(m_objBlocks[i])};
			arr[i]=t;
		}
		qsort(arr,m,sizeof(PushableBlockAndIndex),PushableBlockAndIndexCompare);
		for(int i=0;i<m;i++){
			arr[i].block->MySerialize(ar);
		}
		free(arr);
	}
}

bool CLevel::Undo(){
	if(IsAnimating()) return true;

	if(CanUndo()){
		m_objCurrentUndo=NULL;
		CLevelUndo *objUndo=(CLevelUndo*)(m_objUndo[m_nCurrentUndo-1]);

		switch(objUndo->m_nType){
		case 0:
			if(SwitchPlayer(objUndo->m_x,objUndo->m_y,false)){
				m_nCurrentUndo--;
				return true;
			}
			break;
		case 1:
			{
				//reset player position
				int x=objUndo->m_x,y=objUndo->m_y;
				int dx=objUndo->m_dx,dy=objUndo->m_dy;
				int idx=(y+dy)*m_nWidth+(x+dx);
				if(m_bMapData[idx]==PLAYER_TILE) m_bMapData[idx]=FLOOR_TILE;
				idx=y*m_nWidth+x;
				m_bMapData[idx]=PLAYER_TILE;
				m_nPlayerX=x;
				m_nPlayerY=y;

				//play fake animation
				m_nMoveAnimationX=-dx;
				m_nMoveAnimationY=-dy;
				m_nMoveAnimationTime=GetAnimationTime();
				m_nBlockAnimationIndex=-1;

				idx=objUndo->m_nMovedBlockIndex;
				if(idx>=0){
					//restore deleted blocks
					CPushableBlock *objBlock=objUndo->m_objDeletedBlock;
					if(objBlock){
						m_objBlocks.InsertAt(idx,new CPushableBlock(*objBlock));

						//reset ground
						if(objBlock->m_nType==NORMAL_BLOCK){
							for(int j=0;j<objBlock->m_h;j++){
								for(int i=0;i<objBlock->m_w;i++){
									int x=i+objBlock->m_x,y=j+objBlock->m_y;
									if((*objBlock)(i,j) && x>=0 && y>=0 && x<m_nWidth && y<m_nHeight){
										m_bMapData[y*m_nWidth+x]=EMPTY_TILE;
									}
								}
							}
						}
					}

					//reset block movement
					objBlock=(CPushableBlock*)(m_objBlocks[idx]);
					if(objBlock->m_nType==ROTATE_BLOCK){
						m_nBlockAnimationFlags=(x-objBlock->m_x)*dy-(y-objBlock->m_y)*dx<0;
						objBlock->Rotate(m_nBlockAnimationFlags);
					}else{
						objBlock->m_x-=dx;
						objBlock->m_y-=dy;
					}

					//play fake animation
					m_nBlockAnimationIndex=idx;
				}

				//over
				UpdateMoveableData();
				m_nCurrentUndo--;
				m_nMoves--;
				return true;
			}
			break;
		default:
			break;
		}
	}

	return false;
}

bool CLevel::Redo(){
	if(IsAnimating()) return true;

	if(CanRedo()){
		m_objCurrentUndo=NULL;
		CLevelUndo *objUndo=(CLevelUndo*)(m_objUndo[m_nCurrentUndo]);

		switch(objUndo->m_nType){
		case 0:
			if(SwitchPlayer(objUndo->m_dx,objUndo->m_dy,false)){
				m_nCurrentUndo++;
				return true;
			}
			break;
		case 1:
			{
				int dx=objUndo->m_dx;
				int dy=objUndo->m_dy;
				dx=(dx>0)?1:(dx<0)?-1:0;
				dy=(dy>0)?1:(dy<0)?-1:0;
				if(MovePlayer(dx,dy,false)){
					m_nCurrentUndo++;
					return true;
				}
			}
			break;
		default:
			break;
		}
	}

	return false;
}

void CLevel::StartGame()
{
	int i,j;

	m_nMoves=0;
	m_nPlayerCount=0;
	m_nExitCount=0;
	m_nPlayerX=-1;
	m_nPlayerY=-1;
	m_nMoveAnimationX=0;
	m_nMoveAnimationY=0;
	m_nMoveAnimationTime=0;
	m_nBlockAnimationIndex=-1;
	m_nSwitchAnimationTime=0;
	m_nTargetCount=0;
	m_nTargetFinished=0;

	//reset undo data
	for(i=0;i<m_objUndo.GetSize();i++) delete m_objUndo[i];
	m_objUndo.RemoveAll();
	m_nCurrentUndo=0;
	m_objCurrentUndo=NULL;

	//find player and init target data
	m_bTargetData.SetSize(m_nWidth*m_nHeight);
	for(j=0;j<m_nHeight;j++){
		for(i=0;i<m_nWidth;i++){
			int idx=j*m_nWidth+i;
			m_bTargetData[idx]=0;
			switch(m_bMapData[idx]){
			case PLAYER_TILE:
				if((m_nPlayerCount++)==0){
					m_nPlayerX=i;
					m_nPlayerY=j;
				}
				break;
			case PLAYER_AND_TARGET_TILE: //edit mode only
				if((m_nPlayerCount++)==0){
					m_nPlayerX=i;
					m_nPlayerY=j;
				}
				m_bMapData[idx]=PLAYER_TILE;
				m_bTargetData[idx]=1;
				m_nTargetCount++;
				break;
			case TARGET_TILE:
				m_bMapData[idx]=0;
				m_bTargetData[idx]=1;
				m_nTargetCount++;
				break;
			case EXIT_TILE:
				m_nExitCount++;
				break;
			default:
				break;
			}
		}
	}

	//init moveable data
	m_bMoveableData.SetSize(m_nWidth*m_nHeight);
	UpdateMoveableData();
}

bool CLevel::IsAnimating() const
{
	return m_nMoveAnimationTime || m_nSwitchAnimationTime;
}

bool CLevel::IsWin() const
{
	return m_nPlayerCount==0 || (m_nExitCount==0 && m_nTargetCount>0 && m_nTargetCount==m_nTargetFinished);
}

void CLevel::SaveUndo(CLevelUndo* objUndo){
	//delete existing outdated undo data
	if(m_nCurrentUndo<m_objUndo.GetSize()){
		for(int i=m_nCurrentUndo;i<m_objUndo.GetSize();i++){
			delete m_objUndo[i];
		}
		m_objUndo.SetSize(m_nCurrentUndo);
	}

	//add new undo data
	m_objUndo.Add(objUndo);
	m_objCurrentUndo=objUndo;
	m_nCurrentUndo=m_objUndo.GetSize();
}

bool CLevel::SwitchPlayer(int x,int y,bool bSaveUndo)
{
	m_objCurrentUndo=NULL;

	if(m_nWidth<=0 || m_nHeight<=0) return false;

	if(x>=0 && y>=0 && x<m_nWidth && y<m_nHeight){
		if(m_bMapData[y*m_nWidth+x]==PLAYER_TILE){
			if(x==m_nPlayerX && y==m_nPlayerY) return false;

			//save undo
			if(bSaveUndo){
				CLevelUndo* objUndo=new CLevelUndo;
				objUndo->m_x=m_nPlayerX;
				objUndo->m_y=m_nPlayerY;
				objUndo->m_dx=x;
				objUndo->m_dy=y;
				SaveUndo(objUndo);
			}

			m_nPlayerX=x;
			m_nPlayerY=y;
			m_nSwitchAnimationTime=GetAnimationTime();
			return true;
		}
		return false;
	}

	int i,j=-1,m=m_nWidth*m_nHeight;
	x=m_nPlayerX;
	y=m_nPlayerY;
	if(x>=0 && y>=0 && x<m_nWidth && y<m_nHeight) j=y*m_nWidth+x;

	for(i=0;i<m;i++){
		j++;
		if(j>=m) j-=m;
		if(m_bMapData[j]==PLAYER_TILE){
			x=j%m_nWidth;
			y=j/m_nWidth;

			if(x==m_nPlayerX && y==m_nPlayerY) return false;

			//save undo
			if(bSaveUndo){
				CLevelUndo* objUndo=new CLevelUndo;
				objUndo->m_x=m_nPlayerX;
				objUndo->m_y=m_nPlayerY;
				objUndo->m_dx=x;
				objUndo->m_dy=y;
				SaveUndo(objUndo);
			}

			m_nPlayerX=x;
			m_nPlayerY=y;
			m_nSwitchAnimationTime=GetAnimationTime();
			return true;
		}
	}

	return false;
}

bool CLevel::MovePlayer(int dx,int dy,bool bSaveUndo){
	m_objCurrentUndo=NULL;

	int x=m_nPlayerX;
	int y=m_nPlayerY;
	int x1=x+dx;
	int y1=y+dy;
	bool ret=false;
	bool bMoveTwice=false;

	if(x>=0 && y>=0 && x<m_nWidth && y<m_nHeight
		&& x1>=0 && y1>=0 && x1<m_nWidth && y1<m_nHeight)
	{
		int idx=y*m_nWidth+x;
		int idx1=y1*m_nWidth+x1;
		switch(m_bMoveableData[idx1]){
		case 0:
			{
				//now chcek pushable blocks
				bool bHitTest=false;
				m_nBlockAnimationIndex=-1;
				for(int i=0;i<m_objBlocks.GetSize();i++){
					CPushableBlock *objBlock=(CPushableBlock*)(m_objBlocks[i]);
					if(objBlock->HitTest(x1,y1)){
						if(objBlock->m_nType==ROTATE_BLOCK){
							//check the rotation direction; calculate the cross product
							int dir=(x1-objBlock->m_x)*dy-(y1-objBlock->m_y)*dx;
							if(dir==0){
								bHitTest=true;
								break;
							}

							//check if there are no obstacles
							int nRadiusSquare[4]={
								objBlock->m_bData[dir>0?1:0], //top-left
								objBlock->m_bData[dir>0?2:1], //bottom-left
								objBlock->m_bData[dir>0?3:2], //bottom-right
								objBlock->m_bData[dir>0?0:3], //top-right
							};
							int m=0;
							for(int k=0;k<4;k++){
								if(m<nRadiusSquare[k]) m=nRadiusSquare[k];
								nRadiusSquare[k]*=(nRadiusSquare[k]+2); // (r+1)^2-1
							}
							for(int yy=-m;yy<=m;yy++){
								for(int xx=-m;xx<=m;xx++){
									if((xx<=0 && yy<=0 && xx*xx+yy*yy<=nRadiusSquare[0])
										|| (xx<=0 && yy>=0 && xx*xx+yy*yy<=nRadiusSquare[1])
										|| (xx>=0 && yy>=0 && xx*xx+yy*yy<=nRadiusSquare[2])
										|| (xx>=0 && yy<=0 && xx*xx+yy*yy<=nRadiusSquare[3])
										)
									{
										int xxx=xx+objBlock->m_x;
										int yyy=yy+objBlock->m_y;
										if((xxx!=x || yyy!=y) && HitTestForPlacingBlocks(xxx,yyy,i)!=-1){
											bHitTest=true;
											break;
										}
									}
								}
								if(bHitTest) break;
							}
							if(bHitTest) break;

							//check if player should move twice
							if(x1==objBlock->m_x){
								if(y1<objBlock->m_y){
									bMoveTwice=objBlock->m_bData[dir>0?1:3]>=objBlock->m_y-y1;
								}else{
									bMoveTwice=objBlock->m_bData[dir>0?3:1]>=y1-objBlock->m_y;
								}
							}else{
								if(x1<objBlock->m_x){
									bMoveTwice=objBlock->m_bData[dir>0?2:0]>=objBlock->m_x-x1;
								}else{
									bMoveTwice=objBlock->m_bData[dir>0?0:2]>=x1-objBlock->m_x;
								}
							}
							if(bMoveTwice){
								int xx=x1+dx,yy=y1+dy;
								if(!(xx>=0 && yy>=0 && xx<m_nWidth && yy<m_nHeight
									&& m_bMoveableData[yy*m_nWidth+xx]==0))
								{
									bHitTest=true;
									break;
								}
							}

							//push it
							m_nBlockAnimationIndex=i;
							m_nBlockAnimationFlags=(dir>0);
						}else{
							//check if we can push it
							for(int yy=0;yy<objBlock->m_h;yy++){
								for(int xx=0;xx<objBlock->m_w;xx++){
									int xxx=xx+objBlock->m_x+dx;
									int yyy=yy+objBlock->m_y+dy;
									if((xxx!=x || yyy!=y) && (*objBlock)(xx,yy) && HitTestForPlacingBlocks(xxx,yyy,i)!=-1){
										bHitTest=true;
										break;
									}
								}
								if(bHitTest) break;
							}
							if(bHitTest) break;
							//push it
							m_nBlockAnimationIndex=i;
						}
					}
				}
				if(bHitTest){
					m_nBlockAnimationIndex=-1;
					break;
				}

				//---we can move

				//check if we should move twice
				if(bMoveTwice){
					x1+=dx;
					y1+=dy;
					dx+=dx;
					dy+=dy;
					idx1=y1*m_nWidth+x1;
				}

				//create undo object
				if(bSaveUndo){
					CLevelUndo *objUndo=new CLevelUndo;
					objUndo->m_nType=1;
					objUndo->m_x=x;
					objUndo->m_y=y;
					objUndo->m_dx=dx;
					objUndo->m_dy=dy;
					objUndo->m_nMovedBlockIndex=m_nBlockAnimationIndex;
					SaveUndo(objUndo);
				}

				m_bMapData[idx]=FLOOR_TILE;
				if(m_bMapData[idx1]!=EXIT_TILE) m_bMapData[idx1]=PLAYER_TILE;

				m_nPlayerX=x1;
				m_nPlayerY=y1;

				m_nMoveAnimationX=dx;
				m_nMoveAnimationY=dy;
				m_nMoveAnimationTime=GetAnimationTime();

				if(m_nBlockAnimationIndex>=0){
					CPushableBlock *objBlock=(CPushableBlock*)(m_objBlocks[m_nBlockAnimationIndex]);
					if(objBlock->m_nType==ROTATE_BLOCK){
						objBlock->Rotate(m_nBlockAnimationFlags);
					}else{
						objBlock->m_x+=dx;
						objBlock->m_y+=dy;
					}
				}

				m_nMoves++;

				ret=true;
			}
			break;
		case 1: //blocked
			break;
		default:
			break;
		}
	}

	if(ret) UpdateMoveableData();

	return ret;
}

bool CLevel::CheckIfCurrentPlayerCanMove(int dx,int dy,int x0,int y0){
	if(x0<0) x0=m_nPlayerX;
	if(y0<0) y0=m_nPlayerY;

	if(x0<0 || x0>=m_nWidth || y0<0 || y0>=m_nHeight) return false;

	//backup
	int idx=-1;
	int backup=-1;
	int oldX=m_nPlayerX;
	int oldY=m_nPlayerY;
	if(oldX>=0 && oldX<m_nWidth && oldY>=0 && oldY<m_nHeight){
		idx=oldY*m_nWidth+oldX;
		backup=m_bMapData[idx];
		m_bMapData[idx]=0;
	}
	int idx2=y0*m_nWidth+x0;
	int backup2=m_bMapData[idx2];
	m_bMapData[idx2]=PLAYER_TILE;
	m_nPlayerX=x0;
	m_nPlayerY=y0;
	UpdateMoveableData();

	//try to move
	bool ret=MovePlayer(dx,dy,true);
	while(IsAnimating()) OnTimer();
	if(ret) Undo();
	while(IsAnimating()) OnTimer();

	//restore
	m_nPlayerX=oldX;
	m_nPlayerY=oldY;
	m_bMapData[idx2]=backup2;
	if(idx>=0) m_bMapData[idx]=backup;
	UpdateMoveableData();

	//over
	return ret;
}

void CLevel::UpdateMoveableData(){
	// update target count
	m_nTargetFinished=0;
	for(int idx=0;idx<m_objBlocks.GetSize();idx++){
		CPushableBlock *objBlock=(CPushableBlock*)(m_objBlocks[idx]);
		if(objBlock->m_nType==TARGET_BLOCK){
			for(int j=0;j<objBlock->m_h;j++){
				for(int i=0;i<objBlock->m_w;i++){
					int x=objBlock->m_x+i,y=objBlock->m_y+j;
					if(x>=0 && y>=0 && x<m_nWidth && y<=m_nHeight
						&& (*objBlock)(i,j) && m_bTargetData[y*m_nWidth+x])
					{
						m_nTargetFinished++;
					}
				}
			}
		}
	}

	// update moveable data and player count again
	m_nPlayerCount=0;
	for(int i=0;i<m_bMapData.GetSize();i++){
		switch(m_bMapData[i]){
		case 0:
			m_bMoveableData[i]=0;
			break;
		case EXIT_TILE:
			m_bMoveableData[i]=(m_nTargetFinished==m_nTargetCount)?0:1;
			break;
		case PLAYER_TILE:
			m_nPlayerCount++;
			m_bMoveableData[i]=1;
			break;
		default:
			m_bMoveableData[i]=1;
			break;
		}
	}
}

void CLevel::OnTimer()
{
	int mask=GetAnimationTime();

	if(m_nMoveAnimationTime>0){
		m_nMoveAnimationTime=(m_nMoveAnimationTime+mask)&(-mask);
		if(m_nMoveAnimationTime>=8){
			m_nMoveAnimationTime=0;

			//check moving blocks
			if(m_nBlockAnimationIndex>=0){
				CPushableBlock *objBlock=(CPushableBlock*)(m_objBlocks[m_nBlockAnimationIndex]);
				if(objBlock->m_nType==NORMAL_BLOCK){
					bool bFill=true;

					for(int j=0;j<objBlock->m_h;j++){
						for(int i=0;i<objBlock->m_w;i++){
							int x=objBlock->m_x+i,y=objBlock->m_y+j;
							if(x>=0 && y>=0 && x<m_nWidth && y<=m_nHeight){
								if((*objBlock)(i,j) && m_bMapData[y*m_nWidth+x]!=EMPTY_TILE){
									bFill=false;
									break;
								}
							}else{
								bFill=false;
								break;
							}
						}
					}

					if(bFill){
						for(int j=0;j<objBlock->m_h;j++){
							for(int i=0;i<objBlock->m_w;i++){
								if((*objBlock)(i,j)){
									int x=objBlock->m_x+i,y=objBlock->m_y+j;
									m_bMapData[y*m_nWidth+x]=FLOOR_TILE;
								}
							}
						}

						//check if we should save a backup
						if(m_objCurrentUndo){
							delete m_objCurrentUndo->m_objDeletedBlock;
							m_objCurrentUndo->m_objDeletedBlock=objBlock;
						}else{
							delete objBlock;
						}

						m_objBlocks.RemoveAt(m_nBlockAnimationIndex);

						UpdateMoveableData();
					}
				}
			}
			m_nBlockAnimationIndex=-1;

			//check if player comes to the exit
			int x=m_nPlayerX;
			int y=m_nPlayerY;
			if(!(x>=0 && y>=0 && x<m_nWidth && y<m_nHeight && m_bMapData[y*m_nWidth+x]==PLAYER_TILE)){
				if(m_nPlayerCount>0){
					SwitchPlayer(-1,-1,false);
					return;
				}
			}
		}
	}
	if(m_nSwitchAnimationTime>0){
		m_nSwitchAnimationTime=(m_nSwitchAnimationTime+mask)&(-mask);
		if(m_nSwitchAnimationTime>=8) m_nSwitchAnimationTime=0;
	}
}

CString CLevel::GetRecord()
{
	CString s;

	for(int i=0;i<m_nCurrentUndo;i++){
		CLevelUndo *obj=(CLevelUndo*)m_objUndo[i];

		switch(obj->m_nType){
		case 0:
			s.AppendFormat(_T("(%d,%d)"),obj->m_dx+1,obj->m_dy+1);
			break;
		case 1:
			s.AppendChar(obj->m_dx<0?'A':obj->m_dy<0?'W':obj->m_dx>0?'D':'S');
			break;
		default:
			break;
		}
	}

	return s;
}

bool CLevel::ApplyRecord(const CString& rec)
{
	for(int i=0,m=rec.GetLength();i<m;i++){
		switch(rec[i]){
		case 'A':
		case 'a':
			MovePlayer(-1,0,true);
			break;
		case 'W':
		case 'w':
			MovePlayer(0,-1,true);
			break;
		case 'D':
		case 'd':
			MovePlayer(1,0,true);
			break;
		case 'S':
		case 's':
			MovePlayer(0,1,true);
			break;
		case '(':
			{
				int x=0,y=0;
				for(i++;i<m;i++){
					int ch=rec[i];
					if(ch>='0' && ch<='9'){
						x=x*10+ch-'0';
					}else if(ch==','){
						break;
					}
				}
				if(i>=m || x<=0 || x>m_nWidth) return false;
				for(i++;i<m;i++){
					int ch=rec[i];
					if(ch>='0' && ch<='9'){
						y=y*10+ch-'0';
					}else if(ch==')'){
						break;
					}
				}
				if(i>=m || y<=0 || y>m_nHeight) return false;
				SwitchPlayer(x-1,y-1,true);
			}
			break;
		default:
			break;
		}

		while(IsAnimating()) OnTimer();

		if(IsWin()) break;
	}

	return true;
}
