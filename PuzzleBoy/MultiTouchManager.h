#pragma once

#include <vector>
#include <map>

//NOTE: this class is only an interface, and can not be deleted
class MultiTouchView{
public:
	//fx,fy: position of new center in normalized coordinate: (w,h)-->(screenAspectRatio,1.0f)
	//dx,dy: relative motion in normalized coordinate
	//zoom: relative zoom factor
	virtual void OnMultiGesture(float fx,float fy,float dx,float dy,float zoom);

	//state: mouse button state
	//nFlags: SDL_GetModState()
	//nType: SDL_MOUSEBUTTONDOWN or SDL_MOUSEBUTTONUP or SDL_MOUSEMOTION
	virtual void OnMouseEvent(int which,int state,int xMouse,int yMouse,int nFlags,int nType);

	//nFlags: SDL_GetModState()
	virtual void OnMouseWheel(int which,int x,int y,int dx,int dy,int nFlags);
};

namespace MultiTouchViewFlags{
	const int AcceptDragging=0x1;
	const int AcceptZoom=0x2;

	const int Disabled=0x80000000;
}

struct SDL_Finger;

//all position are in normalized coordinate: (w,h)-->(screenAspectRatio,1.0f)
struct MultiTouchViewStruct{
	float left;
	float top;
	float right;
	float bottom;
	int flags;
	MultiTouchView* view; //weak reference, don't delete!

	//internal variables
	float m_fMultitouchOldX,m_fMultitouchOldY;
	float m_fMultitouchOldDistSquared;

	//0=not dragging or failed,1=mouse down,2=dragging,
	//3=multi-touch is running or failed (if m_fMultitouchOldDistSquared<0)
	int m_nDraggingState;

	//temp variable
	SDL_Finger *f0,*f1;
};

class MultiTouchManager{
public:
	MultiTouchManager();
	~MultiTouchManager();

	void clear();

	void ResetDraggingState();

	void AddView(float left,float top,float right,float bottom,int flags,MultiTouchView* view);
	MultiTouchViewStruct* FindView(MultiTouchView* view);
	void RemoveView(MultiTouchView* view);

	//ad-hoc function
	void DisableTemporarily(MultiTouchView* view);

	bool OnEvent();
private:
	std::vector<MultiTouchViewStruct> views;

	int m_nDraggingIndex; //valid only when mouse is used

	int HitTest(float x,float y);

	//x,y: position of new center in normalized coordinate: (w,h)-->(screenAspectRatio,1.0f)
	void CheckDragging(int index,float x,float y);

	//fingerID->view index
	//only add new finger when SDL_FINGERDOWN is called and hit test return true
	std::map<long long,int> fingers;
};

