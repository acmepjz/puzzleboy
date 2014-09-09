#pragma once

#include "UTF8-16.h"
#include <vector>

class SimpleText;

class SimpleTitleBarButton{
public:
	SimpleTitleBarButton(int index,int showIndex,const char* caption,
		int left,int top,int right,int bottom,
		float left2=0.0f,float top2=0.0f,float right2=0.0f,float bottom2=0.0f,bool visible=true);
	void Create(std::vector<float>& v,std::vector<unsigned short>& idx,SimpleText*& txt);
public:
	std::pair<int,float> m_position[4]; //left,top,right,bottom
	int m_x,m_y,m_w,m_h; //valid after Create()
	int m_index;
	int m_showIndex; //-1 means AddEmptyHorizontalButton
	int m_visible;
	u8string m_sCaption;
};

class SimpleTitleBar{
public:
	SimpleTitleBar();
	~SimpleTitleBar();
	void Create();

	bool OnTimer();

	//return value:
	//>=0 clicked index
	//-1 no button clicked but event processed
	//-2 event not processed
	int OnEvent();

	void Draw();
public:
	u8string m_sTitle;
	std::vector<int> m_LeftButtons;
	std::vector<int> m_RightButtons;
	std::vector<SimpleTitleBarButton> m_AdditionalButtons;
private:
	SimpleText *m_txtTitle;
	SimpleText *m_txtAdditional;
	std::vector<float> m_v;
	std::vector<unsigned short> m_idx;
	int m_nMyResizeTime;
	int m_pressedKey; //key index, >=0 index -1 non-button area -2 none
};
