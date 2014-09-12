#include "PushableBlock.h"
#include "VertexList.h"
#include "PuzzleBoyLevel.h"
#include "PuzzleBoyApp.h"
#include "main.h"

#include <assert.h>
#include <math.h>
#include <string.h>

// PushableBlock

PushableBlock::PushableBlock()
: m_nType(0)
, m_x(0)
, m_y(0)
, m_w(0)
, m_h(0)
, m_faces(NULL), m_shade(NULL), m_lines(NULL)
{
}

PushableBlock::PushableBlock(const PushableBlock& obj)
: m_nType(obj.m_nType)
, m_x(obj.m_x)
, m_y(obj.m_y)
, m_w(obj.m_w)
, m_h(obj.m_h)
, m_bData(obj.m_bData)
, m_faces(NULL), m_shade(NULL), m_lines(NULL)
{
}

PushableBlock::~PushableBlock()
{
	delete m_faces;
	delete m_shade;
	delete m_lines;
}

void PushableBlock::CreateGraphics(){
	if(m_faces==NULL) m_faces=new VertexList(false,true);
	else m_faces->clear();

	if(m_shade==NULL) m_shade=new VertexList(true,true);
	else m_shade->clear();

	if(theApp->m_bShowLines){
		if(m_lines==NULL) m_lines=new VertexList(false,true,VertexList::Lines);
		else m_lines->clear();
	}else{
		delete m_lines;
		m_lines=NULL;
	}

	switch(m_nType){
	case NORMAL_BLOCK:
	case TARGET_BLOCK:
		PuzzleBoyLevel::CreateRectangles(m_x,m_y,m_w,m_h,&(m_bData[0]),-1,m_faces);
		PuzzleBoyLevel::CreateBorder(m_x,m_y,m_w,m_h,&(m_bData[0]),-1,m_shade,m_lines,m_fBorderSize);
		break;
	case ROTATE_BLOCK:
		{
			//create rectangles
			if(m_bData[0]>0) m_faces->AddRectangle(float(m_x),float(m_y-m_bData[0]),float(m_x+1),float(m_y));
			m_faces->AddRectangle(float(m_x-m_bData[1]),float(m_y),float(m_x+1+m_bData[3]),float(m_y+1));
			if(m_bData[2]>0) m_faces->AddRectangle(float(m_x),float(m_y+1),float(m_x+1),float(m_y+1+m_bData[2]));

			//create border
			int pt[12][2]={
				1,-m_bData[0],
				0,-m_bData[0],
				0,0,
				-m_bData[1],0,
				-m_bData[1],1,
				0,1,
				0,m_bData[2]+1,
				1,m_bData[2]+1,
				1,1,
				m_bData[3]+1,1,
				m_bData[3]+1,0,
				1,0,
			};
			int pt2[12][2]={};
			unsigned char clrIndex[12];

			int nCount=12;
			int j=0;

			for(int i=1;i<nCount;i++){
				if(pt[i][0]==pt[i-j-1][0] && pt[i][1]==pt[i-j-1][1]){
					j++;
				}else if(j>0){
					pt[i-j][0]=pt[i][0];
					pt[i-j][1]=pt[i][1];
				}
			}

			nCount-=j;
			while(nCount>0 && pt[0][0]==pt[nCount-1][0] &&
				pt[0][1]==pt[nCount-1][1]) nCount--;

			assert(nCount>=4);

			for(int i=0;i<nCount;i++){
				int j=i+1;
				if(j>=nCount) j=0;
				if(pt[j][0]<pt[i][0]){
					pt2[i][1]++;
					pt2[j][1]++;
					clrIndex[i]=0;
				}else if(pt[j][0]>pt[i][0]){
					pt2[i][1]--;
					pt2[j][1]--;
					clrIndex[i]=2;
				}else if(pt[j][1]<pt[i][1]){
					pt2[i][0]--;
					pt2[j][0]--;
					clrIndex[i]=3;
				}else{
					assert(pt[j][1]>pt[i][1]);
					pt2[i][0]++;
					pt2[j][0]++;
					clrIndex[i]=1;
				}
			}

			j=0;
			for(int i=0;i<nCount;i++){
				if(pt2[i][0]>1 || pt2[i][0]<-1
					|| pt2[i][1]>1 || pt2[i][1]<-1)
				{
					j++;
				}else if(j>0){
					pt[i-j][0]=pt[i][0];
					pt[i-j][1]=pt[i][1];
					pt2[i-j][0]=pt2[i][0];
					pt2[i-j][1]=pt2[i][1];
					clrIndex[i-j]=clrIndex[i];
				}
			}

			nCount-=j;
			assert(nCount>=4);

			for(int i=0;i<nCount;i++){
				int j=i+1;
				if(j>=nCount) j=0;

				float n[3]={0.0f,0.0f,SQRT_1_2};

				switch(clrIndex[i]){
				case 0: n[1]=-SQRT_1_2; break;
				case 1: n[0]=-SQRT_1_2; break;
				case 2: n[1]= SQRT_1_2; break;
				case 3: n[0]= SQRT_1_2; break;
				}

				float p[8]={
					float(m_x+pt[i][0]),float(m_y+pt[i][1]),
					float(m_x+pt[j][0]),float(m_y+pt[j][1]),
					float(m_x+pt[j][0])+m_fBorderSize*pt2[j][0],float(m_y+pt[j][1])+m_fBorderSize*pt2[j][1],
					float(m_x+pt[i][0])+m_fBorderSize*pt2[i][0],float(m_y+pt[i][1])+m_fBorderSize*pt2[i][1],
				};

				m_shade->AddPrimitives(VertexList::Quads,p,n,4);

				if(m_lines) m_lines->AddPrimitives(VertexList::LineStrip,p,NULL,4);
			}
		}
		break;
	}
}

