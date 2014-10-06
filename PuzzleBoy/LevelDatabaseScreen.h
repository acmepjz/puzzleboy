#pragma once

#include "SimpleListScreen.h"
#include "RecordManager.h"

class LevelDatabaseScreen:public SimpleListScreen{
public:
	void OnDirty() override;
	int OnClick(int index) override;
	int OnTitleBarButtonClick(int index) override;
	int DoModal() override;
	int OnMsgBoxClick(int index) override;
private:
	void FilterDirty();
private:
	RecordFile m_tFile;
	RecordLevelChecksum m_tSelected;
	std::vector<int> m_searchResult;
	u8string m_searchFilter;
};
