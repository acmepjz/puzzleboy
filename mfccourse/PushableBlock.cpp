// PushableBlock.cpp : 实现文件
//

#include "stdafx.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include "mfccourse.h"
#include "Level.h"
#include "PushableBlock.h"

IMPLEMENT_SERIAL(CPushableBlock,CObject,VERSIONABLE_SCHEMA | 2);

// CPushableBlock

CPushableBlock::CPushableBlock()
: m_nType(0)
, m_x(0)
, m_y(0)
, m_w(0)
, m_h(0)
{
}

CPushableBlock::CPushableBlock(const CPushableBlock& obj)
: m_nType(obj.m_nType)
, m_x(obj.m_x)
, m_y(obj.m_y)
, m_w(obj.m_w)
, m_h(obj.m_h)
{
	m_bData.Copy(obj.m_bData);
}

CPushableBlock::~CPushableBlock()
{
}


// CPushableBlock 成员函数

static void DoBlockDataRLE(const CArray<unsigned char>& src,CArray<unsigned char>& dst){
	dst.RemoveAll();
	if(src.IsEmpty()) return;

	int lastIndex=0;
	int m=src.GetSize();
	bool lastValue=(src[0]!=0);

	for(int i=1;i<=m;i++){
		int count=i-lastIndex;
		if(count>=128 || i>=m || (src[i]!=0)!=lastValue){
			dst.Add(((count-1)<<1)|(lastValue?1:0));

			if(i>=m) break;

			lastValue=(src[i]!=0);
			lastIndex=i;
		}
	}
}

static void DoBlockDataUnRLE(const CArray<unsigned char>& src,CArray<unsigned char>& dst){
	dst.RemoveAll();
	if(src.IsEmpty()) return;

	int m=src.GetSize();
	for(int i=0;i<m;i++){
		unsigned char b=src[i];
		int n=(b>>1)+1;
		b&=1;

		do dst.Add(b);
		while((--n)>0);
	}
}

