#pragma once

#include "UTF8-16.h"
#include <vector>

class SimpleText;

class SimpleMessageBox{
public:
	SimpleMessageBox();
	~SimpleMessageBox();
	void Create();
	//bool OnTimer();
	bool OnEvent();
	void Draw();
public:
	intptr_t m_nUserData;
	int m_nDefaultValue;
	int m_nCancelValue;
	int m_nValue;
	u8string m_prompt;
	std::vector<u8string> m_buttons;
private:
	SimpleText *m_txtPrompt;
	SimpleText *m_txtButtons;
	int m_x,m_y; ///< position of top-left corner of first button
	float m_textWidth,m_textHeight;
	std::vector<float> m_v;
	std::vector<unsigned short> m_idx;
	int m_nPressedValue;
};
