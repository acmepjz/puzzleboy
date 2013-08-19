//A simple and buggy implementation of GNU GetText

#include "GNUGetText.h"
#include "FileSystem.h"

#include <stdlib.h>
#include <string.h>
#include <SDL_endian.h>

#ifdef WIN32
#include <windows.h>
#endif

bool GNUGetText::LoadFileWithAutoLocale(const u8string& sFileName){
	size_t nReplaceIndex;
	if((nReplaceIndex=sFileName.find_first_of('*'))==u8string::npos){
		return LoadFile(sFileName);
	}

	char buf[256];

	char *s=getenv("LANG");
	if(s==NULL) buf[0]=0;
	else strncpy(buf,s,sizeof(buf));

	if(buf[0]==0){
		s=getenv("LANGUAGE");
		if(s) strncpy(buf,s,sizeof(buf));
	}

#ifdef WIN32
	if(buf[0]==0){
		int i=GetLocaleInfoA(LOCALE_USER_DEFAULT,LOCALE_SISO639LANGNAME,buf,sizeof(buf));
		if(i>0 && i<sizeof(buf)){
			int j=GetLocaleInfoA(LOCALE_USER_DEFAULT,LOCALE_SISO3166CTRYNAME,buf+i,sizeof(buf)-i);
			if(j>0) buf[i-1]='_';
		}
	}
#endif

	int lps=0;

	for(;;){
		if(buf[lps]==0) break;

		int lpe=lps;

		while(buf[lpe]!=';' && buf[lpe]!=',' && buf[lpe]!=':' && buf[lpe]!=0) lpe++;
		bool bExit=(buf[lpe]==0);
		buf[lpe]=0;

		u8string str(buf+lps);
		u8string fn(sFileName);

		fn.replace(nReplaceIndex,1,str);
		if(LoadFile(fn)) return true;

		size_t i=str.find_first_of('.');
		if(i!=str.npos){
			fn=sFileName;
			fn.replace(nReplaceIndex,1,str.substr(0,i));
			if(LoadFile(fn)) return true;
		}

		i=str.find_first_of('@');
		if(i!=str.npos){
			fn=sFileName;
			fn.replace(nReplaceIndex,1,str.substr(0,i));
			if(LoadFile(fn)) return true;
		}

		i=str.find_first_of('_');
		if(i!=str.npos){
			fn=sFileName;
			fn.replace(nReplaceIndex,1,str.substr(0,i));
			if(LoadFile(fn)) return true;
		}

		if(bExit) break;
		lps=lpe+1;
	}

	return false;
}

bool GNUGetText::LoadFile(const u8string& sFileName){
	u8file *f=u8fopen(sFileName.c_str(),"rb");
	if(f==NULL) return false;

	bool ret=false;

	int header[5];
	u8fread(header,20,1,f);
#if SDL_BYTEORDER==SDL_BIG_ENDIAN
	for(int i=0;i<5;i++) header[i]=SDL_SwapLE32(header[i]);
#endif

	if(header[0]==0x950412DE && header[1]==0){
		m_objString.clear();

		if(header[2]>0 && header[3]>0 && header[4]>0){
			std::vector<int> OriginalString,TranslatedString;
			OriginalString.resize(header[2]*2);
			TranslatedString.resize(header[2]*2);

			u8fseek(f,header[3],SEEK_SET);
			u8fread(&(OriginalString[0]),header[2]*8,1,f);
			u8fseek(f,header[4],SEEK_SET);
			u8fread(&(TranslatedString[0]),header[2]*8,1,f);

			for(int i=0;i<header[2];i++){
				u8string s1,s2;

				int length,offset;

				if((length=SDL_SwapLE32(OriginalString[i*2]))>0
					&& (offset=SDL_SwapLE32(OriginalString[i*2+1]))>0)
				{
					u8fseek(f,offset,SEEK_SET);
					s1.resize(length);
					u8fread(&(s1[0]),length,1,f);
				}

				if((length=SDL_SwapLE32(TranslatedString[i*2]))>0
					&& (offset=SDL_SwapLE32(TranslatedString[i*2+1]))>0)
				{
					u8fseek(f,offset,SEEK_SET);
					s2.resize(length);
					u8fread(&(s2[0]),length,1,f);
				}

				m_objString[s1]=s2;
			}
		}

		ret=true;
	}

	u8fclose(f);

	return ret;
}

u8string GNUGetText::GetText(const u8string& s) const{
	std::map<u8string,u8string>::const_iterator it=m_objString.find(s);

	if(it==m_objString.end()) return s;
	else return it->second;
}
