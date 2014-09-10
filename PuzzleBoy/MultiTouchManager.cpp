#include "MultiTouchManager.h"
#include "PuzzleBoyApp.h"
#include "main.h"

#include <math.h>

#include "include_sdl.h"

extern SDL_Event event;

void MultiTouchView::OnMultiGesture(float fx,float fy,float dx,float dy,float zoom){
}

void MultiTouchView::OnMouseEvent(int which,int state,int x,int y,int nFlags,int nType){
}

void MultiTouchView::OnMouseWheel(int which,int x,int y,int dx,int dy,int nFlags){
}

MultiTouchManager::MultiTouchManager()
:m_nDraggingIndex(-1)
{
}

MultiTouchManager::~MultiTouchManager(){
}

void MultiTouchManager::clear(){
	views.clear();
	ResetDraggingState();
}

void MultiTouchManager::ResetDraggingState(){
	fingers.clear();
	m_nDraggingIndex=-1;
}

void MultiTouchManager::AddView(float left,float top,float right,float bottom,int flags,MultiTouchView* view,int index){
	MultiTouchViewStruct viewStruct={
		left,top,right,bottom,flags,view,
		0.0f,0.0f,-1.0f
	};

	if(index>=0 && index<(int)views.size()){
		views.insert(views.begin()+index,viewStruct);

		if(m_nDraggingIndex>=index) m_nDraggingIndex++;

		for(std::map<long long,int>::iterator it=fingers.begin();it!=fingers.end();++it){
			if(it->second>=index) it->second++;
		}
	}else{
		views.push_back(viewStruct);
	}
}

MultiTouchViewStruct* MultiTouchManager::FindView(MultiTouchView* view){
	for(int i=0,m=views.size();i<m;i++){
		if(views[i].view==view) return &(views[i]);
	}

	return NULL;
}

void MultiTouchManager::RemoveView(MultiTouchView* view){
	for(int i=views.size()-1;i>=0;i--){
		if(views[i].view==view){
			views.erase(views.begin()+i);

			if(m_nDraggingIndex==i) m_nDraggingIndex=-1;
			else if(m_nDraggingIndex>i) m_nDraggingIndex--;

			for(std::map<long long,int>::iterator it=fingers.begin();it!=fingers.end();++it){
				if(it->second==i) fingers.erase(it); //??
				else if(it->second>i) it->second--;
			}
		}
	}
}

int MultiTouchManager::HitTest(float x,float y){
	for(int i=0,m=views.size();i<m;i++){
		if(views[i].flags>=0 && x>=views[i].left && y>=views[i].top
			&& x<views[i].right && y<views[i].bottom) return i;
	}

	return -1;
}

