#pragma once

#include "SimpleListScreen.h"

class ConfigScreen:public SimpleListScreen{
public:
	virtual void OnDirty();
	virtual int OnClick(int index);
	virtual int DoModal();
public:
	bool m_bConfigDirty;
};
