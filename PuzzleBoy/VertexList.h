#pragma once

#include <vector>

class VertexList{
public:
	static const int Points;
	static const int Lines;
	static const int Triangles;

	static const int LineLoop;
	static const int LineStrip;

	static const int TriangleStrip;
	static const int TriangleFan;

	static const int Quads;
	static const int QuadStrip;

	static const int Rectangles;
public:
	VertexList(bool hasNormal=false,bool isIndexed=false,int type=Triangles);
	~VertexList();

	void Create(bool hasNormal=false,bool isIndexed=false,int type=Triangles);

	void Draw() const;

	bool HasNormal() const {return m_bHasNormal;}
	bool IsIndexed() const {return m_bIndexed;}
	bool empty() const {return m_nVertexCount<=0;}
	int VertexCount() const {return m_nVertexCount;}
	int Type() const {return m_nType;}

	void clear();

	//warning: no sanity check, only for points
	void AddPoint(float x,float y);

	//warning: no sanity check, only for lines
	void AddLine(float x1,float y1,float x2,float y2);
	void AddLineStrip(float x,float y);

	//warning: no sanity check, only for lines and triangles
	void AddTriangle(float x1,float y1,float x2,float y2,float x3,float y3,float nx=0.0f,float ny=0.0f,float nz=1.0f);
	void AddQuad(float x1,float y1,float x2,float y2,float x3,float y3,float x4,float y4,float nx=0.0f,float ny=0.0f,float nz=1.0f);
	void AddRectangle(float x1,float y1,float x2,float y2,float nx=0.0f,float ny=0.0f,float nz=1.0f){
		AddQuad(x1,y1,x2,y1,x2,y2,x1,y2,nx,ny,nz);
	}

	//warning: no sanity check
	//the normal is per primitive, not per vertex,
	//and only works properly for points, lines, triangles, quads and rectangles
	//return value: point count processed
	int AddPrimitives(int type,const float *p,const float *n,int nPointCount,bool uniformNormal=false);

	//warning: no sanity check, only for lines and triangles
	void AddEllipse(float x0,float y0,float cx,float cy,float angle,int nCount,float nx=0.0f,float ny=0.0f,float nz=1.0f);

	//warning: no sanity check, only for triangles
	void AddHemisphere(float x0,float y0,float cx,float cy,float cz,float angle,int nCount,int nCount2);
private:
	bool m_bHasNormal;
	bool m_bIndexed;
	int m_nType;
	int m_nVertexArraySize;
	int m_nVertexCount;

	std::vector<float> m_Vertices;
	std::vector<unsigned short> m_nIndex;

	//internal function
	void AddVertex(float x,float y,float nx=0.0f,float ny=0.0f,float nz=1.0f){
		m_Vertices.push_back(x);
		m_Vertices.push_back(y);
		if(m_bHasNormal){
			m_Vertices.push_back(nx);
			m_Vertices.push_back(ny);
			m_Vertices.push_back(nz);
		}
	}
};
