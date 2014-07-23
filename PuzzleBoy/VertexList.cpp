#include "VertexList.h"

#include <stdlib.h>
#include <math.h>

#include "include_gl.h"

const int VertexList::Points=0x0000;

const int VertexList::Lines=0x0001;
const int VertexList::LineLoop=0x0002;
const int VertexList::LineStrip=0x0003;

const int VertexList::Triangles=0x0004;
const int VertexList::TriangleStrip=0x0005;
const int VertexList::TriangleFan=0x0006;

const int VertexList::Quads=0x0007;
const int VertexList::QuadStrip=0x0008;

const int VertexList::Rectangles=0x1001;

VertexList::VertexList(bool hasNormal,bool isIndexed,int type)
:m_nType(type),m_bHasNormal(hasNormal),m_bIndexed(isIndexed),m_nVertexArraySize(0),m_nVertexCount(0)
{
}

void VertexList::Create(bool hasNormal,bool isIndexed,int type){
	m_nType=type;
	m_bHasNormal=hasNormal;
	m_bIndexed=isIndexed;
	m_nVertexArraySize=0;
	m_nVertexCount=0;

	m_Vertices.clear();
	m_nIndex.clear();
}

void VertexList::clear(){
	m_nVertexArraySize=0;
	m_nVertexCount=0;

	m_Vertices.clear();
	m_nIndex.clear();
}

VertexList::~VertexList(){
}

