#pragma once

#include "PuzzleBoyLevelData.h"
#include "MFCSerializer.h"
#include "UTF8-16.h"
#include <vector>

class PuzzleBoyLevelFile
{
public:
	PuzzleBoyLevelFile();

	PuzzleBoyLevelData* GetLevel(int nIndex){
		if(nIndex>=0 && nIndex<(int)m_objLevels.size())
			return m_objLevels[nIndex];
		else
			return NULL;
	}

	bool LoadSokobanLevel(const u8string& fileName,bool bClear);

	bool MFCSerialize(MFCSerializer& ar);

	void CreateNew();

	virtual ~PuzzleBoyLevelFile();

	void SetModifiedFlag(){m_bModified=true;}

public:
	u16string m_sLevelPackName;
	std::vector<PuzzleBoyLevelData*> m_objLevels;

	bool m_bModified;
};


