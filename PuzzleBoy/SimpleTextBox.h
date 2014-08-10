#pragma once

#include "MultiTouchManager.h"
#include "SimpleScrollView.h"
#include "UTF8-16.h"

#include <SDL_events.h>

#include <vector>

struct SimpleTextBoxCharacter{
	int c;
	float x; //in pixels
	int row;
};

class SimpleTextBox:public virtual MultiTouchView{
public:
	SimpleTextBox();
	~SimpleTextBox();

	bool OnTimer();

	void SetText(const u8string& text);
	void GetText(u8string& text) const;

	void SetMultiline(bool multiline,bool wrap=false);

	static SimpleTextBox* GetFocus(){return m_objFocus;}
	bool HasFocus() const{return this==m_objFocus;}
	void SetFocus();
	static void ClearFocus();

	void RegisterView(MultiTouchManager& mgr);

	void OnMultiGesture(float fx,float fy,float dx,float dy,float zoom) override;
	void OnMouseEvent(int which,int state,int xMouse,int yMouse,int nFlags,int nType) override;
	void OnMouseWheel(int which,int x,int y,int dx,int dy,int nFlags) override;

	bool OnEvent();

	void Draw();

public:
	SimpleScrollView m_scrollView;

	bool m_bLocked;

private:
	static SimpleTextBox* m_objFocus;

	std::vector<SimpleTextBoxCharacter> m_chars;

	int m_caretPos;
	int m_caretTimer;

	bool m_caretDirty;

	static char m_IMEText[SDL_TEXTEDITINGEVENT_TEXT_SIZE];
	static int m_IMETextCaret;
};
