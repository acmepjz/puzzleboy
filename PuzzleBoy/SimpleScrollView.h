#pragma once

#include <SDL_rect.h>

namespace SimpleScrollViewFlags{
	const int Horizontal=0x1;
	const int Vertical=0x2;
	const int Both=Horizontal|Vertical;
	const int Zoom=0x4;
	const int BothAndZoom=Both|Zoom;
	const int DrawScrollBar=0x8;
	const int AutoResize=0x10;
}

class SimpleScrollView{
public:
	SimpleScrollView();

	bool OnTimer();

	void SetProjectionMatrix();

	void EnableScissorRect();
	static void DisableScissorRect();

	//fx,fy: position of new center in normalized coordinate: (w,h)-->(screenAspectRatio,1.0f)
	//dx,dy: relative motion in normalized coordinate
	//zoom: relative zoom factor
	void OnMultiGesture(float fx,float fy,float dx,float dy,float zoom);

	//translate screen coordinate to world
	void TranslateCoordinate(int x,int y,int& out_x,int& out_y);
	void TranslateCoordinate(float x,float y,float& out_x,float& out_y);

	void TranslateScreenCoordinateToClient(int x,int y,int& out_x,int& out_y);

	void CenterView(int x=0,int y=0,int w=-1,int h=-1);
	void EnsureVisible(int x,int y,int w=0,int h=0);

	void ConstraintView(bool zoom);

	void Draw();
public:
	//virtual area
	SDL_Rect m_virtual;
	//screen area
	SDL_Rect m_screen;
	//flags
	int m_flags;
	float m_fAutoResizeScale[4];
	int m_nAutoResizeOffset[4];
	//minimal zoom factor (maximal zoom in)
	float m_fMinZoomPerScreen;

	//0=down 1=right 2=up 3=left
	int m_nOrientation;

	int m_nScissorOffset[4];
private:
	//target pos, x,y: coord of top-left corner (in absolute screen coord!), zoom: size per pixel
	float m_zoom,m_x,m_y;
	//currently animation pos
	float m_zoom2,m_x2,m_y2;

	int m_nMyResizeTime;
	int m_nScrollBarIdleTime;
};

