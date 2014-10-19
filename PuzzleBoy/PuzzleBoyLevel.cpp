#include "PuzzleBoyLevel.h"
#include "PushableBlock.h"
#include "PuzzleBoyLevelUndo.h"
#include "PuzzleBoyApp.h"
#include "VertexList.h"
#include "PuzzleBoyLevelView.h"
#include "main.h"
#include "RecordManager.h"
#include "NetworkManager.h"

#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "include_gl.h"

//----------------

/*#define FLOOR_COLOR 0xFFC0C0

#define BLOCK_COLOR 0x0040FF
#define BLOCK_LIGHT_COLOR 0x0080FF
#define BLOCK_DARK_COLOR 0x0000FF

#define PUSH_BLOCK_COLOR 0x00FFFF
#define PUSH_BLOCK_LIGHT_COLOR 0x80FFFF
#define PUSH_BLOCK_DARK_COLOR 0x0080FF

#define ROTATE_BLOCK_COLOR 0xFFFF00

#define TARGET_BLOCK_COLOR 0x00FF00
#define TARGET_BLOCK_LIGHT_COLOR 0x80FF80
#define TARGET_BLOCK_DARK_COLOR 0x00C000

#define TARGET_COLOR 0x008000*/

#define FLOOR_COLOR 0.75f,0.75f,1.0f,1.0f

#define GRID_COLOR 0.4f,0.4f,0.4f,1.0f

#define BLOCK_COLOR 1.0f,0.25f,0.0f,1.0f
const float BLOCK_COLOR_mat[4]={1.0f,0.5f,0.0f,1.0f};

const float PUSH_BLOCK_COLOR_mat[4]={1.0f,1.0f,0.5f,1.0f};
const float PUSH_BLOCK_COLOR_trans_mat[4]={1.0f,1.0f,0.5f,0.5f};

#define ROTATE_BLOCK_COLOR 0.0f,1.0f,1.0f,1.0f
const float ROTATE_BLOCK_COLOR_mat[4]={0.5f,1.0f,1.0f,1.0f};

const float TARGET_BLOCK_COLOR_mat[4]={0.5f,1.0f,0.5f,1.0f};
const float TARGET_BLOCK_COLOR_trans_mat[4]={0.5f,1.0f,0.5f,0.5f};

#define TARGET_COLOR_OVERLAY 0.0f,0.5f,0.0f,0.75f

#define EMPTY_COLOR 0.0f,0.0f,0.0f,1.0f
#define EMPTY_COLOR_OVERLAY 0.0f,0.0f,0.0f,1.0f

#define LINE_COLOR 0.0f,0.0f,0.0f,1.0f

const float ROTATE_BLOCK_CENTER_mat[4]={0.5f,0.5f,0.5f,1.0f};
const float PLAYER_mat[4]={1.0f,1.0f,0.0f,1.0f};
const float PLAYER_ACTIVATED_mat[4]={1.0f,0.5f,0.0f,1.0f};

struct PuzzleBoyLevelGraphics{
	VertexList bg,wall,empty;

	VertexList target_overlay,empty_overlay;

	VertexList grid;

	VertexList wall_shade;

	VertexList wall_line;

	VertexList rotate_block_center;

	VertexList player,player_line;

	VertexList exit,exit_left,exit_right,exit_line;

	PuzzleBoyLevelGraphics()
		:bg(false,false)
		,wall(false,true)
		,empty(false,true)
		,target_overlay(false,true)
		,empty_overlay(false,true)
		,grid(false,false,VertexList::Lines)
		,wall_shade(true,true)
		,wall_line(false,true,VertexList::Lines)
		,rotate_block_center(true,true)
		,player(true,true)
		,player_line(false,true,VertexList::Lines)
		,exit(false,true)
		,exit_left(true,true)
		,exit_right(true,true)
		,exit_line(false,true,VertexList::Lines)
	{
	}
};

inline int GetAnimationTime(){
	return 1<<theApp->m_nAnimationSpeed;
}

// PuzzleBoyLevel

PuzzleBoyLevel::PuzzleBoyLevel()
: m_bSendNetworkMove(false)
, m_nPlayerX(-1)
, m_nPlayerY(-1)
, m_objCurrentUndo(NULL)
, m_nMoves(0)
, m_Graphics(NULL)
, m_view(NULL)
{
}

PuzzleBoyLevel::PuzzleBoyLevel(const PuzzleBoyLevelData& obj)
: PuzzleBoyLevelData(obj)
, m_bSendNetworkMove(false)
, m_nPlayerX(-1)
, m_nPlayerY(-1)
, m_objCurrentUndo(NULL)
, m_nMoves(0)
, m_Graphics(NULL)
, m_view(NULL)
{
}


PuzzleBoyLevel::~PuzzleBoyLevel()
{
	for(unsigned int i=0;i<m_objUndo.size();i++) delete m_objUndo[i];
	delete m_Graphics;
}

void PuzzleBoyLevel::CreateRectangles(int nLeft,int nTop,int nWidth,int nHeight,const unsigned char* bData,int nValue,VertexList *v){
	for(int j=0;j<nHeight;j++){
		int l=-1,r=-1;
		for(int i=0;i<nWidth;i++){
			int idx=j*nWidth+i;
			int a=bData[idx];
			if(a==nValue || (nValue<0 && a)){
				if(l<0) l=i;
				r=i+1;
			}else{
				if(l>=0) v->AddRectangle(float(nLeft+l),float(nTop+j),float(nLeft+r),float(nTop+j+1));
				l=-1;
			}
		}
		if(l>=0) v->AddRectangle(float(nLeft+l),float(nTop+j),float(nLeft+r),float(nTop+j+1));
	}
}

