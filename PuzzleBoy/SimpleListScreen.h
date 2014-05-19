#pragma once

#include "UTF8-16.h"
#include <vector>

class SimpleText;

class SimpleListScreen{
public:
	SimpleListScreen();
	virtual ~SimpleListScreen();

	virtual void OnDirty();
	virtual int OnClick(int index);
	virtual int OnTitleBarButtonClick(int index);

	void CreateTitleBarText(const u8string& title);
	void CreateTitleBarButtons();

	virtual int DoModal();
public:
	SimpleText *m_txtTitle;
	SimpleText *m_txtList;
	int m_nListCount;
	int m_nReturnValue;

	bool m_bDirty;
	bool m_bDirtyOnResize;

	std::vector<int> m_LeftButtons;
	std::vector<int> m_RightButtons;
protected:
	//for title bar buttons
	std::vector<float> m_v;
	std::vector<unsigned short> m_idx;

	int m_nMyResizeTime;

	//0=not dragging or failed,1=mouse down,2=dragging,3=dragging scrollbar or failed
	int m_nDraggingState;
	float m_fOldY;
};
