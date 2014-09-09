#pragma once

#include "UTF8-16.h"

bool SimpleInputScreen(const u8string& title,const u8string& prompt,u8string& text,const char* allowedChars=0);

//-1 means cancel
int SimpleConfigKeyScreen(int key);

class ChangeSizeScreen{
public:
	ChangeSizeScreen(int w,int h);
	bool DoModal();
public:
	int m_nOldWidth,m_nOldHeight;
	int m_nXOffset,m_nYOffset,m_nWidth,m_nHeight;
	bool m_bPreserve;
};
