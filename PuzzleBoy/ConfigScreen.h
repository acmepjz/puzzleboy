#pragma once

#include "SimpleListScreen.h"

class ConfigScreen:public SimpleListScreen{
public:
	void OnDirty() override;
	int OnClick(int index) override;
	int DoModal() override;
public:
	bool m_bConfigDirty;
};
