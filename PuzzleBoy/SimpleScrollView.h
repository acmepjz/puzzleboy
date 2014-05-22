#pragma once

#include <SDL_rect.h>

class SimpleScrollView{
public:
	SimpleScrollView();

	bool OnTimer();

	//0=down 1=right 2=up 3=left
	void SetProjectionMatrix();

	void EnableScissorRect();
	static void DisableScissorRect();

	//fx,fy: position of new center in normalized coordinate: (w,h)-->(screenAspectRatio,1.0f)
	//dx,dy: relative motion in normalized coordinate
	//zoom: relative zoom factor
	void OnMultiGesture(float fx,float fy,float dx,float dy,float zoom);

	void TranslateCoordinate(int x,int y,int& out_x,int& out_y);

	void CenterView(int x=0,int y=0,int w=-1,int h=-1);
	void EnsureVisible(int x,int y,int w=0,int h=0);

	void ConstraintView(bool zoom);

	void DrawScrollBar();
public:
	//virtual area
	SDL_Rect m_virtual;
	//screen area
	SDL_Rect m_screen;
	//auto resize?
	bool m_bAutoResize;
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

