#pragma once

#include "UTF8-16.h"

//title: the title
//promot: the prompt
//record [in,out]: the level record
//readOnly: read only, e.g. solver mode
//return value: 0=cancel 1=apply 2=demo
int LevelRecordScreen(const u8string& title,const u8string& prompt,u8string& record,bool readOnly=false);
