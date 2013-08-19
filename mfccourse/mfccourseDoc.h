// mfccourseDoc.h : CmfccourseDoc 类的接口
//


#pragma once

#include "Level.h"

class CmfccourseDoc : public CDocument
{
protected: // 仅从序列化创建
	CmfccourseDoc();
	DECLARE_DYNCREATE(CmfccourseDoc)

// 属性
public:

// 操作
public:
	CLevel* GetLevel(int nIndex);
	bool LoadSokobanLevel(const CString& fileName,bool bClear);

// 重写
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

// 实现
public:
	virtual ~CmfccourseDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 生成的消息映射函数
protected:
	DECLARE_MESSAGE_MAP()
public:
	CString m_sLevelPackName;
	CObArray m_objLevels;
};