void PuzzleBoyLevel::CreateBorder(int nLeft,int nTop,int nWidth,int nHeight,const unsigned char* bData,int nValue,VertexList *shade,VertexList *lines,float fSize){
	//horizontal
	for(int j=0;j<nHeight;j++){
		int prev1=0,prev2=0;
		int start1=-1,start2=-1;
		int start1b,start2b;
		for(int i=0;i<=nWidth;i++){
			int a=0,a1=0,a2=0;
			if(i<nWidth){
				int idx=j*nWidth+i;

				a=bData[idx];
				a=(a==nValue || (nValue<0 && a))?2:0;
				if(j>0){
					a1=bData[idx-nWidth];
					a1=(a1==nValue || (nValue<0 && a1))?1:0;
				}
				if(j<nHeight-1){
					a2=bData[idx+nWidth];
					a2=(a2==nValue || (nValue<0 && a2))?1:0;
				}
			}

			a1|=a;a2|=a;

			//check top
			if(a1==2){
				if(start1<0){
					start1=i;
					start1b=(prev1==3)?-1:1;
				}
			}else if(start1>=0){
				int end1b=(a1==3)?1:-1;

				float p[8]={
					float(nLeft+start1),float(nTop+j),
					float(nLeft+i),float(nTop+j),
					float(nLeft+i)+end1b*fSize,float(nTop+j)+fSize,
					float(nLeft+start1)+start1b*fSize,float(nTop+j)+fSize,
				};

				float n[3]={
					0,-SQRT_1_2,SQRT_1_2,
				};

				shade->AddPrimitives(VertexList::Quads,p,n,4);
				if(theApp->m_bShowLines && lines){
					lines->AddPrimitives(VertexList::LineLoop,p,NULL,4);
				}

				start1=-1;
			}
			prev1=a1;

			//check bottom
			if(a2==2){
				if(start2<0){
					start2=i;
					start2b=(prev2==3)?-1:1;
				}
			}else if(start2>=0){
				int end2b=(a2==3)?1:-1;

				float p[8]={
					float(nLeft+start2),float(nTop+j+1),
					float(nLeft+i),float(nTop+j+1),
					float(nLeft+i)+end2b*fSize,float(nTop+j+1)-fSize,
					float(nLeft+start2)+start2b*fSize,float(nTop+j+1)-fSize,
				};

				float n[3]={
					0,SQRT_1_2,SQRT_1_2,
				};

				shade->AddPrimitives(VertexList::Quads,p,n,4);
				if(theApp->m_bShowLines && lines){
					lines->AddPrimitives(VertexList::LineLoop,p,NULL,4);
				}

				start2=-1;
			}
			prev2=a2;
		}
	}

	//vertical
	for(int i=0;i<nWidth;i++){
		int prev1=0,prev2=0;
		int start1=-1,start2=-1;
		int start1b,start2b;
		for(int j=0;j<=nHeight;j++){
			int a=0,a1=0,a2=0;
			if(j<nHeight){
				int idx=j*nWidth+i;

				a=bData[idx];
				a=(a==nValue || (nValue<0 && a))?2:0;
				if(i>0){
					a1=bData[idx-1];
					a1=(a1==nValue || (nValue<0 && a1))?1:0;
				}
				if(i<nWidth-1){
					a2=bData[idx+1];
					a2=(a2==nValue || (nValue<0 && a2))?1:0;
				}
			}

			a1|=a;a2|=a;

			//check left
			if(a1==2){
				if(start1<0){
					start1=j;
					start1b=(prev1==3)?-1:1;
				}
			}else if(start1>=0){
				int end1b=(a1==3)?1:-1;

				float p[8]={
					float(nLeft+i),float(nTop+start1),
					float(nLeft+i),float(nTop+j),
					float(nLeft+i)+fSize,float(nTop+j)+end1b*fSize,
					float(nLeft+i)+fSize,float(nTop+start1)+start1b*fSize,
				};

				float n[3]={
					-SQRT_1_2,0,SQRT_1_2,
				};

				shade->AddPrimitives(VertexList::Quads,p,n,4);
				if(theApp->m_bShowLines && lines){
					lines->AddPrimitives(VertexList::Lines,p,NULL,4);
				}

				start1=-1;
			}
			prev1=a1;

			//check right
			if(a2==2){
				if(start2<0){
					start2=j;
					start2b=(prev2==3)?-1:1;
				}
			}else if(start2>=0){
				int end2b=(a2==3)?1:-1;

				float p[8]={
					float(nLeft+i+1),float(nTop+start2),
					float(nLeft+i+1),float(nTop+j),
					float(nLeft+i+1)-fSize,float(nTop+j)+end2b*fSize,
					float(nLeft+i+1)-fSize,float(nTop+start2)+start2b*fSize,
				};

				float n[3]={
					SQRT_1_2,0,SQRT_1_2,
				};

				shade->AddPrimitives(VertexList::Quads,p,n,4);
				if(theApp->m_bShowLines && lines){
					lines->AddPrimitives(VertexList::Lines,p,NULL,4);
				}

				start2=-1;
			}
			prev2=a2;
		}
	}
}

