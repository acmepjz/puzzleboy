#pragma once

#include "MultiTouchManager.h"
#include "SimpleScrollView.h"
#include "UTF8-16.h"

#include <SDL_events.h>

#include <vector>

struct SimpleTextBoxCharacter;
class SimpleTextBoxScreenKeyboard;

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

	void CopyToClipboard() const;
	void PasteFromClipboard();

	void RegisterView(MultiTouchManager& mgr);

	void OnMultiGesture(float fx,float fy,float dx,float dy,float zoom) override;
	void OnMouseEvent(int which,int state,int xMouse,int yMouse,int nFlags,int nType) override;
	void OnMouseWheel(int which,int x,int y,int dx,int dy,int nFlags) override;

	bool OnEvent();

	void Draw();
	void DrawOverlay();

	//should be called before SetText or other similiar functions
	void SetAllowedChars(const char* allowedChars);

public:
	SimpleScrollView m_scrollView;

	bool m_bLocked;

private:
	static SimpleTextBox* m_objFocus;

	SimpleTextBoxScreenKeyboard *m_keyboard;

	unsigned char m_bAllowedChars[128-32]; //only 32-127, if [0]==0xFF then disabled

	bool IsCharAllowed(int c) const{
		return m_bAllowedChars[0]==0xFF || (c>=32 && c<=127 && m_bAllowedChars[c-32]);
	}

	std::vector<SimpleTextBoxCharacter> m_chars;

	int m_caretPos;
	int m_caretTimer;

	bool m_caretDirty;

	static char m_IMEText[SDL_TEXTEDITINGEVENT_TEXT_SIZE];
	static int m_IMETextCaret;
};