void CPushableBlock::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// 在此添加存储代码

		switch(m_nType){
		case NORMAL_BLOCK:
		case TARGET_BLOCK:
			{
				//check if it's full
				bool b=true;
				for(int i=0,m=m_w*m_h;i<m;i++){
					if(m_bData[i]==0){
						b=false;
						break;
					}
				}

				if(b){
					//use new format
					int i;
					if(m_w>=1 && m_w<=4 && m_h>=1 && m_h<=4){
						i=((m_nType+1)<<4)
							| (m_w-1) | ((m_h-1)<<2);
						ar<<i<<m_x<<m_y;
					}else{
						i=m_nType|NORMAL_BLOCK_SOLID;
						ar<<i<<m_x<<m_y;
						ar<<m_w<<m_h;
					}
				}else{
					//use old format (not really)
					ar<<m_nType<<m_x<<m_y;
					ar<<m_w<<m_h;

					CArray<unsigned char> d;
					DoBlockDataRLE(m_bData,d);
					d.Serialize(ar);
				}
			}
			break;
		case ROTATE_BLOCK:
			{
				unsigned char m=m_bData[0];

				//check it it's small
				if(m<=1 && m_bData[1]<=1 && m_bData[2]<=1 && m_bData[3]<=1){
					int i=ROTATE_BLOCK_WITH_FIXED_SIZE
						| m | (m_bData[1]<<1) | (m_bData[2]<<2) | (m_bData[3]<<3);
					ar<<i<<m_x<<m_y;
					break;
				}

				//check if four arms are of equal length
				if(m==m_bData[1] && m==m_bData[2] && m==m_bData[3]){
					int i=ROTATE_BLOCK_WITH_EQUAL_ARMS;
					ar<<i<<m_x<<m_y<<m;
					break;
				}

				//use old format
				ar<<m_nType<<m_x<<m_y;
				m_bData.Serialize(ar);
			}
			break;
		}
	}
	else
	{
		//check version
		if(ar.GetObjectSchema()>2){
			AfxThrowArchiveException(CArchiveException::badSchema);
		}

		int type;
		ar>>type>>m_x>>m_y;

		switch(type){
		case NORMAL_BLOCK:
		case TARGET_BLOCK:
			m_nType=type;
			ar>>m_w>>m_h;
			{
				CArray<unsigned char> d;
				d.Serialize(ar);
				DoBlockDataUnRLE(d,m_bData);
				if(m_bData.GetSize()!=m_w*m_h){
					AfxThrowArchiveException(CArchiveException::badSchema);
				}
			}
			break;
		case ROTATE_BLOCK:
			m_nType=ROTATE_BLOCK;
			m_w=m_h=1;
			m_bData.Serialize(ar);
			break;
		case NORMAL_BLOCK_SOLID:
		case TARGET_BLOCK_SOLID:
			m_nType=type & 0xF;
			ar>>m_w>>m_h;
			if(m_w>0 && m_h>0){
				m_bData.SetSize(m_w*m_h);
				memset(&(m_bData[0]),1,m_w*m_h);
			}
			break;
		case ROTATE_BLOCK_WITH_EQUAL_ARMS:
			m_nType=ROTATE_BLOCK;
			m_w=m_h=1;
			{
				unsigned char m;
				ar>>m;
				m_bData.SetSize(4);
				m_bData[0]=m_bData[1]=m_bData[2]=m_bData[3]=m;
			}
			break;
		default:
			switch(type & 0xF0){
			case NORMAL_BLOCK_WITH_FIXED_SIZE:
			case TARGET_BLOCK_WITH_FIXED_SIZE:
				m_nType=(type>>4)-1;
				m_w=(type & 0x3)+1;
				m_h=((type>>2) & 0x3)+1;
				m_bData.SetSize(m_w*m_h);
				memset(&(m_bData[0]),1,m_w*m_h);
				break;
			case ROTATE_BLOCK_WITH_FIXED_SIZE:
				m_nType=ROTATE_BLOCK;
				m_w=m_h=1;
				m_bData.SetSize(4);
				m_bData[0]=type & 0x1;
				m_bData[1]=(type>>1) & 0x1;
				m_bData[2]=(type>>2) & 0x1;
				m_bData[3]=(type>>3) & 0x1;
				break;
			default:
				AfxThrowArchiveException(CArchiveException::badSchema);
			}
		}
	}
}

void CPushableBlock::MySerialize(MySerializer& ar){
	ar.PutVUInt32(m_nType);
	ar.PutVUInt32(m_x);
	ar.PutVUInt32(m_y);

	if(m_nType!=ROTATE_BLOCK){
		ar.PutVUInt32(m_w);
		ar.PutVUInt32(m_h);
		assert(m_w*m_h==m_bData.GetSize());
		for(int i=0;i<m_bData.GetSize();i++){
			ar.PutIntN(m_bData[i],1);
		}
		ar.PutFlush();
	}else{
		assert(4==m_bData.GetSize());
		for(int i=0;i<m_bData.GetSize();i++){
			ar.PutInt8(m_bData[i]);
		}
	}
}

bool CPushableBlock::Rotate(int bIsClockwise)
{
	if(m_nType==ROTATE_BLOCK){
		if(bIsClockwise){
			//rotate clockwise
			unsigned char tmp=m_bData[0];
			m_bData[0]=m_bData[1];
			m_bData[1]=m_bData[2];
			m_bData[2]=m_bData[3];
			m_bData[3]=tmp;
		}else{
			//rotate counter-clockwise
			unsigned char tmp=m_bData[0];
			m_bData[0]=m_bData[3];
			m_bData[3]=m_bData[2];
			m_bData[2]=m_bData[1];
			m_bData[1]=tmp;
		}
		return true;
	}
	return false;
}