#define CREATE_RECTANGULAR_OVERLAY(NAME,EXPRESSION,HALF_SIZE) \
	for(int j=0;j<m_nHeight;j++){ \
		for(int i=0;i<m_nWidth;i++){ \
			int idx=j*m_nWidth+i; \
			if(EXPRESSION){ \
				float x=float(i)+0.5f,y=float(j)+0.5f; \
				m_Graphics->NAME.AddRectangle(x-HALF_SIZE,y-HALF_SIZE,x+HALF_SIZE,y+HALF_SIZE); \
			} \
		} \
	}

void PuzzleBoyLevel::CreateGraphics(){
	delete m_Graphics;
	m_Graphics=new PuzzleBoyLevelGraphics;

	//background
	m_Graphics->bg.AddRectangle(0,0,float(m_nWidth),float(m_nHeight));

	//grid
	for(int i=0;i<=m_nWidth;i++){
		m_Graphics->grid.AddLine(float(i),0,float(i),float(m_nHeight));
	}
	for(int i=0;i<=m_nHeight;i++){
		m_Graphics->grid.AddLine(0,float(i),float(m_nWidth),float(i));
	}

	//wall
	CreateRectangles(0,0,m_nWidth,m_nHeight,&(m_bMapData[0]),WALL_TILE,&(m_Graphics->wall));

	//target overlay (?)
	CREATE_RECTANGULAR_OVERLAY(target_overlay,m_bTargetData[idx],m_fOverlaySize);

	//shade of wall
	CreateBorder(0,0,m_nWidth,m_nHeight,&(m_bMapData[0]),WALL_TILE,
		&(m_Graphics->wall_shade),&(m_Graphics->wall_line),m_fBorderSize);

	//pushable blocks
	for(unsigned int i=0;i<m_objBlocks.size();i++){
		m_objBlocks[i]->CreateGraphics();
		if(m_objBlocks[i]->m_nType==ROTATE_BLOCK && theApp->m_bShowLines){
			m_Graphics->wall_line.AddEllipse(float(m_objBlocks[i]->m_x)+0.5f,
				float(m_objBlocks[i]->m_y)+0.5f,m_fRotateBlockCenterSize,m_fRotateBlockCenterSize,
				0.0f,12);
		}
	}

	//center of rotate block
	m_Graphics->rotate_block_center.AddHemisphere(0.5f,0.5f,
		m_fRotateBlockCenterSize,m_fRotateBlockCenterSize,m_fRotateBlockCenterSize,0.0f,12,4);

	//player
	m_Graphics->player.AddHemisphere(0.5f,0.5f,m_fPlayerSize,m_fPlayerSize,m_fPlayerSize,0.0f,16,4);
	m_Graphics->player.AddEllipse(0.375f,0.375f,0.0625f,0.0625f,0.0f,8,0.0f,0.0f,0.0f);
	m_Graphics->player.AddEllipse(0.625f,0.375f,0.0625f,0.0625f,0.0f,8,0.0f,0.0f,0.0f);
	m_Graphics->player.AddRectangle(0.375f,0.6f,0.625f,0.625f,0.0f,0.0f,0.0f);
	if(theApp->m_bShowLines) m_Graphics->player_line.AddEllipse(0.5f,0.5f,m_fPlayerSize,m_fPlayerSize,0.0f,16);

	//exit
	{
		const float r=0.375f;

		float pt[38];

		for(int k=0;k<16;k++){
			float a=3.14159265f/16.0f*k;
			pt[k*2]=0.5f-r*cos(a);
			pt[k*2+1]=0.5f-r*sin(a);
		}

		pt[16]=0.5f;
		pt[17]=0.5f-r;
		pt[32]=0.5f+r;
		pt[33]=0.5f;
		pt[34]=0.5f+r;
		pt[35]=0.5f+r;
		pt[36]=0.5f-r;
		pt[37]=0.5f+r;

		m_Graphics->exit.AddPrimitives(VertexList::TriangleFan,pt,NULL,19);
		if(theApp->m_bShowLines){
			m_Graphics->exit_line.AddPrimitives(VertexList::LineLoop,pt,NULL,19);
			m_Graphics->exit_line.AddLine(0.5f,0.5f-r,0.5f,0.5f+r);
		}

		pt[36]=0.5f;
		m_Graphics->exit_right.AddPrimitives(VertexList::TriangleFan,pt+16,NULL,11);
		pt[18]=0.5f;
		pt[19]=0.5f+r;
		pt[20]=0.5f-r;
		pt[21]=0.5f+r;
		m_Graphics->exit_left.AddPrimitives(VertexList::TriangleFan,pt,NULL,11);

		m_Graphics->exit_left.AddEllipse(0.31f,0.57f,0.07f,0.07f,0.0f,8,0.0f,0.0f,0.0f);
		m_Graphics->exit_right.AddEllipse(0.69f,0.57f,0.07f,0.07f,0.0f,8,0.0f,0.0f,0.0f);
	}

	UpdateGraphics(-1);
}

void PuzzleBoyLevel::DestroyGraphics(){
	delete m_Graphics;
	m_Graphics=NULL;
}

