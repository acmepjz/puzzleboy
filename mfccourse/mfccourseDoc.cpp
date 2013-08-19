// mfccourseDoc.cpp : CmfccourseDoc 类的实现
//

#include "stdafx.h"
#include "mfccourse.h"

#include "mfccourseDoc.h"
#include "PushableBlock.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <string>
#include <map>

#define S_COPYRIGHT L"\x00A9"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CmfccourseDoc

IMPLEMENT_DYNCREATE(CmfccourseDoc, CDocument)

BEGIN_MESSAGE_MAP(CmfccourseDoc, CDocument)
END_MESSAGE_MAP()


// CmfccourseDoc 构造/析构

CmfccourseDoc::CmfccourseDoc()
{
	// TODO: 在此添加一次性构造代码
}

CmfccourseDoc::~CmfccourseDoc()
{
	for(int i=0;i<m_objLevels.GetSize();i++) delete m_objLevels[i];
}

BOOL CmfccourseDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	m_sLevelPackName=_T("未命名关卡包");
	for(int i=0;i<m_objLevels.GetSize();i++) delete m_objLevels[i];
	m_objLevels.RemoveAll();
	m_objLevels.Add(new CLevel);
	((CLevel*)m_objLevels[0])->CreateDefault();

	return TRUE;
}




// CmfccourseDoc 序列化

void CmfccourseDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar<<m_sLevelPackName;
		m_objLevels.Serialize(ar);
	}
	else
	{
		for(int i=0;i<m_objLevels.GetSize();i++) delete m_objLevels[i];
		m_objLevels.RemoveAll();

		ar>>m_sLevelPackName;
		m_objLevels.Serialize(ar);
	}
}


// CmfccourseDoc 诊断

#ifdef _DEBUG
void CmfccourseDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CmfccourseDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CmfccourseDoc 命令

CLevel* CmfccourseDoc::GetLevel(int nIndex)
{
	if(nIndex>=0 && nIndex<m_objLevels.GetSize())
		return (CLevel*)(m_objLevels[nIndex]);
	else
		return NULL;
}

#if _UNICODE
typedef std::wstring tstring;
#else
typedef std::string tstring;
#endif

