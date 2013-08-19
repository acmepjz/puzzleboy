// mfccourseDoc.h : CmfccourseDoc ��Ľӿ�
//


#pragma once

#include "Level.h"

class CmfccourseDoc : public CDocument
{
protected: // �������л�����
	CmfccourseDoc();
	DECLARE_DYNCREATE(CmfccourseDoc)

// ����
public:

// ����
public:
	CLevel* GetLevel(int nIndex);
	bool LoadSokobanLevel(const CString& fileName,bool bClear);

// ��д
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

// ʵ��
public:
	virtual ~CmfccourseDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// ���ɵ���Ϣӳ�亯��
protected:
	DECLARE_MESSAGE_MAP()
public:
	CString m_sLevelPackName;
	CObArray m_objLevels;
};


