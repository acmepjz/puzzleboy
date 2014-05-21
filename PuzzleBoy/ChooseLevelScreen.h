#pragma once

#include "SimpleListScreen.h"

class ChooseLevelFileScreen:public SimpleListScreen{
public:
	virtual void OnDirty();
	virtual int OnClick(int index);
	virtual int DoModal();
private:
	std::vector<u8string> m_files;
	std::vector<u8string> m_fileDisplayName;
};

class ChooseLevelScreen:public SimpleListScreen{
public:
	virtual void OnDirty();
	virtual int OnClick(int index);
	virtual int OnTitleBarButtonClick(int index);
	virtual int DoModal();
private:
	std::vector<int> m_searchResult;
	u8string m_searchFilter;
	std::vector<int> m_bestStep;
};
