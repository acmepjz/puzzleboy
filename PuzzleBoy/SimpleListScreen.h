#pragma once

#include "SimpleTitleBar.h"
#include "UTF8-16.h"
#include <vector>

class SimpleText;
class SimpleMessageBox;

class SimpleListScreen{
public:
	SimpleListScreen();
	virtual ~SimpleListScreen();

	virtual void OnDirty();
	virtual int OnClick(int index);
	virtual int OnTitleBarButtonClick(int index);
	virtual int DoModal();
	virtual int OnMsgBoxClick(int index);

	void EnsureVisible(int index);
	void EnsureVisible(){EnsureVisible(m_nListIndex);}

	int ListCount() const {return m_nListCount;}

protected:
	void ResetList();

	void AddEmptyItem();
	void AddItem(const u8string& str,bool newIndex=true,float x=0.0f,float w=0.0f,int extraFlags=0);

	void RecreateTitleBar(){m_titleBar.Create();}
private:
	SimpleText *m_txtList;
	int m_nListCount;
public:
	SimpleTitleBar m_titleBar;

	int m_nListIndex; //0-based
	int m_nReturnValue;

	bool m_bDirty;
	bool m_bDirtyOnResize;

protected:
	SimpleMessageBox *m_msgBox;

private:
	int m_y0;
};

class SimpleStaticListScreen:public SimpleListScreen{
public:
	void OnDirty() override;
	int OnClick(int index) override;
public:
	std::vector<u8string> m_sList;
};