static void DoBlockDataRLE(const std::vector<unsigned char>& src,std::vector<unsigned char>& dst){
	dst.clear();
	if(src.empty()) return;

	int lastIndex=0;
	int m=src.size();
	bool lastValue=(src[0]!=0);

	for(int i=1;i<=m;i++){
		int count=i-lastIndex;
		if(count>=128 || i>=m || (src[i]!=0)!=lastValue){
			dst.push_back(((count-1)<<1)|(lastValue?1:0));

			if(i>=m) break;

			lastValue=(src[i]!=0);
			lastIndex=i;
		}
	}
}

static void DoBlockDataUnRLE(const std::vector<unsigned char>& src,std::vector<unsigned char>& dst){
	dst.clear();
	if(src.empty()) return;

	int m=src.size();
	for(int i=0;i<m;i++){
		unsigned char b=src[i];
		int n=(b>>1)+1;
		b&=1;

		do dst.push_back(b);
		while((--n)>0);
	}
}

bool PushableBlock::IsSolid() const{
	bool b=false;

	switch(m_nType){
	case NORMAL_BLOCK:
	case TARGET_BLOCK:
		b=true;
		for(int i=0,m=m_w*m_h;i<m;i++){
			if(m_bData[i]==0){
				b=false;
				break;
			}
		}
		break;
	}

	return b;
}