bool MultiTouchManager::OnEvent(){
	if(views.empty()) return false;

	switch(event.type){
	case SDL_MOUSEMOTION:
		theApp->SetTouchscreen(event.motion.which==SDL_TOUCH_MOUSEID);
		if(event.motion.which!=SDL_TOUCH_MOUSEID){
			float fx=float(event.motion.x)/float(screenHeight);
			float fy=float(event.motion.y)/float(screenHeight);

			//experimental dragging support
			if(event.motion.state
				&& m_nDraggingIndex>=0 && m_nDraggingIndex<(int)views.size()
				&& (views[m_nDraggingIndex].m_nDraggingState==1 || views[m_nDraggingIndex].m_nDraggingState==2)
				&& (views[m_nDraggingIndex].flags & MultiTouchViewFlags::AcceptDragging)!=0)
			{
				CheckDragging(m_nDraggingIndex,fx,fy);
			}

			if(m_nDraggingIndex>=0 && m_nDraggingIndex<(int)views.size()
				&& views[m_nDraggingIndex].m_nDraggingState>=2)
			{
				return true;
			}

			int idx=m_nDraggingIndex;
			if(idx<0){
				//we should run hit test first
				idx=HitTest(fx,fy);
			}

			if(idx>=0 && idx<(int)views.size()){
				if(views[idx].view) views[idx].view->OnMouseEvent(
					event.motion.which,
					event.motion.state,
					event.motion.x,
					event.motion.y,
					SDL_GetModState(),
					SDL_MOUSEMOTION);

				return true;
			}
		}
		break;
	case SDL_MOUSEBUTTONDOWN:
		theApp->SetTouchscreen(event.button.which==SDL_TOUCH_MOUSEID);
		if(event.motion.which==SDL_TOUCH_MOUSEID){
			//eat mouse down event
			if(HitTest(
				float(event.button.x)/float(screenHeight),
				float(event.button.y)/float(screenHeight))>=0)
			{
				return true;
			}
		}else{
			float fx=float(event.button.x)/float(screenHeight);
			float fy=float(event.button.y)/float(screenHeight);

			//experimental dragging support
			if(m_nDraggingIndex<0){
				m_nDraggingIndex=HitTest(fx,fy);
				if(m_nDraggingIndex>=0 && m_nDraggingIndex<(int)views.size()){
					views[m_nDraggingIndex].m_nDraggingState=1;
					views[m_nDraggingIndex].m_fMultitouchOldX=fx;
					views[m_nDraggingIndex].m_fMultitouchOldY=fy;
				}
			}

			if(m_nDraggingIndex>=0 && m_nDraggingIndex<(int)views.size()
				&& views[m_nDraggingIndex].m_nDraggingState==1
				&& views[m_nDraggingIndex].flags>=0)
			{
				if(views[m_nDraggingIndex].view) views[m_nDraggingIndex].view->OnMouseEvent(
					event.button.which,
					SDL_BUTTON(event.button.button),
					event.button.x,
					event.button.y,
					SDL_GetModState(),
					SDL_MOUSEBUTTONDOWN);

				return true;
			}
		}
		break;
	case SDL_MOUSEBUTTONUP:
		theApp->SetTouchscreen(event.button.which==SDL_TOUCH_MOUSEID);
		if(event.motion.which==SDL_TOUCH_MOUSEID){
			//eat mouse up event
			if(HitTest(
				float(event.button.x)/float(screenHeight),
				float(event.button.y)/float(screenHeight))>=0)
			{
				return true;
			}
		}else{
			bool ret=false;

			if(m_nDraggingIndex>=0 && m_nDraggingIndex<(int)views.size()
				&& views[m_nDraggingIndex].m_nDraggingState<2
				&& views[m_nDraggingIndex].flags>=0)
			{
				if(views[m_nDraggingIndex].view) views[m_nDraggingIndex].view->OnMouseEvent(
					event.button.which,
					SDL_BUTTON(event.button.button),
					event.button.x,
					event.button.y,
					SDL_GetModState(),
					SDL_MOUSEBUTTONUP);

				ret=true;
			}

			//experimental dragging support
			if(m_nDraggingIndex>=0 && m_nDraggingIndex<(int)views.size()
				&& SDL_GetMouseState(NULL,NULL)==0)
			{
				views[m_nDraggingIndex].m_nDraggingState=0;
				m_nDraggingIndex=-1;
			}

			return ret;
		}
		break;
	case SDL_MOUSEWHEEL:
		theApp->SetTouchscreen(event.wheel.which==SDL_TOUCH_MOUSEID);
		{
			int idx=m_nDraggingIndex;
			int x,y;
			SDL_GetMouseState(&x,&y);

			if(idx<0){
				//we should run hit test first
				idx=HitTest(float(x)/float(screenHeight),
					float(y)/float(screenHeight));
			}

			if(idx>=0 && idx<(int)views.size()
				&& views[idx].flags>=0)
			{
				int x,y;
				SDL_GetMouseState(&x,&y);

				if(views[idx].flags & MultiTouchViewFlags::AcceptZoom){
					if(event.wheel.y){
						if(views[idx].view) views[idx].view->OnMultiGesture(float(x)/float(screenHeight),float(y)/float(screenHeight),0.0f,0.0f,
							event.wheel.y>0?1.1f:1.0f/1.1f);
					}
				}else{
					if(views[idx].view) views[idx].view->OnMouseWheel(event.wheel.which,
						x,y,
						event.wheel.x,
						event.wheel.y,
						SDL_GetModState());
				}
			}
		}
		break;
	case SDL_FINGERDOWN:
		theApp->SetTouchscreen(true);
		{
			//run hit test for newly-added finger
			float fx=event.tfinger.x*screenAspectRatio;
			float fy=event.tfinger.y;

			int idx=HitTest(fx,fy);

			if(idx>=0 && idx<(int)views.size()){
				//the finger is in some view,
				//now we need to check existing fingers already in this view
				SDL_Finger *f0=NULL,*f1=NULL;

				int numFingers=SDL_GetNumTouchFingers(event.tfinger.touchId);
				for(int i=0;i<numFingers;i++){
					SDL_Finger *f=SDL_GetTouchFinger(event.tfinger.touchId,i);

					if(f==NULL || f->id==event.tfinger.fingerId) continue;

					std::map<long long,int>::iterator it=fingers.find(f->id);
					if(it!=fingers.end() && it->second==idx){
						if(f0==NULL){
							//we get the second finger (including newly-added one)
							f0=f;
						}else{
							//more than 2 fingers!
							if(f1==NULL) f1=f;
							break;
						}
					}
				}

				if(f1 || (f0 && (views[idx].flags & MultiTouchViewFlags::AcceptZoom)==0)){
					//more than 2 fingers, or the view doesn't support zooming
					views[idx].m_nDraggingState=3;
					views[idx].m_fMultitouchOldDistSquared=-1.0f;
				}else if(f0){
					//there are 2 fingers, start multitouch zooming
					views[idx].m_nDraggingState=3;
					views[idx].m_fMultitouchOldX=(f0->x+event.tfinger.x)*screenAspectRatio*0.5f;
					views[idx].m_fMultitouchOldY=(f0->y+event.tfinger.y)*0.5f;
					float dx=(f0->x-event.tfinger.x)*screenAspectRatio;
					float dy=f0->y-event.tfinger.y;
					views[idx].m_fMultitouchOldDistSquared=dx*dx+dy*dy;
				}else{
					//there is only 1 finger, send mouse down event and start dragging
					views[idx].m_nDraggingState=1;
					views[idx].m_fMultitouchOldX=fx;
					views[idx].m_fMultitouchOldY=fy;
					views[idx].m_fMultitouchOldDistSquared=-1.0f;

					if(views[idx].view) views[idx].view->OnMouseEvent(
						SDL_TOUCH_MOUSEID,
						SDL_BUTTON_LMASK,
						int(fx*float(screenHeight)+0.5f),
						int(fy*float(screenHeight)+0.5f),
						SDL_GetModState(),
						SDL_MOUSEBUTTONDOWN);
				}

				//now register the finger with this view
				fingers[event.tfinger.fingerId]=idx;

				return true;
			}
		}
		break;
	case SDL_FINGERMOTION:
		theApp->SetTouchscreen(true);
		{
			int m=views.size();
			int numFingers=SDL_GetNumTouchFingers(event.tfinger.touchId);

			for(int i=0;i<m;i++){
				views[i].f0=NULL;
				views[i].f1=NULL;
			}

			//assign fingers to views
			for(int i=0;i<numFingers;i++){
				SDL_Finger *f=SDL_GetTouchFinger(event.tfinger.touchId,i);

				if(f==NULL) continue;

				std::map<long long,int>::iterator it=fingers.find(f->id);
				if(it!=fingers.end()){
					int idx=it->second;
					if(idx>=0 && idx<m
						&& views[idx].flags>=0)
					{
						if(views[idx].f0==NULL){
							//we get the first finger
							views[idx].f0=f;
						}else if(views[idx].f1 || (views[idx].flags & MultiTouchViewFlags::AcceptZoom)==0){
							//more than 2 fingers, or the view doesn't support zooming
							views[idx].m_nDraggingState=3;
							views[idx].m_fMultitouchOldDistSquared=-1.0f;
						}else{
							//we get the second finger
							views[idx].f1=f;
						}
					}else{
						fingers.erase(it); //???
					}
				}
			}

			//generate event for each view
			for(int i=0;i<m;i++){
				//process event only if the view has at least 1 finger
				if(views[i].flags>=0 && views[i].f0){
					if(views[i].f1 && views[i].m_nDraggingState==3
						&& views[i].m_fMultitouchOldDistSquared>0.0f)
					{
						//process multitouch zooming
						float x=(views[i].f0->x+views[i].f1->x)*screenAspectRatio*0.5f;
						float y=(views[i].f0->y+views[i].f1->y)*0.5f;

						float dx=(views[i].f0->x-views[i].f1->x)*screenAspectRatio;
						float dy=views[i].f0->y-views[i].f1->y;
						dx=dx*dx+dy*dy;
						if(dx<1E-30f) dx=1E-30f;

						float zoom=sqrt(dx/views[i].m_fMultitouchOldDistSquared);
						if(zoom<0.1f) zoom=0.1f;
						else if(zoom>10.0f) zoom=10.0f;
						views[i].m_fMultitouchOldDistSquared=dx;

						dx=x-views[i].m_fMultitouchOldX;
						dy=y-views[i].m_fMultitouchOldY;
						views[i].m_fMultitouchOldX=x;
						views[i].m_fMultitouchOldY=y;

						if(views[i].view) views[i].view->OnMultiGesture(x,y,dx,dy,zoom);
					}else if(views[i].f1==NULL
						&& (views[i].m_nDraggingState==1 || views[i].m_nDraggingState==2))
					{
						//check dragging
						if(views[i].flags & MultiTouchViewFlags::AcceptDragging){
							CheckDragging(i,views[i].f0->x*screenAspectRatio,views[i].f0->y);
						}else{
							if(views[i].view) views[i].view->OnMouseEvent(
								SDL_TOUCH_MOUSEID,
								SDL_BUTTON_LMASK,
								int(views[i].f0->x*float(screenWidth)+0.5f),
								int(views[i].f0->y*float(screenHeight)+0.5f),
								SDL_GetModState(),
								SDL_MOUSEMOTION);
						}
					}else{
						//something goes wrong
						views[i].m_nDraggingState=3;
						views[i].m_fMultitouchOldDistSquared=-1.0f;
					}
				}else{
					// XXX don't reset state here because SDL will
					// remove all fingers before SDL_FINGERUP event
					/*//reset dragging state if no fingers
					views[i].m_nDraggingState=0;
					views[i].m_fMultitouchOldDistSquared=-1.0f;*/
				}
			}
		}
		break;
	case SDL_FINGERUP:
		theApp->SetTouchscreen(true);
		{
			//check if the finger is registered
			std::map<long long,int>::iterator it=fingers.find(event.tfinger.fingerId);
			if(it!=fingers.end()){
				int idx=it->second;

				//remove this finger
				fingers.erase(it);

				if(idx>=0 && idx<(int)views.size()){
					if(views[idx].m_nDraggingState<2
						&& views[idx].flags>=0)
					{
						//send mouse up event
						if(views[idx].view) views[idx].view->OnMouseEvent(
							SDL_TOUCH_MOUSEID,
							SDL_BUTTON_LMASK,
							int(event.tfinger.x*float(screenWidth)+0.5f),
							int(event.tfinger.y*float(screenHeight)+0.5f),
							SDL_GetModState(),
							SDL_MOUSEBUTTONUP);
					}

					//reset dragging state if there are no fingers in this view
					bool b=true;
					for(it=fingers.begin();it!=fingers.end();++it){
						if(it->second==idx){
							b=false;
							break;
						}
					}
					if(b){
						views[idx].m_nDraggingState=0;
						views[idx].m_fMultitouchOldDistSquared=-1.0f;
					}
				}

				return true;
			}
		}
		break;
	case SDL_KEYDOWN:
		//TODO: page down, etc.
		break;
	}

	return false;
}

void MultiTouchManager::CheckDragging(int index,float x,float y){
	float dx=x-views[index].m_fMultitouchOldX;
	float dy=y-views[index].m_fMultitouchOldY;
	if(views[index].m_nDraggingState==2 || dx*dx+dy*dy>0.003f){
		views[index].m_nDraggingState=2;
		views[index].m_fMultitouchOldX=x;
		views[index].m_fMultitouchOldY=y;
		if(views[index].view) views[index].view->OnMultiGesture(x,y,dx,dy,1.0f);
	}
}