bool CmfccourseDoc::LoadSokobanLevel(const CString& fileName,bool bClear){
	FILE *f;
	char s[512];
	std::map<std::string,std::string> names;

	if(bClear){
		m_sLevelPackName=_T("未命名关卡包");
		for(int i=0;i<m_objLevels.GetSize();i++) delete m_objLevels[i];
		m_objLevels.RemoveAll();
		SetModifiedFlag();
	}

#if _UNICODE
	f=_wfopen(fileName,L"rt");
#else
	f=fopen(fileName,"rt");
#endif

	if(!f) return false;

	//get file name
	tstring packName(fileName);
	{
		int lps;
		lps=packName.find_last_of(_T("\\/"));
		if(lps!=tstring::npos) packName=packName.substr(lps+1);
		lps=packName.find_first_of(_T('.'));
		if(lps!=tstring::npos) packName=packName.substr(0,lps);
	}

	CString levelName;
	std::vector<std::vector<unsigned char> > levelMap;

	for(;;){
		bool bRead=fgets(s,sizeof(s),f)!=NULL;
		bool bReadLine=false;
		int m=0;

		if(bRead){
			m=strlen(s);
			for(int i=0;i<m;i++){
				if(s[i]=='\r' || s[i]=='\n') s[i]='\0';
			}
			m=strlen(s);

			//YASC: comments
			if(s[0]==':' && s[1]==':') continue;

			//WinSoko: define author names
			if(s[0]=='@'){
				int lps=1,lpe;
				for(;lps<m;lps++){
					if(s[lps]==' ') break;
				}
				for(;lps<m;lps++){
					if(s[lps]!=' ' && s[lps]!='\"') break;
					s[lps]='\0';
				}
				for(lpe=lps;lpe<m;lpe++){
					if(s[lpe]=='\"'){
						s[lpe]='\0';
						break;
					}
				}
				if(lps<m){
					names[s+1]=(s+lps);
					continue;
				}
			}

			//ini file (?) MapLineXX=XXX
			if(m>=9 && tolower(s[0])=='m'
				&& tolower(s[1])=='a'
				&& tolower(s[2])=='p'
				&& tolower(s[3])=='l'
				&& tolower(s[4])=='i'
				&& tolower(s[5])=='n'
				&& tolower(s[6])=='e')
			{
				int lps=8;
				for(;lps<m;lps++){
					if(s[lps]=='=') break;
				}

				const unsigned char table[8]={
					FLOOR_TILE,FLOOR_TILE,WALL_TILE,'$',
					TARGET_TILE,PLAYER_TILE,PLAYER_AND_TARGET_TILE,'*'
				};
				std::vector<unsigned char> row;
				for(int i=lps+1;lps<m;i++){
					int c=s[i]-'0';
					
					if(c<0 || c>7) break;
					row.push_back(table[c]);
				}

				if(!row.empty()){
					levelMap.push_back(row);
					continue;
				}
			}

			//try to read line. RLE unsupported
			do{
				int lps,lpe;
				for(lps=0;lps<m;lps++){
					if(s[lps]!=' ' && s[lps]!='-' && s[lps]!='_') break;
				}
				if(s[lps]!='#' && s[lps]!='B' && s[lps]!='*') break;
				for(lpe=m-1;lpe>=0;lpe--){
					if(s[lpe]!=' ' && s[lpe]!='-' && s[lpe]!='_') break;
				}
				if(lpe<0 || (s[lpe]!='#' && s[lpe]!='B' && s[lpe]!='*')) break;

				bReadLine=true;

				std::vector<unsigned char> row;
				for(int i=0;i<=lpe;i++){
					switch(s[i]){
					case ' ':
					case '-':
					case '_':
						row.push_back(0);
						break;
					case '#':
						row.push_back(WALL_TILE);
						break;
					case 'p':
					case '@':
						row.push_back(PLAYER_TILE);
						break;
					case 'P':
					case '+':
						row.push_back(PLAYER_AND_TARGET_TILE);
						break;
					case 'b':
					case '$':
						row.push_back('$');
						break;
					case 'B':
					case '*':
						row.push_back('*');
						break;
					case '.':
						row.push_back(TARGET_TILE);
						break;
					default:
						bReadLine=false;
						break;
					}
				}

				if(bReadLine) levelMap.push_back(row);
			}while(false);

			if(bReadLine) continue;
		}

		if(!bReadLine && !levelMap.empty()){
			//calc level width
			int nLeft=0x7FFFFFFF,nRight=0x80000000;
			for(int j=0;j<(int)levelMap.size();j++){
				for(int i=0;i<(int)levelMap[j].size();i++){
					if(levelMap[j][i]){
						if(nLeft>i) nLeft=i;
						if(nRight<i) nRight=i;
					}
				}
			}
			assert(nLeft<=nRight);

			//create a new level
			CLevel *lev=new CLevel;
			lev->Create(nRight-nLeft+1,levelMap.size());
			lev->m_sLevelName=levelName;

			for(int j=0;j<(int)levelMap.size();j++){
				for(int i=nLeft;i<=nRight;i++){
					if(i>=(int)levelMap[j].size()){
						(*lev)(i-nLeft,j)=0;
					}else{
						unsigned char c=levelMap[j][i];
						if(c=='$' || c=='*'){
							CPushableBlock *block=new CPushableBlock;
							block->CreateSingle(TARGET_BLOCK,i-nLeft,j);
							lev->m_objBlocks.Add(block);
							c=(c=='*')?TARGET_TILE:0;
						}

						(*lev)(i-nLeft,j)=c;
					}
				}
			}

			m_objLevels.Add(lev);
			SetModifiedFlag();

			levelMap.clear();
		}

		if(bRead && !bReadLine){
			//WinSoko: level name
			if(s[0]=='>'){
				int lps=1,lpe;
				for(;lps<m;lps++){
					if(s[lps]!=' ') break;
				}
				for(;lps<m;lps++){
					if(s[lps]==' ') break;
				}
				for(;lps<m;lps++){
					if(s[lps]!=' ') break;
				}
				for(lpe=lps;lpe<m;lpe++){
					if(s[lpe]==' ') break;
				}
				std::string author;
				if(lpe>lps){
					author=names[std::string(s+lps,lpe-lps)];
				}

				levelName=packName.c_str();
				s[lps]='\0';
				levelName+=(s+1);
				levelName+=(s+lpe);
				if(!author.empty()){
					levelName+=L" " S_COPYRIGHT L" ";
					levelName+=author.c_str();
				}

				continue;
			}

			//YASC level name (TODO)
			for(int i=0;i<m;i++){
				if(s[i]!=' '){
					levelName=s+i;
					break;
				}
			}
		}

		if(!bRead) break;
	}

	fclose(f);

	if(m_objLevels.IsEmpty()){
		m_objLevels.Add(new CLevel);
		((CLevel*)m_objLevels[0])->CreateDefault();
		SetModifiedFlag();
	}

	return true;
}
