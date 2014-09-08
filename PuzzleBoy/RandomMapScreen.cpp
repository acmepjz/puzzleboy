#include "RandomMapScreen.h"
#include "PuzzleBoyApp.h"
#include "PuzzleBoyLevelFile.h"
#include "SimpleText.h"
#include "SimpleProgressScreen.h"
#include "MT19937.h"
#include "MyFormat.h"
#include "main.h"

#include <stdio.h>

#include "include_sdl.h"

void RandomMapScreen::OnDirty(){
	ResetList();

	AddItem(_("Rotate block only 6x6"));
	AddItem(_("Rotate block only 8x6"));
	AddItem(_("Rotate block only 8x8"));

	AddItem(_("Random"));
}

int RandomMapScreen::OnClick(int index){
	return index+1;
}

int RandomMapScreen::DoModal(){
	//show
	m_titleBar.m_sTitle=_("Random Map");
	return SimpleListScreen::DoModal();
}

int RandomMapScreen::DoRandom(int type,PuzzleBoyLevelData*& outputLevel,MT19937 *rnd,void *userData,RandomLevelCallback callback){
	const int MaxRandomTypes=3;

	type--;
	if(type<0 || type>=MaxRandomTypes){
		type=int((float)MaxRandomTypes*(float)rnd->Rnd()/4294967296.0f);
	}

	switch(type){
		case 0: return RandomTest(6,6,outputLevel,rnd,userData,callback);
		case 1: return RandomTest(8,6,outputLevel,rnd,userData,callback);
		case 2: return RandomTest(8,8,outputLevel,rnd,userData,callback);
	}

	//shouldn't goes here
	return 0;
}

struct RandomLevelBatchProgress{
	int type;
	int nCount;
	float progress;
	volatile int *lpCurrent;
	volatile int *lpNextUnprocessed;
	volatile bool *lpbAbort;
	SDL_mutex *mutex;
	PuzzleBoyLevelFile *doc;
	SimpleProgressScreen *progressScreen; ///< only used when single-threaded
	MT19937 *rnd;
};

static int TestRandomLevelCallback(void* userData,float progress){
	RandomLevelBatchProgress *t=(RandomLevelBatchProgress*)userData;

	if(t->mutex){
		//multi-threaded
		t->progress=progress;
		if(*(t->lpbAbort)) return 1;
	}else{
		//single-threaded
		t->progressScreen->progress=(float(*(t->lpCurrent))+progress)/float(t->nCount);
		if(!t->progressScreen->DrawAndDoEvents()){
			*(t->lpbAbort)=true;
			return 1;
		}
	}

	return 0;
}

static int TestRandomLevelThreadFunc(void* userData){
	RandomLevelBatchProgress *t=(RandomLevelBatchProgress*)userData;

	if(t->mutex) SDL_LockMutex(t->mutex);

	for(;;){
		//get next level
		int i=(*(t->lpNextUnprocessed))++;
		if(t->mutex) SDL_UnlockMutex(t->mutex);

		if(i>=t->nCount) break;

		//create random level
		PuzzleBoyLevelData *level=NULL;

		if(!RandomMapScreen::DoRandom(t->type,level,t->rnd,userData,TestRandomLevelCallback)) break;

		//save level
		if(t->mutex) SDL_LockMutex(t->mutex);

		if(t->nCount>1){
			level->m_sLevelName=toUTF16(str(MyFormat(_("Random Level %d"))<<int(t->doc->m_objLevels.size()+1)));
		}else{
			level->m_sLevelName=toUTF16(_("Random Level"));
		}

		t->doc->m_objLevels.push_back(level);

		(*(t->lpCurrent))++;
		t->progress=0.0f;
	}

	return 0;
}

PuzzleBoyLevelFile* RandomMapScreen::DoRandomLevels(int type,int levelCount){
	if(levelCount<=0) return NULL;

#ifdef RANDOM_MAP_PROFILING
	Uint32 t=SDL_GetTicks();
#endif

	//determine thread count
	int threadCount=theApp->m_nThreadCount;
	if(threadCount<=0) threadCount=SDL_GetCPUCount();
	if(threadCount<1) threadCount=1;
	else if(threadCount>8) threadCount=8;
	if(threadCount>levelCount) threadCount=levelCount;

	//create levels
	PuzzleBoyLevelFile* doc=new PuzzleBoyLevelFile();
	doc->m_sLevelPackName=toUTF16(_("Random Level"));

	//create progress screen
	SimpleProgressScreen progressScreen;
	progressScreen.Create();

	SDL_mutex *mutex=NULL;
	if(threadCount>1) mutex=SDL_CreateMutex();

	volatile int nCurrent=0;
	volatile int nNextUnprocessed=0;
	volatile bool bAbort=false;

	RandomLevelBatchProgress prog[8];
	for(int i=0;i<threadCount;i++){
		prog[i].type=type;
		prog[i].nCount=levelCount;
		prog[i].progress=0.0f;
		prog[i].lpCurrent=&nCurrent;
		prog[i].lpNextUnprocessed=&nNextUnprocessed;
		prog[i].lpbAbort=&bAbort;
		prog[i].mutex=mutex;
		prog[i].doc=doc;
		prog[i].progressScreen=&progressScreen;

		MT19937 *rnd=new MT19937;
		unsigned int seed[16];
		for(int j=0;j<16;j++){
			seed[j]=theApp->m_objMainRnd.Rnd();
		}
		rnd->Init(seed,sizeof(seed)/sizeof(unsigned int));

		prog[i].rnd=rnd;
	}

	//start thread
	if(threadCount>1){
		SDL_Thread *thread[8];
		for(int i=0;i<threadCount;i++){
			thread[i]=SDL_CreateThread(TestRandomLevelThreadFunc,NULL,&(prog[i]));
		}

		for(;;){
			SDL_Delay(30);

			if(nCurrent>=levelCount) break;

			float progress=(float)nCurrent;
			for(int i=0;i<threadCount;i++){
				progress+=prog[i].progress;
			}

			progressScreen.progress=progress/(float)levelCount;
			if(!progressScreen.DrawAndDoEvents()){
				bAbort=true;
				break;
			}
		}

		for(int i=0;i<threadCount;i++){
			SDL_WaitThread(thread[i],NULL);
		}
	}else{
		TestRandomLevelThreadFunc(&(prog[0]));
	}

	//over
	if(mutex) SDL_DestroyMutex(mutex);

	for(int i=0;i<threadCount;i++){
		delete prog[i].rnd;
	}

#ifdef RANDOM_MAP_PROFILING
	//print statistics
	printf("[DoRandomLevels] Create %d random level(s) in %dms\n",doc->m_objLevels.size(),SDL_GetTicks()-t);
#endif

	if(doc->m_objLevels.empty()){
		delete doc;
		return NULL;
	}else{
		return doc;
	}
}
