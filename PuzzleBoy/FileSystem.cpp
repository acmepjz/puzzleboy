#include "FileSystem.h"
#include <string.h>
#ifdef WIN32
#include <windows.h>
#else
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#endif

using namespace std;

#ifdef USE_SDL_RWOPS
char* u8fgets(char* buf, int count, u8file* file){
	for(int i=0;i<count-1;i++){
		if(u8fread(buf+i,1,1,file)==0){
			buf[i]=0;
			return (i>0)?buf:NULL;
		}
		if(buf[i]=='\n'){
			buf[i+1]=0;
			return buf;
		}
	}

	buf[count-1]=0;
	return buf;
}
#else
#ifdef WIN32
u8file *u8fopen(const char* filename,const char* mode){
	u16string filenameW=toUTF16(filename);
	u16string modeW=toUTF16(mode);
	return _wfopen((const wchar_t*)filenameW.c_str(),(const wchar_t*)modeW.c_str());
}
#endif
#endif

std::vector<u8string> enumAllFiles(u8string path,const char* extension,bool containsPath){
	vector<u8string> v;
#ifdef WIN32
	WIN32_FIND_DATAW f;

	if(!path.empty()){
		char c=path[path.size()-1];
		if(c!='/' && c!='\\') path+="\\";
	}

	HANDLE h=NULL;
	{
		u8string s1=path;
		if(extension!=NULL && *extension){
			s1+="*.";
			s1+=extension;
		}else{
			s1+="*";
		}
		u16string s1b=toUTF16(s1);
		h=FindFirstFileW((LPCWSTR)s1b.c_str(),&f);
	}

	if(h==NULL || h==INVALID_HANDLE_VALUE) return v;

	do{
		if(!(f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
			u8string s2=toUTF8((const unsigned short*)f.cFileName);
			if(containsPath){
				v.push_back(path+s2);
			}else{
				v.push_back(s2);
			}
		}
	}while(FindNextFileW(h,&f));

	FindClose(h);

	return v;
#else
	int len=0;
	if(extension!=NULL && *extension) len=strlen(extension);
	if(!path.empty()){
		char c=path[path.size()-1];
		if(c!='/'&&c!='\\') path+="/";
	}
	DIR *pDir;
	struct dirent *pDirent;
	pDir=opendir(path.c_str());
	if(pDir==NULL) return v;
	while((pDirent=readdir(pDir))!=NULL){
		if(pDirent->d_name[0]=='.'){
			if(pDirent->d_name[1]==0||
				(pDirent->d_name[1]=='.'&&pDirent->d_name[2]==0)) continue;
		}
		string s1=path+pDirent->d_name;
		struct stat S_stat;
		lstat(s1.c_str(),&S_stat);
		if(!S_ISDIR(S_stat.st_mode)){
			if(len>0){
				if((int)s1.size()<len+1) continue;
				if(s1[s1.size()-len-1]!='.') continue;
				if(strcasecmp(&s1[s1.size()-len],extension)) continue;
			}

			if(containsPath){
				v.push_back(s1);
			}else{
				v.push_back(string(pDirent->d_name));
			}
		}
	}
	closedir(pDir);
	return v;
#endif
}

std::vector<u8string> enumAllDirs(u8string path,bool containsPath){
	vector<u8string> v;
#ifdef WIN32
	WIN32_FIND_DATAW f;

	if(!path.empty()){
		char c=path[path.size()-1];
		if(c!='/' && c!='\\') path+="\\";
	}

	HANDLE h=NULL;
	{
		u16string s1b=toUTF16(path);
		s1b.push_back('*');
		h=FindFirstFileW((LPCWSTR)s1b.c_str(),&f);
	}

	if(h==NULL || h==INVALID_HANDLE_VALUE) return v;

	do{
		// skip '.' and '..' and hidden folders
		if(f.cFileName[0]=='.'){
			/*if(f.cFileName[1]==0||
				(f.cFileName[1]=='.'&&f.cFileName[2]==0))*/ continue;
		}
		if(f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
			u8string s2=toUTF8((const unsigned short*)f.cFileName);
			if(containsPath){
				v.push_back(path+s2);
			}else{
				v.push_back(s2);
			}
		}
	}while(FindNextFileW(h,&f));

	FindClose(h);

	return v;
#else
	if(!path.empty()){
		char c=path[path.size()-1];
		if(c!='/'&&c!='\\') path+="/";
	}
	DIR *pDir;
	struct dirent *pDirent;
	pDir=opendir(path.c_str());
	if(pDir==NULL) return v;
	while((pDirent=readdir(pDir))!=NULL){
		if(pDirent->d_name[0]=='.'){
			if(pDirent->d_name[1]==0||
				(pDirent->d_name[1]=='.'&&pDirent->d_name[2]==0)) continue;
		}
		string s1=path+pDirent->d_name;
		struct stat S_stat;
		lstat(s1.c_str(),&S_stat);
		if(S_ISDIR(S_stat.st_mode)){
			//Skip hidden folders.
			s1=string(pDirent->d_name);
			if(s1.find('.')==0) continue;
			
			//Add result to vector.
			if(containsPath){
				v.push_back(path+pDirent->d_name);
			}else{
				v.push_back(s1);
			}
		}
	}
	closedir(pDir);
	return v;
#endif
}