void PuzzleBoyLevel::UpdateGraphics(int type){
	if(m_Graphics==NULL) return;

	if(type & UpdateEmptyTile){
		m_Graphics->empty.clear();
		m_Graphics->empty_overlay.clear();
		CreateRectangles(0,0,m_nWidth,m_nHeight,&(m_bMapData[0]),EMPTY_TILE,&(m_Graphics->empty));
		CREATE_RECTANGULAR_OVERLAY(empty_overlay,m_bMapData[idx]==EMPTY_TILE,m_fOverlaySize);
	}
}

void PuzzleBoyLevel::Draw(bool bEditMode,int nEditingBlockIndex){
	if(m_Graphics==NULL) return;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_LIGHTING);

	//bg
	glColor4f(FLOOR_COLOR);
	m_Graphics->bg.Draw();

	//empty block
	glColor4f(EMPTY_COLOR);
	m_Graphics->empty.Draw();

	//grid
	if(theApp->m_bShowGrid){
		glColor4f(GRID_COLOR);
		m_Graphics->grid.Draw();
	}

	//wall
	glColor4f(BLOCK_COLOR);
	m_Graphics->wall.Draw();

	//exit
	glColor4f(LINE_COLOR);
	for(int j=0;j<m_nHeight;j++){
		for(int i=0;i<m_nWidth;i++){
			int idx=j*m_nWidth+i;
			if(m_bMapData[idx]==EXIT_TILE){
				glTranslatef(float(i),float(j),0.0f);
				m_Graphics->exit.Draw();
				glLoadIdentity();
			}
		}
	}

	//pushable blocks
	glColor4f(PUSH_BLOCK_COLOR);
	for(unsigned int i=0;i<m_objBlocks.size();i++){
		if(m_objBlocks[i]->m_nType==NORMAL_BLOCK && m_objBlocks[i]->m_faces && (!bEditMode || i!=nEditingBlockIndex)){
			bool b=(i==m_nBlockAnimationIndex);
			if(b) glTranslatef(
				m_nMoveAnimationX*float(m_nMoveAnimationTime-8)*0.125f,
				m_nMoveAnimationY*float(m_nMoveAnimationTime-8)*0.125f,
				0.0f);
			m_objBlocks[i]->m_faces->Draw();
			if(b) glLoadIdentity();
		}
	}

	//target blocks
	glColor4f(TARGET_BLOCK_COLOR);
	for(unsigned int i=0;i<m_objBlocks.size();i++){
		if(m_objBlocks[i]->m_nType==TARGET_BLOCK && m_objBlocks[i]->m_faces && (!bEditMode || i!=nEditingBlockIndex)){
			bool b=(i==m_nBlockAnimationIndex);
			if(b) glTranslatef(
				m_nMoveAnimationX*float(m_nMoveAnimationTime-8)*0.125f,
				m_nMoveAnimationY*float(m_nMoveAnimationTime-8)*0.125f,
				0.0f);
			m_objBlocks[i]->m_faces->Draw();
			if(b) glLoadIdentity();
		}
	}

	//rotate blocks
	glColor4f(ROTATE_BLOCK_COLOR);
	for(unsigned int i=0;i<m_objBlocks.size();i++){
		if(m_objBlocks[i]->m_nType==ROTATE_BLOCK && m_objBlocks[i]->m_faces && (!bEditMode || i!=nEditingBlockIndex)){
			bool b=(i==m_nBlockAnimationIndex);
			if(b){
				float x=float(m_objBlocks[i]->m_x)+0.5f,y=float(m_objBlocks[i]->m_y)+0.5f;
				glTranslatef(x,y,0.0f);
				glRotatef(float(m_nBlockAnimationFlags?m_nMoveAnimationTime-8:8-m_nMoveAnimationTime)*(90.0f*0.125f),0.0f,0.0f,1.0f);
				glTranslatef(-x,-y,0.0f);
			}
			m_objBlocks[i]->m_faces->Draw();
			if(b) glLoadIdentity();
		}
	}

	glEnable(GL_LIGHTING);

	//shade of walls
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, BLOCK_COLOR_mat);
	m_Graphics->wall_shade.Draw();

	//shade of pushable blocks
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, PUSH_BLOCK_COLOR_mat);
	for(unsigned int i=0;i<m_objBlocks.size();i++){
		if(m_objBlocks[i]->m_nType==NORMAL_BLOCK && m_objBlocks[i]->m_shade && (!bEditMode || i!=nEditingBlockIndex)){
			bool b=(i==m_nBlockAnimationIndex);
			if(b) glTranslatef(
				m_nMoveAnimationX*float(m_nMoveAnimationTime-8)*0.125f,
				m_nMoveAnimationY*float(m_nMoveAnimationTime-8)*0.125f,
				0.0f);
			m_objBlocks[i]->m_shade->Draw();
			if(b) glLoadIdentity();
		}
	}

	//shade of target blocks
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, TARGET_BLOCK_COLOR_mat);
	for(unsigned int i=0;i<m_objBlocks.size();i++){
		if(m_objBlocks[i]->m_nType==TARGET_BLOCK && m_objBlocks[i]->m_shade && (!bEditMode || i!=nEditingBlockIndex)){
			bool b=(i==m_nBlockAnimationIndex);
			if(b) glTranslatef(
				m_nMoveAnimationX*float(m_nMoveAnimationTime-8)*0.125f,
				m_nMoveAnimationY*float(m_nMoveAnimationTime-8)*0.125f,
				0.0f);
			m_objBlocks[i]->m_shade->Draw();
			if(b) glLoadIdentity();
		}
	}

	//shade of rotate blocks
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, ROTATE_BLOCK_COLOR_mat);
	for(unsigned int i=0;i<m_objBlocks.size();i++){
		if(m_objBlocks[i]->m_nType==ROTATE_BLOCK && m_objBlocks[i]->m_shade && (!bEditMode || i!=nEditingBlockIndex)){
			bool b=(i==m_nBlockAnimationIndex);
			if(b){
				float x=float(m_objBlocks[i]->m_x)+0.5f,y=float(m_objBlocks[i]->m_y)+0.5f;
				glTranslatef(x,y,0.0f);
				glRotatef(float(m_nBlockAnimationFlags?m_nMoveAnimationTime-8:8-m_nMoveAnimationTime)*(90.0f*0.125f),0.0f,0.0f,1.0f);
				glTranslatef(-x,-y,0.0f);
			}
			m_objBlocks[i]->m_shade->Draw();
			if(b) glLoadIdentity();
		}
	}

	//center of rotate blocks & exit
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, ROTATE_BLOCK_CENTER_mat);
	for(unsigned int i=0;i<m_objBlocks.size();i++){
		if(m_objBlocks[i]->m_nType==ROTATE_BLOCK && (!bEditMode || i!=nEditingBlockIndex)){
			glTranslatef(float(m_objBlocks[i]->m_x),float(m_objBlocks[i]->m_y),0.0f);
			m_Graphics->rotate_block_center.Draw();
			glLoadIdentity();
		}
	}
	if(m_nExitAnimationTime>=0 && m_nExitAnimationTime<8){
		for(int j=0;j<m_nHeight;j++){
			for(int i=0;i<m_nWidth;i++){
				int idx=j*m_nWidth+i;
				if(m_bMapData[idx]==EXIT_TILE){
					if(m_nExitAnimationTime==0){
						glTranslatef(float(i),float(j),0.0f);
						m_Graphics->exit_left.Draw();
						m_Graphics->exit_right.Draw();
					}else{
						float a=(90.0f/8.0f)*m_nExitAnimationTime;
						glTranslatef(float(i)+0.125f,float(j),0.0f);
						glRotatef(-a,0.0f,1.0f,0.0f);
						glTranslatef(-0.125f,0.0f,0.0f);
						m_Graphics->exit_left.Draw();
						glLoadIdentity();
						glTranslatef(float(i)+0.875f,float(j),0.0f);
						glRotatef(a,0.0f,1.0f,0.0f);
						glTranslatef(-0.875f,0.0f,0.0f);
						m_Graphics->exit_right.Draw();
					}
					glLoadIdentity();
				}
			}
		}
	}

	//player
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, PLAYER_mat);
	for(int j=0;j<m_nHeight;j++){
		for(int i=0;i<m_nWidth;i++){
			int idx=j*m_nWidth+i;
			if(m_bMapData[idx]==PLAYER_TILE && (i!=m_nPlayerX || j!=m_nPlayerY || bEditMode)){
				glTranslatef(float(i),float(j),0.0f);
				m_Graphics->player.Draw();
				glLoadIdentity();
			}
		}
	}

	//active player and animation
	if(m_nPlayerX>=0 && m_nPlayerX<m_nWidth && m_nPlayerY>=0 && m_nPlayerY<m_nHeight && !bEditMode){
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, PLAYER_ACTIVATED_mat);

		float x0=float(m_nPlayerX),y0=float(m_nPlayerY);

		if(m_nMoveAnimationTime>0){
			x0-=(m_nMoveAnimationX*float(8-m_nMoveAnimationTime))/8.0f;
			y0-=(m_nMoveAnimationY*float(8-m_nMoveAnimationTime))/8.0f;
		}

		if(m_nSwitchAnimationTime>0){
			int i=m_nSwitchAnimationTime<4?m_nSwitchAnimationTime:8-m_nSwitchAnimationTime;
			y0-=i*0.125f;
		}

		glTranslatef(x0,y0,0.0f);
		m_Graphics->player.Draw();
		glLoadIdentity();
	}

	glDisable(GL_LIGHTING);

	//lines
	if(theApp->m_bShowLines){
		glColor4f(LINE_COLOR);
		m_Graphics->wall_line.Draw();

		//line of all blocks
		for(unsigned int i=0;i<m_objBlocks.size();i++){
			if(m_objBlocks[i]->m_lines && (!bEditMode || i!=nEditingBlockIndex)){
				bool b=(i==m_nBlockAnimationIndex);
				if(b){
					switch(m_objBlocks[i]->m_nType){
					case NORMAL_BLOCK:
					case TARGET_BLOCK:
						glTranslatef(
							m_nMoveAnimationX*float(m_nMoveAnimationTime-8)*0.125f,
							m_nMoveAnimationY*float(m_nMoveAnimationTime-8)*0.125f,
							0.0f);
						break;
					case ROTATE_BLOCK:
						{
							float x=float(m_objBlocks[i]->m_x)+0.5f,y=float(m_objBlocks[i]->m_y)+0.5f;
							glTranslatef(x,y,0.0f);
							glRotatef(float(m_nBlockAnimationFlags?m_nMoveAnimationTime-8:8-m_nMoveAnimationTime)*(90.0f*0.125f),0.0f,0.0f,1.0f);
							glTranslatef(-x,-y,0.0f);
						}
						break;
					}
				}
				m_objBlocks[i]->m_lines->Draw();
				if(b) glLoadIdentity();
			}
		}

		//line of player & exit
		for(int j=0;j<m_nHeight;j++){
			for(int i=0;i<m_nWidth;i++){
				int idx=j*m_nWidth+i;
				unsigned char d=m_bMapData[idx];
				bool b=(i==m_nPlayerX && j==m_nPlayerY && !bEditMode);
				if(b || d==PLAYER_TILE){
					float x0=float(i),y0=float(j);

					if(b && m_nMoveAnimationTime>0){
						x0-=(m_nMoveAnimationX*float(8-m_nMoveAnimationTime))/8.0f;
						y0-=(m_nMoveAnimationY*float(8-m_nMoveAnimationTime))/8.0f;
					}

					if(b && m_nSwitchAnimationTime>0){
						int i=m_nSwitchAnimationTime<4?m_nSwitchAnimationTime:8-m_nSwitchAnimationTime;
						y0-=i*0.125f;
					}

					glTranslatef(x0,y0,0.0f);
					m_Graphics->player_line.Draw();
					glLoadIdentity();
				}else if(d==EXIT_TILE && (m_nExitAnimationTime==0 || m_nExitAnimationTime==8)){
					glTranslatef(float(i),float(j),0.0f);
					m_Graphics->exit_line.Draw();
					glLoadIdentity();
				}
			}
		}
	}

	//target overlay
	glColor4f(TARGET_COLOR_OVERLAY);
	m_Graphics->target_overlay.Draw();

	//empty overlay
	glColor4f(EMPTY_COLOR_OVERLAY);
	m_Graphics->empty_overlay.Draw();

	//edit mode
	if(bEditMode && nEditingBlockIndex>=0 && nEditingBlockIndex<(int)m_objBlocks.size()){
		//background
		float vv[8]={
			-1,-1,
			-1,float(m_nHeight+1),
			float(m_nWidth+1),-1,
			float(m_nWidth+1),float(m_nHeight+1),
		};

		unsigned short ii[6]={0,1,3,0,3,2};

		glColor4f(0.0f, 0.0f, 0.0f, 0.5f);

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2,GL_FLOAT,0,vv);
		glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,ii);
		glDisableClientState(GL_VERTEX_ARRAY);

		int type=m_objBlocks[nEditingBlockIndex]->m_nType;

		if(m_objBlocks[nEditingBlockIndex]->m_faces){
			switch(type){
			case NORMAL_BLOCK: glColor4f(PUSH_BLOCK_COLOR); break;
			case ROTATE_BLOCK: glColor4f(ROTATE_BLOCK_COLOR); break;
			case TARGET_BLOCK: glColor4f(TARGET_BLOCK_COLOR); break;
			}
			m_objBlocks[nEditingBlockIndex]->m_faces->Draw();
		}

		glEnable(GL_LIGHTING);

		if(m_objBlocks[nEditingBlockIndex]->m_shade){
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
				type==TARGET_BLOCK?TARGET_BLOCK_COLOR_mat:
				(type==ROTATE_BLOCK?ROTATE_BLOCK_COLOR_mat:PUSH_BLOCK_COLOR_mat));
			m_objBlocks[nEditingBlockIndex]->m_shade->Draw();
		}

		if(type==ROTATE_BLOCK){
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, ROTATE_BLOCK_CENTER_mat);
			glTranslatef(float(m_objBlocks[nEditingBlockIndex]->m_x),float(m_objBlocks[nEditingBlockIndex]->m_y),0.0f);
			m_Graphics->rotate_block_center.Draw();
			glLoadIdentity();
		}

		glDisable(GL_LIGHTING);

		if(theApp->m_bShowLines && m_objBlocks[nEditingBlockIndex]->m_lines){
			glColor4f(LINE_COLOR);
			m_objBlocks[nEditingBlockIndex]->m_lines->Draw();
		}
	}
}