void VertexList::Draw() const{
	if(m_nVertexCount<=0 || m_Vertices.empty()) return;

	glEnableClientState(GL_VERTEX_ARRAY);

	if(m_bHasNormal){
		glEnableClientState(GL_NORMAL_ARRAY);
		glVertexPointer(2,GL_FLOAT,5*sizeof(float),&(m_Vertices[0]));
		glNormalPointer(GL_FLOAT,5*sizeof(float),&(m_Vertices[2]));
	}else{
		glVertexPointer(2,GL_FLOAT,0,&(m_Vertices[0]));
	}

	if(m_bIndexed){
		glDrawElements(m_nType,m_nVertexCount,GL_UNSIGNED_SHORT,&(m_nIndex[0]));
	}else{
		glDrawArrays(m_nType,0,m_nVertexCount);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}

void VertexList::AddPoint(float x,float y){
	AddVertex(x,y);

	if(m_bIndexed){
		m_nIndex.push_back(m_nVertexArraySize);
		m_nVertexArraySize++;
	}

	m_nVertexCount++;
}

void VertexList::AddLine(float x1,float y1,float x2,float y2){
	AddVertex(x1,y1);
	AddVertex(x2,y2);

	if(m_bIndexed){
		m_nIndex.push_back(m_nVertexArraySize);
		m_nIndex.push_back(m_nVertexArraySize+1);
		m_nVertexArraySize+=2;
	}

	m_nVertexCount+=2;
}

void VertexList::AddLineStrip(float x,float y){
	if(m_nVertexCount<=0){
		bool b=m_Vertices.empty();

		AddVertex(x,y);

		if(!b){
			if(m_bIndexed){
				m_nIndex.push_back(0);
				m_nIndex.push_back(1);
				m_nVertexArraySize=2;
			}

			m_nVertexCount=2;
		}
	}else{
		if(m_bIndexed){
			m_nIndex.push_back(m_nVertexArraySize-1);
			m_nIndex.push_back(m_nVertexArraySize);
			m_nVertexArraySize++;
		}else{
			int i=m_Vertices.size()-2;
			if(m_bHasNormal) i-=3;

			AddVertex(m_Vertices[i],m_Vertices[i+1]);
		}

		AddVertex(x,y);

		m_nVertexCount+=2;
	}
}

void VertexList::AddTriangle(float x1,float y1,float x2,float y2,float x3,float y3,float nx,float ny,float nz){
	switch(m_nType){
	case Lines:
		if(m_bIndexed){
			AddVertex(x1,y1,nx,ny,nz);
			AddVertex(x2,y2,nx,ny,nz);
			AddVertex(x3,y3,nx,ny,nz);
			m_nIndex.push_back(m_nVertexArraySize);
			m_nIndex.push_back(m_nVertexArraySize+1);
			m_nIndex.push_back(m_nVertexArraySize+1);
			m_nIndex.push_back(m_nVertexArraySize+2);
			m_nIndex.push_back(m_nVertexArraySize+2);
			m_nIndex.push_back(m_nVertexArraySize);
			m_nVertexArraySize+=3;
		}else{
			AddVertex(x1,y1,nx,ny,nz);
			AddVertex(x2,y2,nx,ny,nz);
			AddVertex(x2,y2,nx,ny,nz);
			AddVertex(x3,y3,nx,ny,nz);
			AddVertex(x3,y3,nx,ny,nz);
			AddVertex(x1,y1,nx,ny,nz);
		}
		m_nVertexCount+=6;
		break;
	case Triangles:
		AddVertex(x1,y1,nx,ny,nz);
		AddVertex(x2,y2,nx,ny,nz);
		AddVertex(x3,y3,nx,ny,nz);

		if(m_bIndexed){
			m_nIndex.push_back(m_nVertexArraySize);
			m_nIndex.push_back(m_nVertexArraySize+1);
			m_nIndex.push_back(m_nVertexArraySize+2);
			m_nVertexArraySize+=3;
		}

		m_nVertexCount+=3;
		break;
	}
}

void VertexList::AddQuad(float x1,float y1,float x2,float y2,float x3,float y3,float x4,float y4,float nx,float ny,float nz){
	switch(m_nType){
	case Lines:
		if(m_bIndexed){
			AddVertex(x1,y1,nx,ny,nz);
			AddVertex(x2,y2,nx,ny,nz);
			AddVertex(x3,y3,nx,ny,nz);
			AddVertex(x4,y4,nx,ny,nz);
			m_nIndex.push_back(m_nVertexArraySize);
			m_nIndex.push_back(m_nVertexArraySize+1);
			m_nIndex.push_back(m_nVertexArraySize+1);
			m_nIndex.push_back(m_nVertexArraySize+2);
			m_nIndex.push_back(m_nVertexArraySize+2);
			m_nIndex.push_back(m_nVertexArraySize+3);
			m_nIndex.push_back(m_nVertexArraySize+3);
			m_nIndex.push_back(m_nVertexArraySize);
			m_nVertexArraySize+=4;
		}else{
			AddVertex(x1,y1,nx,ny,nz);
			AddVertex(x2,y2,nx,ny,nz);
			AddVertex(x2,y2,nx,ny,nz);
			AddVertex(x3,y3,nx,ny,nz);
			AddVertex(x3,y3,nx,ny,nz);
			AddVertex(x4,y4,nx,ny,nz);
			AddVertex(x4,y4,nx,ny,nz);
			AddVertex(x1,y1,nx,ny,nz);
		}
		m_nVertexCount+=8;
		break;
	case Triangles:
		if(m_bIndexed){
			AddVertex(x1,y1,nx,ny,nz);
			AddVertex(x2,y2,nx,ny,nz);
			AddVertex(x3,y3,nx,ny,nz);
			AddVertex(x4,y4,nx,ny,nz);

			m_nIndex.push_back(m_nVertexArraySize);
			m_nIndex.push_back(m_nVertexArraySize+1);
			m_nIndex.push_back(m_nVertexArraySize+2);
			m_nIndex.push_back(m_nVertexArraySize);
			m_nIndex.push_back(m_nVertexArraySize+2);
			m_nIndex.push_back(m_nVertexArraySize+3);

			m_nVertexArraySize+=4;
		}else{
			AddVertex(x1,y1,nx,ny,nz);
			AddVertex(x2,y2,nx,ny,nz);
			AddVertex(x3,y3,nx,ny,nz);
			AddVertex(x1,y1,nx,ny,nz);
			AddVertex(x3,y3,nx,ny,nz);
			AddVertex(x4,y4,nx,ny,nz);
		}
		m_nVertexCount+=6;
		break;
	}
}

int VertexList::AddPrimitives(int type,const float *p,const float *n,int nPointCount,bool uniformNormal){
	float nx=0.0f,ny=0.0f,nz=1.0f;

	if(uniformNormal && n){
		nx=n[0];ny=n[1];nz=n[2];n=NULL;
	}

	switch(type){
	case Points:
		if(m_bHasNormal){
			for(int i=0;i<nPointCount;i++){
				if(n){
					nx=n[0];ny=n[1];nz=n[2];n+=3;
				}
				AddVertex(p[0],p[1],nx,ny,nz);
				p+=2;
			}
		}else{
			m_Vertices.insert(m_Vertices.end(),p,p+nPointCount*2);
		}
		if(m_bIndexed){
			for(int i=0;i<nPointCount;i++){
				m_nIndex.push_back(m_nVertexArraySize+i);
			}
			m_nVertexArraySize+=nPointCount;
		}
		m_nVertexCount+=nPointCount;
		return nPointCount;
		break;

	case Lines:
		nPointCount/=2;
		if(m_bHasNormal){
			for(int i=0;i<nPointCount;i++){
				if(n){
					nx=n[0];ny=n[1];nz=n[2];n+=3;
				}
				AddVertex(p[0],p[1],nx,ny,nz);
				AddVertex(p[2],p[3],nx,ny,nz);
				p+=4;
			}
		}else{
			m_Vertices.insert(m_Vertices.end(),p,p+nPointCount*4);
		}
		nPointCount*=2;
		if(m_bIndexed){
			for(int i=0;i<nPointCount;i++){
				m_nIndex.push_back(m_nVertexArraySize+i);
			}
			m_nVertexArraySize+=nPointCount;
		}
		m_nVertexCount+=nPointCount;
		return nPointCount;
		break;

	case LineStrip:
	case LineLoop:
		{
			if(nPointCount<2) return 0;
			if(m_bIndexed){
				if(m_bHasNormal){
					//FIXME: normal
					for(int i=0;i<nPointCount;i++){
						AddVertex(p[0],p[1],nx,ny,nz);
						p+=2;
					}
				}else{
					m_Vertices.insert(m_Vertices.end(),p,p+nPointCount*2);
				}
				for(int i=0;i<nPointCount-1;i++){
					m_nIndex.push_back(m_nVertexArraySize+i);
					m_nIndex.push_back(m_nVertexArraySize+i+1);
				}
				if(nPointCount>=3 && type==LineLoop){
					m_nIndex.push_back(m_nVertexArraySize+nPointCount-1);
					m_nIndex.push_back(m_nVertexArraySize);
					m_nVertexCount+=2;
				}
				m_nVertexArraySize+=nPointCount;
			}else{
				for(int i=0;i<nPointCount-1;i++){
					if(n){
						nx=n[0];ny=n[1];nz=n[2];n+=3;
					}
					AddVertex(p[i*2],p[i*2+1],nx,ny,nz);
					AddVertex(p[i*2+2],p[i*2+3],nx,ny,nz);
				}
				if(nPointCount>=3 && type==LineLoop){
					if(n){
						nx=n[0];ny=n[1];nz=n[2];n+=3;
					}
					AddVertex(p[nPointCount*2-2],p[nPointCount*2-1],nx,ny,nz);
					AddVertex(p[0],p[1],nx,ny,nz);
					m_nVertexCount+=2;
				}
			}
			m_nVertexCount+=(nPointCount-1)*2;
		}
		return nPointCount;
		break;

	case Triangles:
		nPointCount/=3;
		if(m_bHasNormal){
			for(int i=0;i<nPointCount;i++){
				if(n){
					nx=n[0];ny=n[1];nz=n[2];n+=3;
				}
				AddVertex(p[0],p[1],nx,ny,nz);
				AddVertex(p[2],p[3],nx,ny,nz);
				AddVertex(p[4],p[5],nx,ny,nz);
				p+=6;
			}
		}else{
			m_Vertices.insert(m_Vertices.end(),p,p+nPointCount*6);
		}
		nPointCount*=3;
		if(m_bIndexed){
			for(int i=0;i<nPointCount;i++){
				m_nIndex.push_back(m_nVertexArraySize+i);
			}
			m_nVertexArraySize+=nPointCount;
		}
		m_nVertexCount+=nPointCount;
		return nPointCount;
		break;

	case TriangleStrip:
	case TriangleFan:
		{
			if(nPointCount<3) return 0;
			if(m_bIndexed){
				if(m_bHasNormal){
					//FIXME: normal
					for(int i=0;i<nPointCount;i++){
						AddVertex(p[0],p[1],nx,ny,nz);
						p+=2;
					}
				}else{
					m_Vertices.insert(m_Vertices.end(),p,p+nPointCount*2);
				}
				if(type==TriangleStrip){
					for(int i=0;i<nPointCount-2;i++){
						m_nIndex.push_back(m_nVertexArraySize+i+(i&1));
						m_nIndex.push_back(m_nVertexArraySize+i+((i&1)^1));
						m_nIndex.push_back(m_nVertexArraySize+i+2);
					}
				}else{
					for(int i=0;i<nPointCount-2;i++){
						m_nIndex.push_back(m_nVertexArraySize);
						m_nIndex.push_back(m_nVertexArraySize+i+1);
						m_nIndex.push_back(m_nVertexArraySize+i+2);
					}
				}
				m_nVertexArraySize+=nPointCount;
			}else{
				if(type==TriangleStrip){
					for(int i=0;i<nPointCount-2;i++){
						if(n){
							nx=n[0];ny=n[1];nz=n[2];n+=3;
						}
						int j=i+(i&1);
						AddVertex(p[j*2],p[j*2+1],nx,ny,nz);
						j=i+((i&1)^1);
						AddVertex(p[j*2],p[j*2+1],nx,ny,nz);
						AddVertex(p[i*2+4],p[i*2+5],nx,ny,nz);
					}
				}else{
					for(int i=0;i<nPointCount-2;i++){
						if(n){
							nx=n[0];ny=n[1];nz=n[2];n+=3;
						}
						AddVertex(p[0],p[1],nx,ny,nz);
						AddVertex(p[i*2+2],p[i*2+3],nx,ny,nz);
						AddVertex(p[i*2+4],p[i*2+5],nx,ny,nz);
					}
				}
			}
			m_nVertexCount+=(nPointCount-2)*3;
		}
		return nPointCount;
		break;

	case Quads:
		nPointCount/=4;
		if(m_bIndexed){
			if(m_bHasNormal){
				for(int i=0;i<nPointCount;i++){
					if(n){
						nx=n[0];ny=n[1];nz=n[2];n+=3;
					}
					AddVertex(p[0],p[1],nx,ny,nz);
					AddVertex(p[2],p[3],nx,ny,nz);
					AddVertex(p[4],p[5],nx,ny,nz);
					AddVertex(p[6],p[7],nx,ny,nz);
					p+=8;
				}
			}else{
				m_Vertices.insert(m_Vertices.end(),p,p+nPointCount*8);
			}
			for(int i=0;i<nPointCount;i++){
				m_nIndex.push_back(m_nVertexArraySize+i*4);
				m_nIndex.push_back(m_nVertexArraySize+i*4+1);
				m_nIndex.push_back(m_nVertexArraySize+i*4+2);
				m_nIndex.push_back(m_nVertexArraySize+i*4);
				m_nIndex.push_back(m_nVertexArraySize+i*4+2);
				m_nIndex.push_back(m_nVertexArraySize+i*4+3);
			}
			m_nVertexArraySize+=nPointCount*4;
		}else{
			for(int i=0;i<nPointCount;i++){
				if(n){
					nx=n[0];ny=n[1];nz=n[2];n+=3;
				}
				AddVertex(p[0],p[1],nx,ny,nz);
				AddVertex(p[2],p[3],nx,ny,nz);
				AddVertex(p[4],p[5],nx,ny,nz);
				AddVertex(p[0],p[1],nx,ny,nz);
				AddVertex(p[4],p[5],nx,ny,nz);
				AddVertex(p[6],p[7],nx,ny,nz);
				p+=8;
			}
		}
		m_nVertexCount+=nPointCount*6;
		return nPointCount*4;
		break;

	case QuadStrip:
		{
			nPointCount/=2;
			if(nPointCount<2) return 0;
			if(m_bIndexed){
				if(m_bHasNormal){
					//FIXME: normal
					for(int i=0;i<nPointCount;i++){
						AddVertex(p[0],p[1],nx,ny,nz);
						AddVertex(p[2],p[3],nx,ny,nz);
						p+=4;
					}
				}else{
					m_Vertices.insert(m_Vertices.end(),p,p+nPointCount*4);
				}
				for(int i=0;i<nPointCount-1;i++){
					m_nIndex.push_back(m_nVertexArraySize+i*2);
					m_nIndex.push_back(m_nVertexArraySize+i*2+1);
					m_nIndex.push_back(m_nVertexArraySize+i*2+2);
					m_nIndex.push_back(m_nVertexArraySize+i*2+1);
					m_nIndex.push_back(m_nVertexArraySize+i*2+3);
					m_nIndex.push_back(m_nVertexArraySize+i*2+2);
				}
				m_nVertexArraySize+=nPointCount*4;
			}else{
				for(int i=0;i<nPointCount-1;i++){
					if(n){
						nx=n[0];ny=n[1];nz=n[2];n+=3;
					}
					AddVertex(p[i*2],p[i*2+1],nx,ny,nz);
					AddVertex(p[i*2+2],p[i*2+3],nx,ny,nz);
					AddVertex(p[i*2+4],p[i*2+5],nx,ny,nz);
					AddVertex(p[i*2+2],p[i*2+3],nx,ny,nz);
					AddVertex(p[i*2+6],p[i*2+7],nx,ny,nz);
					AddVertex(p[i*2+4],p[i*2+5],nx,ny,nz);
				}
			}
			m_nVertexCount+=(nPointCount-1)*6;
		}
		return nPointCount*2;
		break;

	case Rectangles:
		nPointCount/=2;
		if(m_bIndexed){
			for(int i=0;i<nPointCount;i++){
				if(n){
					nx=n[0];ny=n[1];nz=n[2];n+=3;
				}
				AddVertex(p[0],p[1],nx,ny,nz);
				AddVertex(p[2],p[1],nx,ny,nz);
				AddVertex(p[2],p[3],nx,ny,nz);
				AddVertex(p[0],p[3],nx,ny,nz);
				p+=4;
			}
			for(int i=0;i<nPointCount;i++){
				m_nIndex.push_back(m_nVertexArraySize+i*4);
				m_nIndex.push_back(m_nVertexArraySize+i*4+1);
				m_nIndex.push_back(m_nVertexArraySize+i*4+2);
				m_nIndex.push_back(m_nVertexArraySize+i*4);
				m_nIndex.push_back(m_nVertexArraySize+i*4+2);
				m_nIndex.push_back(m_nVertexArraySize+i*4+3);
			}
			m_nVertexArraySize+=nPointCount*4;
		}else{
			float nx=0.0f,ny=0.0f,nz=1.0f;
			for(int i=0;i<nPointCount;i++){
				if(n){
					nx=n[0];ny=n[1];nz=n[2];n+=3;
				}
				AddVertex(p[0],p[1],nx,ny,nz);
				AddVertex(p[2],p[1],nx,ny,nz);
				AddVertex(p[2],p[3],nx,ny,nz);
				AddVertex(p[0],p[1],nx,ny,nz);
				AddVertex(p[2],p[3],nx,ny,nz);
				AddVertex(p[0],p[3],nx,ny,nz);
				p+=4;
			}
		}
		m_nVertexCount+=nPointCount*6;
		return nPointCount*2;
		break;

	default:
		return 0;
		break;
	}
}

void VertexList::AddEllipse(float x0,float y0,float cx,float cy,float angle,int nCount,float nx,float ny,float nz){
	std::vector<float> p;
	float n[3]={nx,ny,nz};

	float ss=sin(angle),cc=cos(angle);
	float a=0.0f,d=3.14159265359f*2.0f/nCount;

	for(int i=0;i<nCount;i++){
		float x=cx*cos(a),y=cy*sin(a);
		p.push_back(x0+x*cc-y*ss);
		p.push_back(y0+x*ss+y*cc);
		a+=d;
	}

	AddPrimitives(m_nType==Triangles?TriangleFan:LineLoop,&(p[0]),n,nCount,true);
}

void VertexList::AddHemisphere(float x0,float y0,float cx,float cy,float cz,float angle,int nCount,int nCount2){
	if(!m_bHasNormal){
		AddEllipse(x0,y0,cx,cy,angle,nCount);
		return;
	}

	std::vector<float> p;

	float ss=sin(angle),cc=cos(angle);
	float d=3.14159265359f*2.0f/nCount;
	float a2=0.0f,d2=3.14159265359f*0.5f/nCount2;

	for(int j=0;j<nCount2;j++){
		float a=0.0f,s2=sin(a2),c2=cos(a2);
		for(int i=0;i<nCount;i++){
			float s=c2*sin(a),c=c2*cos(a);
			float x=cx*c,y=cy*s;

			float nx=c/cx,ny=s/cy,nz=s2/cz;
			s=1.0f/sqrt(nx*nx+ny*ny+nz*nz);

			p.push_back(x0+x*cc-y*ss);
			p.push_back(y0+x*ss+y*cc);
			p.push_back((nx*cc-ny*ss)*s);
			p.push_back((nx*ss+ny*cc)*s);
			p.push_back(nz*s);

			a+=d;
		}
		a2+=d2;
	}

	p.push_back(x0);
	p.push_back(y0);
	p.push_back(0.0f);
	p.push_back(0.0f);
	p.push_back(1.0f);

	if(m_bIndexed){
		m_Vertices.insert(m_Vertices.end(),p.begin(),p.end());

		for(int i=0;i<nCount;i++){
			int ii=i+1;
			if(ii>=nCount) ii=0;

			for(int j=0;;j++){
				m_nIndex.push_back(m_nVertexArraySize+j*nCount+i);
				m_nIndex.push_back(m_nVertexArraySize+j*nCount+ii);
				if(j>=nCount2-1){
					m_nIndex.push_back(m_nVertexArraySize+nCount*nCount2);
					break;
				}
				m_nIndex.push_back(m_nVertexArraySize+(j+1)*nCount+ii);
				m_nIndex.push_back(m_nVertexArraySize+j*nCount+i);
				m_nIndex.push_back(m_nVertexArraySize+(j+1)*nCount+ii);
				m_nIndex.push_back(m_nVertexArraySize+(j+1)*nCount+i);
			}
		}

		m_nVertexArraySize+=nCount*nCount2+1;
	}else{
		for(int i=0;i<nCount;i++){
			int ii=i+1;
			if(ii>=nCount) ii=0;

			for(int j=0;;j++){
				int idx=j*nCount+i;
				m_Vertices.insert(m_Vertices.end(),p.begin()+idx*5,p.begin()+idx*5+5);
				idx=j*nCount+ii;
				m_Vertices.insert(m_Vertices.end(),p.begin()+idx*5,p.begin()+idx*5+5);
				if(j>=nCount2-1){
					idx=nCount*nCount2;
					m_Vertices.insert(m_Vertices.end(),p.begin()+idx*5,p.begin()+idx*5+5);
					break;
				}
				idx=(j+1)*nCount+ii;
				m_Vertices.insert(m_Vertices.end(),p.begin()+idx*5,p.begin()+idx*5+5);
				idx=j*nCount+i;
				m_Vertices.insert(m_Vertices.end(),p.begin()+idx*5,p.begin()+idx*5+5);
				idx=(j+1)*nCount+ii;
				m_Vertices.insert(m_Vertices.end(),p.begin()+idx*5,p.begin()+idx*5+5);
				idx=(j+1)*nCount+i;
				m_Vertices.insert(m_Vertices.end(),p.begin()+idx*5,p.begin()+idx*5+5);
			}
		}
	}

	m_nVertexCount+=nCount*(nCount2*2-1)*3;
}