bool PushableBlock::MFCSerialize(MFCSerializer& ar)
{
	if (ar.IsStoring())
	{
		// saving

		switch(m_nType){
		case NORMAL_BLOCK:
		case TARGET_BLOCK:
			{
				//check if it's full
				if(IsSolid()){
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

					std::vector<unsigned char> d;
					DoBlockDataRLE(m_bData,d);
					ar<<d;
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
				ar>>m_bData;
			}
			break;
		}
	}
	else
	{
		//loading

		//check version
		if(ar.version>2) return false;

		int type;
		ar>>type>>m_x>>m_y;

		switch(type){
		case NORMAL_BLOCK:
		case TARGET_BLOCK:
			m_nType=type;
			ar>>m_w>>m_h;
			{
				std::vector<unsigned char> d;
				ar>>d;
				DoBlockDataUnRLE(d,m_bData);
				if(m_bData.size()!=m_w*m_h) return false;
			}
			break;
		case ROTATE_BLOCK:
			m_nType=ROTATE_BLOCK;
			m_w=m_h=1;
			ar>>m_bData;
			break;
		case NORMAL_BLOCK_SOLID:
		case TARGET_BLOCK_SOLID:
			m_nType=type & 0xF;
			ar>>m_w>>m_h;
			if(m_w>0 && m_h>0){
				m_bData.resize(m_w*m_h);
				memset(&(m_bData[0]),1,m_w*m_h);
			}
			break;
		case ROTATE_BLOCK_WITH_EQUAL_ARMS:
			m_nType=ROTATE_BLOCK;
			m_w=m_h=1;
			{
				unsigned char m;
				ar>>m;
				m_bData.resize(4);
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
				m_bData.resize(m_w*m_h);
				memset(&(m_bData[0]),1,m_w*m_h);
				break;
			case ROTATE_BLOCK_WITH_FIXED_SIZE:
				m_nType=ROTATE_BLOCK;
				m_w=m_h=1;
				m_bData.resize(4);
				m_bData[0]=type & 0x1;
				m_bData[1]=(type>>1) & 0x1;
				m_bData[2]=(type>>2) & 0x1;
				m_bData[3]=(type>>3) & 0x1;
				break;
			default:
				return false;
				break;
			}
		}
	}

	return true;
}

void PushableBlock::MySerialize(MySerializer& ar){
	ar.PutVUInt32(m_nType);
	ar.PutVUInt32(m_x);
	ar.PutVUInt32(m_y);

	if(m_nType!=ROTATE_BLOCK){
		ar.PutVUInt32(m_w);
		ar.PutVUInt32(m_h);
		assert(m_w*m_h==m_bData.size());
		for(unsigned int i=0;i<m_bData.size();i++){
			ar.PutIntN(m_bData[i],1);
		}
		ar.PutFlush();
	}else{
		assert(4==m_bData.size());
		for(unsigned int i=0;i<m_bData.size();i++){
			ar.PutInt8(m_bData[i]);
		}
	}
}

bool PushableBlock::Rotate(int bIsClockwise)
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

void PushableBlock::CreateSingle(int nType,int x,int y){
	m_nType=nType;
	m_x=x;
	m_y=y;
	m_w=1;
	m_h=1;

	switch(nType){
	case NORMAL_BLOCK:
	case TARGET_BLOCK:
		m_bData.resize(1);
		m_bData[0]=1;
		break;
	default:
		m_bData.resize(4);
		m_bData[0]=0;
		m_bData[1]=0;
		m_bData[2]=0;
		m_bData[3]=0;
		break;
	}
}

bool PushableBlock::HitTest(int x,int y) const
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


void PushableBlock::UnoptimizeSize(int nWidth,int nHeight){
	switch(m_nType){
	case NORMAL_BLOCK:
	case TARGET_BLOCK:
		{
			std::vector<unsigned char> bNewData;
			bNewData.resize(nWidth*nHeight);

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

			m_bData=bNewData;
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

bool PushableBlock::OptimizeSize()
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

			std::vector<unsigned char> bNewData;
			bNewData.resize(w2*h2);
			for(int j=0;j<h2;j++){
				for(int i=0;i<w2;i++){
					bNewData[j*w2+i]=m_bData[(j+nTop)*m_w+(i+nLeft)];
				}
			}

			m_bData=bNewData;
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

void PushableBlock::MoveBlockInEditMode(int dx,int dy)
{
	switch(m_nType){
	case NORMAL_BLOCK:
	case TARGET_BLOCK:
		{
			std::vector<unsigned char> bNewData;
			bNewData.resize(m_w*m_h);

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

			m_bData=bNewData;
		}
		break;
	default:
		m_x+=dx;
		m_y+=dy;
		break;
	}
}