bool PuzzleBoyLevel::Undo(){
	if(IsAnimating()) return true;

	if(CanUndo()){
		if(m_bSendNetworkMove){
			NetworkMove move={4,0,0};
			netMgr->SendPlayerMove(move);
		}

		m_objCurrentUndo=NULL;
		PuzzleBoyLevelUndo *objUndo=m_objUndo[m_nCurrentUndo-1];

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
					PushableBlock *objBlock=objUndo->m_objDeletedBlock;
					if(objBlock){
						m_objBlocks.insert(m_objBlocks.begin()+idx,new PushableBlock(*objBlock));

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

						//update graphics
						if(m_Graphics) UpdateGraphics(UpdateEmptyTile);
					}

					//reset block movement
					objBlock=m_objBlocks[idx];
					if(objBlock->m_nType==ROTATE_BLOCK){
						m_nBlockAnimationFlags=(x-objBlock->m_x)*dy-(y-objBlock->m_y)*dx<0;
						objBlock->Rotate(m_nBlockAnimationFlags);
					}else{
						objBlock->m_x-=dx;
						objBlock->m_y-=dy;
					}

					//update graphics
					if(m_Graphics) objBlock->CreateGraphics();

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

bool PuzzleBoyLevel::Redo(){
	if(IsAnimating()) return true;

	if(CanRedo()){
		if(m_bSendNetworkMove){
			NetworkMove move={5,0,0};
			netMgr->SendPlayerMove(move);
		}

		m_objCurrentUndo=NULL;
		PuzzleBoyLevelUndo *objUndo=m_objUndo[m_nCurrentUndo];

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

void PuzzleBoyLevel::StartGame()
{
	int i,j;

	m_nMoves=0;
	m_nPlayerCount=0;
	m_nExitCount=0;
	m_nExitAnimationTime=0;
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
	for(i=0;i<(int)m_objUndo.size();i++) delete m_objUndo[i];
	m_objUndo.clear();
	m_nCurrentUndo=0;
	m_objCurrentUndo=NULL;

	//find player and init target data
	m_bTargetData.resize(m_nWidth*m_nHeight);
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
	m_bMoveableData.resize(m_nWidth*m_nHeight);
	UpdateMoveableData();
}

bool PuzzleBoyLevel::IsAnimating() const
{
	return m_nMoveAnimationTime || m_nSwitchAnimationTime || m_nExitAnimationTime!=(IsTargetFinished()?8:0);
}

bool PuzzleBoyLevel::IsWin() const
{
	return (m_nPlayerCount==0 || (m_nExitCount==0 && m_nTargetCount>0 && IsTargetFinished())) && m_nMoves>0;
}

void PuzzleBoyLevel::SaveUndo(PuzzleBoyLevelUndo* objUndo){
	//delete existing outdated undo data
	if(m_nCurrentUndo<(int)m_objUndo.size()){
		for(int i=m_nCurrentUndo;i<(int)m_objUndo.size();i++){
			delete m_objUndo[i];
		}
		m_objUndo.resize(m_nCurrentUndo);
	}

	//add new undo data
	m_objUndo.push_back(objUndo);
	m_objCurrentUndo=objUndo;
	m_nCurrentUndo=m_objUndo.size();
}

bool PuzzleBoyLevel::SwitchPlayer(int x,int y,bool bSaveUndo)
{
	m_objCurrentUndo=NULL;

	if(m_nWidth<=0 || m_nHeight<=0) return false;

	if(x>=0 && y>=0 && x<m_nWidth && y<m_nHeight){
		if(m_bMapData[y*m_nWidth+x]==PLAYER_TILE){
			if(x==m_nPlayerX && y==m_nPlayerY) return false;

			//save undo
			if(bSaveUndo){
				PuzzleBoyLevelUndo* objUndo=new PuzzleBoyLevelUndo;
				objUndo->m_x=m_nPlayerX;
				objUndo->m_y=m_nPlayerY;
				objUndo->m_dx=x;
				objUndo->m_dy=y;
				SaveUndo(objUndo);

				if(m_bSendNetworkMove){
					NetworkMove move={6,x,y};
					netMgr->SendPlayerMove(move);
				}
			}

			m_nPlayerX=x;
			m_nPlayerY=y;
			m_nSwitchAnimationTime=GetAnimationTime();

			//ensure visible
			if(m_view) m_view->m_scrollView.EnsureVisible(float(x),float(y),1,1);

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
				PuzzleBoyLevelUndo* objUndo=new PuzzleBoyLevelUndo;
				objUndo->m_x=m_nPlayerX;
				objUndo->m_y=m_nPlayerY;
				objUndo->m_dx=x;
				objUndo->m_dy=y;
				SaveUndo(objUndo);

				if(m_bSendNetworkMove){
					NetworkMove move={6,x,y};
					netMgr->SendPlayerMove(move);
				}
			}

			m_nPlayerX=x;
			m_nPlayerY=y;
			m_nSwitchAnimationTime=GetAnimationTime();

			//ensure visible
			if(m_view) m_view->m_scrollView.EnsureVisible(float(x),float(y),1,1);

			return true;
		}
	}

	return false;
}

bool PuzzleBoyLevel::MovePlayer(int dx,int dy,bool bSaveUndo){
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
				for(unsigned int i=0;i<m_objBlocks.size();i++){
					PushableBlock *objBlock=m_objBlocks[i];
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
					PuzzleBoyLevelUndo *objUndo=new PuzzleBoyLevelUndo;
					objUndo->m_nType=1;
					objUndo->m_x=x;
					objUndo->m_y=y;
					objUndo->m_dx=dx;
					objUndo->m_dy=dy;
					objUndo->m_nMovedBlockIndex=m_nBlockAnimationIndex;
					SaveUndo(objUndo);

					if(m_bSendNetworkMove){
						NetworkMove move={
							dy<0?0:dy>0?1:dx<0?2:3,
							0,0};
						netMgr->SendPlayerMove(move);
					}
				}

				m_bMapData[idx]=FLOOR_TILE;
				if(m_bMapData[idx1]!=EXIT_TILE) m_bMapData[idx1]=PLAYER_TILE;

				m_nPlayerX=x1;
				m_nPlayerY=y1;

				m_nMoveAnimationX=dx;
				m_nMoveAnimationY=dy;
				m_nMoveAnimationTime=GetAnimationTime();

				if(m_nBlockAnimationIndex>=0){
					PushableBlock *objBlock=m_objBlocks[m_nBlockAnimationIndex];
					if(objBlock->m_nType==ROTATE_BLOCK){
						objBlock->Rotate(m_nBlockAnimationFlags);
					}else{
						objBlock->m_x+=dx;
						objBlock->m_y+=dy;
					}

					//update graphics
					if(m_Graphics) objBlock->CreateGraphics();
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

	//ensure visible (??)
	if(m_view) m_view->m_scrollView.EnsureVisible(float(m_nPlayerX),float(m_nPlayerY),1,1);

	return ret;
}

bool PuzzleBoyLevel::CheckIfCurrentPlayerCanMove(int dx,int dy,int x0,int y0){
	if(x0<0) x0=m_nPlayerX;
	if(y0<0) y0=m_nPlayerY;

	if(x0<0 || x0>=m_nWidth || y0<0 || y0>=m_nHeight) return false;

	//backup
	bool networkBackup=m_bSendNetworkMove;
	m_bSendNetworkMove=false;
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
	while(IsAnimating()) OnTimer(8);
	if(ret) Undo();
	while(IsAnimating()) OnTimer(8);

	//restore
	m_bSendNetworkMove=networkBackup;
	m_nPlayerX=oldX;
	m_nPlayerY=oldY;
	m_bMapData[idx2]=backup2;
	if(idx>=0) m_bMapData[idx]=backup;
	UpdateMoveableData();

	//over
	return ret;
}

void PuzzleBoyLevel::UpdateMoveableData(){
	// update target count
	m_nTargetFinished=0;
	for(unsigned int idx=0;idx<m_objBlocks.size();idx++){
		PushableBlock *objBlock=m_objBlocks[idx];
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
	for(unsigned int i=0;i<m_bMapData.size();i++){
		switch(m_bMapData[i]){
		case 0:
			m_bMoveableData[i]=0;
			break;
		case EXIT_TILE:
			m_bMoveableData[i]=IsTargetFinished()?0:1;
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

void PuzzleBoyLevel::OnTimer(int animationTime)
{
	if(animationTime<=0) animationTime=GetAnimationTime();

	if(IsTargetFinished()){
		m_nExitAnimationTime=(m_nExitAnimationTime+animationTime)&(-animationTime);
		if(m_nExitAnimationTime>8) m_nExitAnimationTime=8;
	}else{
		m_nExitAnimationTime=(m_nExitAnimationTime-animationTime)&(-animationTime);
		if(m_nExitAnimationTime<0) m_nExitAnimationTime=0;
	}

	if(m_nMoveAnimationTime>0){
		m_nMoveAnimationTime=(m_nMoveAnimationTime+animationTime)&(-animationTime);
		if(m_nMoveAnimationTime>=8){
			m_nMoveAnimationTime=0;

			//check moving blocks
			if(m_nBlockAnimationIndex>=0){
				PushableBlock *objBlock=m_objBlocks[m_nBlockAnimationIndex];
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

						m_objBlocks.erase(m_objBlocks.begin()+m_nBlockAnimationIndex);

						//update graphics
						if(m_Graphics) UpdateGraphics(UpdateEmptyTile);

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
				}else{
					m_nPlayerX=-1;
					m_nPlayerY=-1;
				}
			}
		}
	}

	if(m_nSwitchAnimationTime>0){
		m_nSwitchAnimationTime=(m_nSwitchAnimationTime+animationTime)&(-animationTime);
		if(m_nSwitchAnimationTime>=8) m_nSwitchAnimationTime=0;
	}
}

u8string PuzzleBoyLevel::GetRecord() const
{
	u8string s;

	for(int i=0;i<m_nCurrentUndo;i++){
		PuzzleBoyLevelUndo *obj=m_objUndo[i];

		switch(obj->m_nType){
		case 0:
			{
				char ss[32];
				sprintf(ss,"(%d,%d)",obj->m_dx+1,obj->m_dy+1);
				s.append(ss);
			}
			break;
		case 1:
			s.push_back(obj->m_dx<0?'A':obj->m_dy<0?'W':obj->m_dx>0?'D':'S');
			break;
		default:
			break;
		}
	}

	return s;
}

bool PuzzleBoyLevel::ApplyRecord(const u8string& rec)
{
	for(int i=0,m=rec.size();i<m;i++){
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

		while(IsAnimating()) OnTimer(8);

		if(IsWin()) break;
	}

	return true;
}

void PuzzleBoyLevel::SerializeHistory(MySerializer& ar,bool hasRecord,bool hasRedo){
	bool bak=m_bSendNetworkMove;
	m_bSendNetworkMove=false;

	//serialize record
	if(hasRecord){
		if(ar.IsStoring()){
			RecordManager::ConvertStringToRecordData(ar,GetRecord());
		}else{
			u8string rec;
			RecordManager::ConvertRecordDataToString(ar,rec);
			ApplyRecord(rec);
		}
	}

	//serialize redo
	if(hasRedo){
		if(ar.IsStoring()){
			int m=m_objUndo.size()-m_nCurrentUndo;
			if(m<0) m=0;

			ar.PutVUInt32(m);

			for(int i=0;i<m;i++){
				m_objUndo[m_nCurrentUndo+i]->MySerialize(ar);
			}
		}else{
			int m=ar.GetVUInt32();

			for(int i=0;i<m;i++){
				PuzzleBoyLevelUndo *undo=new PuzzleBoyLevelUndo;
				undo->MySerialize(ar);
				m_objUndo.push_back(undo);
			}
		}
	}

	m_bSendNetworkMove=bak;
}
