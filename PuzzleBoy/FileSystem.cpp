#include "FileSystem.h"
#include <string.h>
#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <direct.h>
#pragma comment(lib,"shlwapi.lib")
#else
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#endif

#include <SDL.h>

#define USE_SDL_RWOPS

using namespace std;

u8string externalStoragePath;

char* u8fgets(char* buf, int count, u8file* file){
#ifdef USE_SDL_RWOPS
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
#else
	return fgets(buf,count,(FILE*)file);
#endif
}

const char* u8fgets2(u8string& s,u8file* file){
	s.clear();

	if(file){
#ifdef USE_SDL_RWOPS
		char c;
		for(int i=0;;i++){
			if(u8fread(&c,1,1,file)==0) return (i>0)?s.c_str():NULL;
			s.push_back(c);
			if(c=='\n') return s.c_str();
		}
#else
		char buf[512];
		for(int i=0;;i++){
			if(fgets(buf,sizeof(buf),(FILE*)file)==NULL) return (i>0)?s.c_str():NULL;
			s.append(buf);
			if(!s.empty() && s[s.size()-1]=='\n') return s.c_str();
		}
#endif
	}

	return NULL;
}

u8file *u8fopen(const char* filename,const char* mode){
#ifdef USE_SDL_RWOPS
	return (u8file*)SDL_RWFromFile(filename,mode);
#else
#ifdef WIN32
	u16string filenameW=toUTF16(filename);
	u16string modeW=toUTF16(mode);
	return (u8file*)_wfopen((const wchar_t*)filenameW.c_str(),(const wchar_t*)modeW.c_str());
#else
	return (u8file*)fopen(filename,mode);
#endif
#endif
}

int u8fseek(u8file* file,long offset,int whence){
#ifdef USE_SDL_RWOPS
	if(file) return SDL_RWseek((SDL_RWops*)file,offset,whence)==-1?-1:0;
	return -1;
#else
	return fseek((FILE*)file,offset,whence);
#endif
}

long u8ftell(u8file* file){
#ifdef USE_SDL_RWOPS
	if(file) return (long)SDL_RWtell((SDL_RWops*)file);
	return -1;
#else
	return ftell((FILE*)file);
#endif
}

size_t u8fread(void* ptr,size_t size,size_t nmemb,u8file* file){
#ifdef USE_SDL_RWOPS
	if(file) return SDL_RWread((SDL_RWops*)file,ptr,size,nmemb);
	return 0;
#else
	return fread(ptr,size,nmemb,(FILE*)file);
#endif
}

size_t u8fwrite(const void* ptr,size_t size,size_t nmemb,u8file* file){
#ifdef USE_SDL_RWOPS
	if(file) return SDL_RWwrite((SDL_RWops*)file,ptr,size,nmemb);
	return 0;
#else
	return fwrite(ptr,size,nmemb,(FILE*)file);
#endif
}

int u8fclose(u8file* file){
#ifdef USE_SDL_RWOPS
	if(file) return SDL_RWclose((SDL_RWops*)file);
	return 0;
#else
	return fclose((FILE*)file);
#endif
}

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

void initPaths(){
#if defined(ANDROID)
	externalStoragePath=SDL_AndroidGetExternalStoragePath();
#elif defined(WIN32)
	const int size=65536;
	wchar_t *s=new wchar_t[size];
	SHGetSpecialFolderPathW(NULL,s,CSIDL_PERSONAL,1);
	externalStoragePath=toUTF8((const unsigned short*)s)+"/My Games/PuzzleBoy";
#else
	const char *env=getenv("HOME");
	if(env==NULL) externalStoragePath="local";
	else externalStoragePath=u8string(env)+"/.PuzzleBoy";
#endif
	if(externalStoragePath.empty()) return;

	//Create subfolders.
	createDirectory(externalStoragePath);
	createDirectory(externalStoragePath+"/levels");
}

bool createDirectory(const u8string& path){
#ifdef WIN32
	const int size=65536;
	wchar_t *s0=new wchar_t[size],*s=new wchar_t[size];

	GetCurrentDirectoryW(size,s0);
	PathCombineW(s,s0,(LPCWSTR)toUTF16(path).c_str());

	for(int i=0;i<size;i++){
		if(s[i]=='\0') break;
		else if(s[i]=='/') s[i]='\\';
	}

	bool ret=(SHCreateDirectoryExW(NULL,s,NULL)!=0);

	delete[] s0;
	delete[] s;

	return ret;
#else
	return mkdir(path.c_str(),0777)==0;
#endif
}
