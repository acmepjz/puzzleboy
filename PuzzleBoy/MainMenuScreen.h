#pragma once

#include "SimpleListScreen.h"

class MainMenuScreen:public SimpleListScreen{
public:
	void OnDirty() override;
	int OnClick(int index) override;
	int DoModal() override;
};

class EditMenuScreen:public SimpleListScreen{
public:
	void OnDirty() override;
	int OnClick(int index) override;
	int OnMsgBoxClick(int index) override;
	int DoModal() override;
};

class TestMenuScreen:public SimpleListScreen{
public:
	void OnDirty() override;
	int OnClick(int index) override;
	int DoModal() override;
};

class NetworkMainMenuScreen:public SimpleListScreen{
public:
	void OnDirty() override;
	int OnClick(int index) override;
	int DoModal() override;
};