void CPushableBlock::Draw(CDC *pDC,RECT r,int nBlockSize,int nEdgeSize,int nAnimation) const
{
	switch(m_nType){
	case NORMAL_BLOCK:
	case TARGET_BLOCK:
		{
			CBrush hbr(m_nType==NORMAL_BLOCK?PUSH_BLOCK_COLOR:TARGET_BLOCK_COLOR);
			CBrush hbrLight(m_nType==NORMAL_BLOCK?PUSH_BLOCK_LIGHT_COLOR:TARGET_BLOCK_LIGHT_COLOR);
			CBrush hbrDark(m_nType==NORMAL_BLOCK?PUSH_BLOCK_DARK_COLOR:TARGET_BLOCK_DARK_COLOR);

			for(int j=0;j<m_h;j++){
				for(int i=0;i<m_w;i++){
					if(m_bData[j*m_w+i]){
						bool bNeighborhood[8]={
							i>0 && j>0 && m_bData[(j-1)*m_w+(i-1)],
							j>0 && m_bData[(j-1)*m_w+i],
							i<m_w-1 && j>0 && m_bData[(j-1)*m_w+(i+1)],
							i>0 && m_bData[j*m_w+(i-1)],
							i<m_w-1 && m_bData[j*m_w+(i+1)],
							i>0 && j<m_h-1 && m_bData[(j+1)*m_w+(i-1)],
							j<m_h-1 && m_bData[(j+1)*m_w+i],
							i<m_w-1 && j<m_h-1 && m_bData[(j+1)*m_w+(i+1)]
						};
						int x0=r.left+(i+m_x)*nBlockSize;
						int y0=r.top+(j+m_y)*nBlockSize;
						RECT r1={x0,y0,x0+nBlockSize,y0+nBlockSize};
						CLevel::DrawEdgedBlock(pDC,r1,nEdgeSize,&hbr,&hbrLight,&hbrDark,bNeighborhood);
					}
				}
			}
		}
		break;
	case ROTATE_BLOCK:
		{
			POINT pt[12]={
				{nBlockSize,-nBlockSize*m_bData[0]},
				{0,-nBlockSize*m_bData[0]},
				{0,0},
				{-nBlockSize*m_bData[1],0},
				{-nBlockSize*m_bData[1],nBlockSize},
				{0,nBlockSize},
				{0,nBlockSize*m_bData[2]+nBlockSize},
				{nBlockSize,nBlockSize*m_bData[2]+nBlockSize},
				{nBlockSize,nBlockSize},
				{nBlockSize*m_bData[3]+nBlockSize,nBlockSize},
				{nBlockSize*m_bData[3]+nBlockSize,0},
				{nBlockSize,0},
			};

			POINT pt2[12]={};
			POINT pt3[4];

			unsigned char clrIndex[12];

			CBrush hbrBorder[4];

			int nCount=12;
			int j=0;

			for(int i=1;i<nCount;i++){
				if(pt[i].x==pt[i-j-1].x && pt[i].y==pt[i-j-1].y){
					j++;
				}else if(j>0){
					pt[i-j]=pt[i];
				}
			}

			nCount-=j;
			while(nCount>0 && pt[0].x==pt[nCount-1].x &&
				pt[0].y==pt[nCount-1].y) nCount--;

			assert(nCount>=4);

			for(int i=0;i<nCount;i++){
				int j=i+1;
				if(j>=nCount) j=0;
				if(pt[j].x<pt[i].x){
					pt2[i].y+=nEdgeSize;
					pt2[j].y+=nEdgeSize;
					clrIndex[i]=0;
				}else if(pt[j].x>pt[i].x){
					pt2[i].y-=nEdgeSize;
					pt2[j].y-=nEdgeSize;
					clrIndex[i]=2;
				}else if(pt[j].y<pt[i].y){
					pt2[i].x-=nEdgeSize;
					pt2[j].x-=nEdgeSize;
					clrIndex[i]=3;
				}else{
					assert(pt[j].y>pt[i].y);
					pt2[i].x+=nEdgeSize;
					pt2[j].x+=nEdgeSize;
					clrIndex[i]=1;
				}
			}

			j=0;
			for(int i=0;i<nCount;i++){
				if(pt2[i].x>nEdgeSize || pt2[i].x<-nEdgeSize
					|| pt2[i].y>nEdgeSize || pt2[i].y<-nEdgeSize)
				{
					j++;
				}else if(j>0){
					pt[i-j]=pt[i];
					pt2[i-j]=pt2[i];
					clrIndex[i-j]=clrIndex[i];
				}
			}

			nCount-=j;
			assert(nCount>=4);

			if(nAnimation>0){
				float offset=float(nBlockSize)/2.0f;
				float a=3.14159265f/16.0f*float(nAnimation);
				float ss=sin(a),cc=cos(a);
				for(int i=0;i<nCount;i++){
					float x0=float(pt[i].x)-offset;
					float y0=float(pt[i].y)-offset;
					pt[i].x=int(floor(x0*ss-y0*cc+offset+0.5f));
					pt[i].y=int(floor(x0*cc+y0*ss+offset+0.5f));
					x0=float(pt2[i].x);
					y0=float(pt2[i].y);
					pt2[i].x=int(floor(x0*ss-y0*cc+0.5f));
					pt2[i].y=int(floor(x0*cc+y0*ss+0.5f));
				}
				a-=.7853981634f;
				for(int i=0;i<4;i++){
					int clr;
					ss=sin(a);
					if(ss<0.0f){
						clr=255-int(ss*(-90.509668f) + 0.5f);
						clr*=0x10100;
					}else{
						clr=0xFFFF00 | int(ss*181.019336f + 0.5f);
					}
					hbrBorder[i].CreateSolidBrush(clr);
					a+=1.57079633f;
				}
			}else{
				hbrBorder[0].CreateSolidBrush(0xFFFF80);
				hbrBorder[1].CreateSolidBrush(0xFFFF80);
				hbrBorder[2].CreateSolidBrush(0xC0C000);
				hbrBorder[3].CreateSolidBrush(0xC0C000);
			}

			int dx=r.left+m_x*nBlockSize;
			int dy=r.top+m_y*nBlockSize;
			for(int i=0;i<nCount;i++){
				pt[i].x+=dx;
				pt[i].y+=dy;
				pt2[i].x+=pt[i].x;
				pt2[i].y+=pt[i].y;
			}

			CBrush hbr(ROTATE_BLOCK_COLOR),hbr0(0x808080);

			CBrush *hbrOld=pDC->SelectObject(&hbr);
			pDC->Polygon(pt,nCount);
			pDC->Polygon(pt2,nCount);
			for(int i=0;i<nCount;i++){
				int j=i+1;
				if(j>=nCount) j=0;
				pt3[0]=pt[i];
				pt3[1]=pt[j];
				pt3[2]=pt2[j];
				pt3[3]=pt2[i];
				pDC->SelectObject(&hbrBorder[clrIndex[i]]);
				pDC->Polygon(pt3,4);
			}
			pDC->SelectObject(&hbr0);
			dx+=nBlockSize/2;
			dy+=nBlockSize/2;
			int r=nBlockSize/5;
			if(r<2) r=2;
			pDC->Ellipse(dx-r,dy-r,dx+r,dy+r);

			pDC->SelectObject(hbrOld);
		}
		break;
	default:
		break;
	}
}

