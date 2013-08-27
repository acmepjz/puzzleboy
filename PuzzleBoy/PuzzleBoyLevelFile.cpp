#include "PuzzleBoyLevelFile.h"
#include "PushableBlock.h"
#include "FileSystem.h"
#include "PuzzleBoyApp.h"

#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <vector>
#include <string>
#include <map>

//#define S_COPYRIGHT L"\x00A9"
#define S_COPYRIGHT_U8 "\xC2\xA9"

PuzzleBoyLevelFile::PuzzleBoyLevelFile()
{
	m_bModified=false;
}

PuzzleBoyLevelFile::~PuzzleBoyLevelFile()
{
	for(unsigned int i=0;i<m_objLevels.size();i++) delete m_objLevels[i];
}

void PuzzleBoyLevelFile::CreateNew()
{
	m_sLevelPackName=toUTF16(_("Unnamed level pack"));
	for(unsigned int i=0;i<m_objLevels.size();i++) delete m_objLevels[i];
	m_objLevels.clear();
	m_objLevels.push_back(new PuzzleBoyLevelData);
	m_objLevels[0]->CreateDefault();
}

bool PuzzleBoyLevelFile::MFCSerialize(MFCSerializer& ar)
{
	if (ar.IsStoring())
	{
		ar<<m_sLevelPackName;
		ar.PutObjectArray(m_objLevels,"CLevel",2);
	}
	else
	{
		for(unsigned int i=0;i<m_objLevels.size();i++) delete m_objLevels[i];
		m_objLevels.clear();

		ar>>m_sLevelPackName;
		if(ar.IsError()) return false;
		if(!ar.GetObjectArray(m_objLevels,"CLevel")) return false;
	}
	return true;
}

bool PuzzleBoyLevelFile::LoadSokobanLevel(const u8string& fileName,bool bClear){
	u8file *f;
	char s[512];
	std::map<std::string,std::string> names;

	if(bClear){
		m_sLevelPackName=toUTF16(_("Unnamed level pack"));
		for(unsigned int i=0;i<m_objLevels.size();i++) delete m_objLevels[i];
		m_objLevels.clear();
		SetModifiedFlag();
	}

	f=u8fopen(fileName.c_str(),"rt");

	if(!f) return false;

	//get file name
	u8string packName(fileName);
	{
		int lps;
		lps=packName.find_last_of("\\/");
		if(lps!=u8string::npos) packName=packName.substr(lps+1);
		lps=packName.find_first_of('.');
		if(lps!=u8string::npos) packName=packName.substr(0,lps);
	}

	u8string levelName;
	std::vector<std::vector<unsigned char> > levelMap;

	for(;;){
		bool bRead=u8fgets(s,sizeof(s),f)!=NULL;
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
			PuzzleBoyLevelData *lev=new PuzzleBoyLevelData;
			lev->Create(nRight-nLeft+1,levelMap.size());
			lev->m_sLevelName=toUTF16(levelName);

			for(int j=0;j<(int)levelMap.size();j++){
				for(int i=nLeft;i<=nRight;i++){
					if(i>=(int)levelMap[j].size()){
						(*lev)(i-nLeft,j)=0;
					}else{
						unsigned char c=levelMap[j][i];
						if(c=='$' || c=='*'){
							PushableBlock *block=new PushableBlock;
							block->CreateSingle(TARGET_BLOCK,i-nLeft,j);
							lev->m_objBlocks.push_back(block);
							c=(c=='*')?TARGET_TILE:0;
						}

						(*lev)(i-nLeft,j)=c;
					}
				}
			}

			m_objLevels.push_back(lev);
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

				levelName=packName;
				s[lps]='\0';
				levelName+=(s+1);
				levelName+=(s+lpe);
				if(!author.empty()){
					levelName+=" " S_COPYRIGHT_U8 " ";
					levelName+=author;
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

	u8fclose(f);

	if(m_objLevels.empty()){
		m_objLevels.push_back(new PuzzleBoyLevelData);
		m_objLevels[0]->CreateDefault();
		SetModifiedFlag();
	}

	return true;
}
