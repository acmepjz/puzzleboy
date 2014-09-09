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

//normal sizes
static const SDL_Rect RotateBlockOnlySizes[]={
	{6,6,1},{6,8,1},{6,10,1},{8,6,1},{8,8,1},{10,6,1},
	{5,5,2},{5,7,2},{7,5,2},{6,6,2},
};
const int RotateBlockOnlySizesCount=sizeof(RotateBlockOnlySizes)/sizeof(SDL_Rect);

const int MaxRandomTypes=RotateBlockOnlySizesCount;

//experimental sizes
static const SDL_Rect RotateBlockOnlySizes2[]={
	{8,10,1},{10,8,1},{10,10,1},
	{6,8,2},{8,6,2},
	{5,5,3},{5,7,3},{7,5,3},{6,6,3},
};
const int RotateBlockOnlySizesCount2=sizeof(RotateBlockOnlySizes2)/sizeof(SDL_Rect);

void RandomMapScreen::OnDirty(){
	ResetList();

	for(int i=0;i<RotateBlockOnlySizesCount;i++){
		MyFormat fmt(_("Rotate block only"));
		fmt(" %dx%d ")<<RotateBlockOnlySizes[i].x<<RotateBlockOnlySizes[i].y;
		if(RotateBlockOnlySizes[i].w>1){
			fmt(_("(%d players)"))<<RotateBlockOnlySizes[i].w;
		}
		AddItem(str(fmt));
	}
	AddItem(_("Random"));

	AddEmptyItem();

	for(int i=0;i<RotateBlockOnlySizesCount2;i++){
		MyFormat fmt(_("Rotate block only"));
		fmt(" %dx%d ")<<RotateBlockOnlySizes2[i].x<<RotateBlockOnlySizes2[i].y;
		if(RotateBlockOnlySizes2[i].w>1){
			fmt(_("(%d players)"))<<RotateBlockOnlySizes2[i].w;
		}
		AddItem(str(fmt));
	}
}

int RandomMapScreen::OnClick(int index){
	switch(index){
	case MaxRandomTypes+1:
		return -1;
	default:
		return index+1;
	}
}

int RandomMapScreen::DoModal(){
	//show
	m_titleBar.m_sTitle=_("Random Map");
	return SimpleListScreen::DoModal();
}

int RandomMapScreen::DoRandom(int type,PuzzleBoyLevelData*& outputLevel,MT19937 *rnd,void *userData,RandomLevelCallback callback){
	type--;
	if(type==MaxRandomTypes){
		type=int((float)MaxRandomTypes*(float)rnd->Rnd()/4294967296.0f);
	}

	int i=type;

	//normal sizes
	if(i>=0 && i<RotateBlockOnlySizesCount){
		return RandomTest(
			RotateBlockOnlySizes[i].x,
			RotateBlockOnlySizes[i].y,
			RotateBlockOnlySizes[i].w,
			outputLevel,rnd,userData,callback);
	}
	i-=RotateBlockOnlySizesCount;

	//experimental sizes
	i-=2;

	if(i>=0 && i<RotateBlockOnlySizesCount2){
		return RandomTest(
			RotateBlockOnlySizes2[i].x,
			RotateBlockOnlySizes2[i].y,
			RotateBlockOnlySizes2[i].w,
			outputLevel,rnd,userData,callback);
	}
	i-=RotateBlockOnlySizesCount2;

	//shouldn't goes here
	printf("[DoRandom] Error: Unknown random map type: %d\n",type); //not thread safe
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