void CPushableBlock::CreateSingle(int nType,int x,int y){
	m_nType=nType;
	m_x=x;
	m_y=y;
	m_w=1;
	m_h=1;

	switch(nType){
	case NORMAL_BLOCK:
	case TARGET_BLOCK:
		m_bData.SetSize(1);
		m_bData[0]=1;
		break;
	default:
		m_bData.SetSize(4);
		m_bData[0]=0;
		m_bData[1]=0;
		m_bData[2]=0;
		m_bData[3]=0;
		break;
	}
}

bool CPushableBlock::HitTest(int x,int y) const
{
	x-=m_x;
	y-=m_y;
	switch(m_nType){
	case NORMAL_BLOCK:
	case TARGET_BLOCK:
		if(x>=0 && y>=0 && x<m_w && y<m_h){
			return m_bData[y*m_w+x]!=0;
		}else{
			return false;
		}
		break;
	default:
		return (x==0 && y>=-int(m_bData[0]) && y<=int(m_bData[2]))
			|| (y==0 && x>=-int(m_bData[1]) && x<=int(m_bData[3]));
		break;
	}
}


void CPushableBlock::UnoptimizeSize(int nWidth,int nHeight){
	switch(m_nType){
	case NORMAL_BLOCK:
	case TARGET_BLOCK:
		{
			CArray<unsigned char> bNewData;
			bNewData.SetSize(nWidth*nHeight);

			for(int j=0;j<nHeight;j++){
				for(int i=0;i<nWidth;i++){
					int ii=i-m_x,jj=j-m_y;
					if(ii>=0 && jj>=0 && ii<m_w &&jj<m_h){
						bNewData[j*nWidth+i]=m_bData[jj*m_w+ii];
					}else{
						bNewData[j*nWidth+i]=0;
					}
				}
			}

			m_bData.Copy(bNewData);
			m_x=0;
			m_y=0;
			m_w=nWidth;
			m_h=nHeight;
		}
		break;
	default:
		break;
	}
}

