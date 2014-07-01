#pragma once

#include "SimpleListScreen.h"

class ChooseLevelFileScreen:public SimpleListScreen{
public:
	void OnDirty() override;
	int OnClick(int index) override;
	int DoModal() override;
private:
	std::vector<u8string> m_files;
	std::vector<u8string> m_fileDisplayName;
};

class ChooseLevelScreen:public SimpleListScreen{
public:
	void OnDirty() override;
	int OnClick(int index) override;
	int OnTitleBarButtonClick(int index) override;
	int DoModal() override;
private:
	std::vector<int> m_searchResult;
	u8string m_searchFilter;
	std::vector<int> m_bestStep;
};
