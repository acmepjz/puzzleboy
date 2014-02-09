#include "clsTiming.h"
#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#endif

//timing function

clsTiming::clsTiming():t1(0.0),t2(0.0),freq(0.0),run(false){
#ifdef WIN32
	LARGE_INTEGER a;
	QueryPerformanceFrequency(&a);
	freq=(double)a.QuadPart;
#else
	freq=1000000.0;
#endif
}

void clsTiming::Clear(){
	t1=0.0;
	t2=0.0;
	run=false;
}

void clsTiming::Start(){
	if(!run){
#ifdef WIN32
		LARGE_INTEGER a;
		QueryPerformanceCounter(&a);
		t2=(double)a.QuadPart;
#else
		timeval time;
		gettimeofday(&time, NULL);
		t2=(double)time.tv_usec+(double)time.tv_sec*1000000.0;
#endif
		run=true;
	}
}

void clsTiming::Stop(){
	if(run){
#ifdef WIN32
		LARGE_INTEGER a;
		QueryPerformanceCounter(&a);
		t1+=(double)a.QuadPart-t2;
#else
		timeval time;
		gettimeofday(&time, NULL);
		t1+=(double)time.tv_usec+(double)time.tv_sec*1000000.0-t2;
#endif
		run=false;
	}
}

double clsTiming::GetMs(){
	if(run){
#ifdef WIN32
		LARGE_INTEGER a;
		QueryPerformanceCounter(&a);
		return (t1+(double)a.QuadPart-t2)/freq*1000.0;
#else
		timeval time;
		gettimeofday(&time, NULL);
		return (t1+(double)time.tv_usec+(double)time.tv_sec*1000000.0-t2)/freq*1000.0;
#endif
	}else{
		return t1/freq*1000.0;
	}
}