bool CPushableBlock::OptimizeSize()
{
	switch(m_nType){
	case NORMAL_BLOCK:
	case TARGET_BLOCK:
		{
			int nLeft=0,nTop=0,nRight=0,nBottom=0;

			for(int j=0;j<m_h;j++){
				bool b=true;
				for(int i=0;i<m_w;i++){
					if(m_bData[j*m_w+i]){
						b=false;
						break;
					}
				}
				if(b) nTop++;
				else break;
			}

			for(int j=m_h-1;j>=0;j--){
				bool b=true;
				for(int i=0;i<m_w;i++){
					if(m_bData[j*m_w+i]){
						b=false;
						break;
					}
				}
				if(b) nBottom++;
				else break;
			}

			for(int i=0;i<m_w;i++){
				bool b=true;
				for(int j=nTop;j<m_h-nBottom;j++){
					if(m_bData[j*m_w+i]){
						b=false;
						break;
					}
				}
				if(b) nLeft++;
				else break;
			}

			for(int i=m_w-1;i>=0;i--){
				bool b=true;
				for(int j=nTop;j<m_h-nBottom;j++){
					if(m_bData[j*m_w+i]){
						b=false;
						break;
					}
				}
				if(b) nRight++;
				else break;
			}

			int w2=m_w-nLeft-nRight;
			int h2=m_h-nTop-nBottom;
			if(w2<=0 || h2<=0) return false;
			if(w2==m_w && h2==m_h) return true;

			CArray<unsigned char> bNewData;
			bNewData.SetSize(w2*h2);
			for(int j=0;j<h2;j++){
				for(int i=0;i<w2;i++){
					bNewData[j*w2+i]=m_bData[(j+nTop)*m_w+(i+nLeft)];
				}
			}

			m_bData.Copy(bNewData);
			m_x+=nLeft;
			m_y+=nTop;
			m_w=w2;
			m_h=h2;
		}
		break;
	default:
		break;
	}
	return true;
}

void CPushableBlock::MoveBlockInEditMode(int dx,int dy)
{
	switch(m_nType){
	case NORMAL_BLOCK:
	case TARGET_BLOCK:
		{
			CArray<unsigned char> bNewData;
			bNewData.SetSize(m_w*m_h);

			for(int j=0;j<m_h;j++){
				for(int i=0;i<m_w;i++){
					int ii=i-dx,jj=j-dy;
					if(ii>=0 && jj>=0 && ii<m_w &&jj<m_h){
						bNewData[j*m_w+i]=m_bData[jj*m_w+ii];
					}else{
						bNewData[j*m_w+i]=0;
					}
				}
			}

			m_bData.Copy(bNewData);
		}
		break;
	default:
		m_x+=dx;
		m_y+=dy;
		break;
	}
}
