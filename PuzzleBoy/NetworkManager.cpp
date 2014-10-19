#include "NetworkManager.h"
#include "SimpleProgressScreen.h"
#include "PuzzleBoyApp.h"
#include "MyFormat.h"
#include "MySerializer.h"
#include "PuzzleBoyLevelFile.h"
#include "PuzzleBoyLevelView.h"
#include "PuzzleBoyLevel.h"
#include "RecordManager.h"

#include <SDL.h>
#include <enet/enet.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vector>

NetworkManager *netMgr=NULL;

bool NetworkManager::enetInitialized=false;

#define m_host ((ENetHost*&)_host)
#define m_peer ((ENetPeer*&)_peer)

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define ENET_HOST_LOCALHOST 0x0100007F
#else
#define ENET_HOST_LOCALHOST 0x7F000001
#endif

const int CHANNELS_USED=2;

//packet type: 1 byte
enum PuzzleBoyNetworkPacketType{
	//send when peer want to make sure the levels are up to date
	//format: flags (uint8) 0x1=player name present 0x2=checksum present
	//0x4=sender's current level present (used when client auto-reconnect)
	//player name: (u16string)
	//checksum: count (vuint32), checksums...
	//if count=0 then doesn't perform level update
	//sender's current level: index (vuint32), data (PuzzleBoyLevel::SerializeHistory)
	//receiver should update history if this level's checksum is match
	PACKET_QUERY_LEVELS,

	//send when peer want to update level pack or level
	//format: max progress (vuint32), current progress (vuint32),
	//flags (uint8) 0x1=level pack name present 0x2=level count present 0x4=level data present
	//level pack name: (u16string)
	//level count: (vuint32)
	//level data: level index (vuint32), level name (u16string), level data (MySerialize)
	PACKET_UPDATE_LEVELS,

	//send when peer want to change current level or want to receive current level
	//format: flags (uint8) 0x1=sender's current level present
	//0x2=receiver's current level present
	//0x4=session id present
	//0x10=sender want to get his current level
	//0x20=sender want to get receiver's current level
	//sender's current level: index (vuint32), data (PuzzleBoyLevel::SerializeHistory)
	//receiver's currrent level: same
	//session id: (int32)
	PACKET_SET_CURRENT_LEVEL,

	//send when sender moves
	//format: sesson id (int32)
	//type (uint8) 0-7=up,down,left,right,undo,redo,switch,restart
	//8=set play from record flag
	//if type==switch (6) then there are also x (vuint32), y (vuint32)
	PACKET_PLAYER_MOVE,

	//send when generating random maps
	//currently it's only a notification
	PACKET_GENERATING_RANDOM_MAP,
};

NetworkManager::NetworkManager()
:m_nSessionID(0)
,progress(NULL)
,_host(NULL)
,_peer(NULL)
,m_nNextRetryTime(0)
,isServer(false)
{
	assert(sizeof(m_address)>=sizeof(ENetAddress));
}

static void SendPacket(ENetPeer *peer,const std::vector<unsigned char>& data){
	ENetPacket *packet=enet_packet_create(&(data[0]),data.size(),ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer,0,packet);
}

static void SendQueryLevelsPacket(ENetPeer *peer,unsigned char flags){
	MySerializer ar;
	ar.PutInt8(PACKET_QUERY_LEVELS);
	ar.PutInt8(flags);

	//player name
	if(flags & 0x1) ar.PutU16String(theApp->m_sPlayerName[0]);

	//check sum
	if(flags & 0x2){
		PuzzleBoyLevelFile *pDoc=theApp->m_pDocument;
		const size_t m=pDoc->m_objLevels.size();

		ar.PutVUInt32(m);

		RecordLevelChecksum checksum;

		for(size_t i=0;i<m;i++){
			MySerializer ar2;
			pDoc->m_objLevels[i]->MySerialize(ar2);
			ar2.CalculateBlake2s(&checksum,PuzzleBoyLevelData::ChecksumSize);
			ar.GetData().insert(ar.GetData().end(),
				checksum.bChecksum,
				checksum.bChecksum+PuzzleBoyLevelData::ChecksumSize);
		}
	}

	//current level
	if(flags & 0x4){
		ar.PutVUInt32(theApp->m_view[0]->m_nCurrentLevel);
		theApp->m_view[0]->m_objPlayingLevel->SerializeHistory(ar);
	}

	SendPacket(peer,ar.GetData());
}

static void SendSetCurrentLevelPacket(ENetPeer *peer,unsigned char flags,int sessionID){
	MySerializer ar;
	ar.PutInt8(PACKET_SET_CURRENT_LEVEL);
	ar.PutInt8(flags);

	//send our level
	if(flags & 0x1){
		ar.PutVUInt32(theApp->m_view[0]->m_nCurrentLevel);
		theApp->m_view[0]->m_objPlayingLevel->SerializeHistory(ar);
	}

	//send their level
	if(flags & 0x2){
		ar.PutVUInt32(theApp->m_view[1]->m_nCurrentLevel);
		theApp->m_view[1]->m_objPlayingLevel->SerializeHistory(ar);
	}

	//send session ID
	if(flags & 0x4){
		ar.PutInt32(sessionID);
	}

	SendPacket(peer,ar.GetData());
}

struct LevelNeedToUpdate{
	int index;
	u16string name;
	MySerializer data;
};

static void SendUpdateLevelsPacket(ENetPeer *peer,int sessionID,const std::vector<RecordLevelChecksum> *checksums){
	PuzzleBoyLevelFile *pDoc=theApp->m_pDocument;
	const size_t m=pDoc->m_objLevels.size();
	const size_t m2=checksums?(checksums->size()):0;

	//check which levels need to update
	std::vector<LevelNeedToUpdate*> levelsNeedToUpdate;
	RecordLevelChecksum checksum;
	for(size_t i=0;i<m;i++){
		LevelNeedToUpdate *lv=NULL;

		lv=new LevelNeedToUpdate;
		pDoc->m_objLevels[i]->MySerialize(lv->data);

		if(checksums && i<m2){
			lv->data.CalculateBlake2s(&checksum,PuzzleBoyLevelData::ChecksumSize);
			if(checksum==(*checksums)[i]){
				delete lv;
				continue;
			}
		}

		lv->index=i;
		lv->name=pDoc->m_objLevels[i]->m_sLevelName;
		levelsNeedToUpdate.push_back(lv);
	}

	//send packets
	MySerializer ar;
	ar.PutInt8(PACKET_QUERY_LEVELS);
	ar.PutInt8(0x1); //flags
	ar.PutU16String(theApp->m_sPlayerName[0]);
	SendPacket(peer,ar.GetData());

	ar.Clear();
	ar.PutInt8(PACKET_UPDATE_LEVELS);
	ar.PutVUInt32(levelsNeedToUpdate.size()); //max progress
	ar.PutVUInt32(0); //current progress
	ar.PutInt8(0x1 | (m!=m2?0x2:0)); //flags
	ar.PutU16String(pDoc->m_sLevelPackName);
	if(m!=m2) ar.PutVUInt32(m);
	SendPacket(peer,ar.GetData());

	printf("[SendUpdateLevelsPacket] There are %d levels need to transfer\n",levelsNeedToUpdate.size());

	for(size_t i=0;i<levelsNeedToUpdate.size();i++){
		LevelNeedToUpdate *lv=levelsNeedToUpdate[i];

		ar.Clear();
		ar.PutInt8(PACKET_UPDATE_LEVELS);
		ar.PutVUInt32(levelsNeedToUpdate.size()); //max progress
		ar.PutVUInt32(i+1); //current progress
		ar.PutInt8(0x4); //flags
		ar.PutVUInt32(lv->index);
		ar.PutU16String(lv->name);
		ar.GetData().insert(ar.GetData().end(),
			lv->data.GetData().begin(),
			lv->data.GetData().end());
		SendPacket(peer,ar.GetData());

		//now it's OK to delete it
		delete lv;
	}

	//change current level and update session ID
	SendSetCurrentLevelPacket(peer,0x7,sessionID);
}

static void SendPlayerMovePacket(ENetPeer *peer,int sessionID,const NetworkMove& move){
	MySerializer ar;

	ar.PutInt8(PACKET_PLAYER_MOVE);
	ar.PutInt32(sessionID);
	ar.PutInt8(move.type);

	if(move.type==6){
		ar.PutVSInt32(move.x);
		ar.PutVSInt32(move.y);
	}

	//DEBGU
	//printf("Send %d %d %d\n",move.type,move.x,move.y);

	SendPacket(peer,ar.GetData());
}

static void SendGeneratingRandomMapPacket(ENetPeer *peer){
	MySerializer ar;

	ar.PutInt8(PACKET_GENERATING_RANDOM_MAP);

	SendPacket(peer,ar.GetData());
}

static void ProcessQueryLevelsPacket(MySerializer& ar0,ENetPeer *peer,NetworkManager* mgr){
	unsigned char flags=ar0.GetInt8();

	//player name
	if(flags & 0x1){
		theApp->m_sPlayerName[2]=ar0.GetU16String();
		if(theApp->m_view.size()>=2 && theApp->m_view[1]){
			theApp->m_view[1]->m_sPlayerName=theApp->m_sPlayerName[2];
		}
	}

	//checksum
	std::vector<RecordLevelChecksum> checksums;
	if(flags & 0x2){
		size_t m=ar0.GetVUInt32();
		if(m){
			checksums.resize(m);
			int count=m*PuzzleBoyLevelData::ChecksumSize;
			memcpy(&(checksums[0]),&(ar0.GetData()[ar0.GetOffset()]),count);
			ar0.SkipOffset(count);
		}
	}

	//receive their current level
	if(flags & 0x4){
		int index=ar0.GetVUInt32();

		//DEBGU
		//printf("%d %d %d\n",index,theApp->m_pDocument->m_objLevels.size(),checksums.size());

		if(index>=0 && index<(int)theApp->m_pDocument->m_objLevels.size() && index<(int)checksums.size()){
			//calculate checksum and check if it matches
			RecordLevelChecksum checksum;

			MySerializer ar2;
			theApp->m_pDocument->m_objLevels[index]->MySerialize(ar2);
			ar2.CalculateBlake2s(&checksum,PuzzleBoyLevelData::ChecksumSize);

			if(checksum==checksums[index]){
				//OK, we can update history
				theApp->m_view[1]->m_sPlayerName=theApp->m_sPlayerName[2];
				theApp->m_view[1]->m_nCurrentLevel=index;
				theApp->m_view[1]->StartGame();
				theApp->m_view[1]->m_objPlayingLevel->SerializeHistory(ar0);

				mgr->ClearReceivedMove();
			}
		}
	}

	//reply packet
	if(!checksums.empty()) SendUpdateLevelsPacket(peer,mgr->m_nSessionID,&checksums);
}

static void ProcessUpdateLevelsPacket(MySerializer& ar0,int *maxProgress=NULL,int *currentProgress=NULL){
	int i;

	i=ar0.GetVUInt32();
	if(maxProgress) *maxProgress=i;
	i=ar0.GetVUInt32();
	if(currentProgress) *currentProgress=i;

	PuzzleBoyLevelFile *pDoc=theApp->m_pDocument;

	const unsigned char flags=ar0.GetInt8();

	//change level pack name
	if(flags & 0x1){
		pDoc->m_sLevelPackName=ar0.GetU16String();
	}

	//change level count
	if(flags & 0x2){
		size_t m=ar0.GetVUInt32();
		if(m!=pDoc->m_objLevels.size()){
			if(m<pDoc->m_objLevels.size()){
				for(i=m;i<(int)pDoc->m_objLevels.size();i++){
					delete pDoc->m_objLevels[i];
				}
				pDoc->m_objLevels.resize(m);
			}else{
				for(i=pDoc->m_objLevels.size();i<(int)m;i++){
					PuzzleBoyLevelData *lv=new PuzzleBoyLevelData;
					lv->CreateDefault();
					pDoc->m_objLevels.push_back(lv);
				}
			}

			theApp->m_sLastFile.clear();
		}
	}

	//change level
	if(flags & 0x4){
		i=ar0.GetVUInt32();
		if(i<0 || i>=(int)pDoc->m_objLevels.size()){
			printf("[ProcessUpdateLevelsPacket] Warning: Level index %d out of range!\n",i);
			return;
		}

		pDoc->m_objLevels[i]->m_sLevelName=ar0.GetU16String();
		pDoc->m_objLevels[i]->MySerialize(ar0);

		theApp->m_sLastFile.clear();
	}
}

static void ProcessSetCurrentLevelPacket(MySerializer& ar0,ENetPeer *peer,NetworkManager* mgr){
	unsigned char flags=ar0.GetInt8();

	if(flags & 0x1){
		int index=ar0.GetVUInt32();
		if(index<0 || index>=(int)theApp->m_pDocument->m_objLevels.size()){
			printf("[ProcessSetCurrentLevelPacket] Warning: Level index %d out of range!\n",index);
			return;
		}

		theApp->m_view[1]->m_sPlayerName=theApp->m_sPlayerName[2];
		theApp->m_view[1]->m_nCurrentLevel=index;
		theApp->m_view[1]->StartGame();
		theApp->m_view[1]->m_objPlayingLevel->SerializeHistory(ar0);

		mgr->ClearReceivedMove();
	}

	if(flags & 0x2){
		int index=ar0.GetVUInt32();
		if(index<0 || index>=(int)theApp->m_pDocument->m_objLevels.size()){
			printf("[ProcessSetCurrentLevelPacket] Warning: Level index %d out of range!\n",index);
			return;
		}

		theApp->m_nCurrentLevel=index;
		theApp->m_view[0]->m_sPlayerName=theApp->m_sPlayerName[0];
		theApp->m_view[0]->m_nCurrentLevel=index;
		theApp->m_view[0]->StartGame();
		theApp->m_view[0]->m_objPlayingLevel->SerializeHistory(ar0);
	}

	if(flags & 0x4){
		mgr->m_nSessionID=ar0.GetInt32();
	}

	unsigned char flags2=0;

	if(flags & 0x20) flags2|=0x1;
	if(flags & 0x10) flags2|=0x2;

	if(flags2) SendSetCurrentLevelPacket(peer,flags2,0);
}

static void ProcessPlayerMovePacket(MySerializer& ar0,NetworkManager* mgr){
	if(ar0.GetInt32()!=mgr->m_nSessionID) return;

	NetworkMove move={};

	move.type=ar0.GetInt8();

	if(move.type==6){
		move.x=ar0.GetVSInt32();
		move.y=ar0.GetVSInt32();
	}

	//DEBGU
	//printf("Recv %d %d %d\n",move.type,move.x,move.y);

	mgr->AddReceivedMove(move);
}

static void ProcessGeneratingRandomMapPacket(MySerializer& ar0){
	theApp->ShowToolTip(_("Remote host is generating random map..."));
}

bool NetworkManager::InitializeENet(){
	if(enetInitialized) return true;
	if(enet_initialize()){
		printf("[NetworkManager] Error: Can't initialize ENet!\n");
		return false;
	}
	enetInitialized=true;
	return true;
}

void NetworkManager::DestroyENet(){
	if(enetInitialized){
		enet_deinitialize();
		enetInitialized=false;
	}
}

bool NetworkManager::CreateServer(int port){
	if(!InitializeENet()) return false;

	Disconnect();
	m_nNextRetryTime=0;
	isServer=true;

	ENetAddress address={ENET_HOST_ANY /*ENET_HOST_LOCALHOST*/,port};

	m_host=enet_host_create(&address,2,CHANNELS_USED,0,0);
	if(m_host==NULL){
		printf("[NetworkManager] Error: Can't create ENet server!\n");
		return false;
	}

	enet_host_compress_with_range_coder(m_host);

	return true;
}

bool NetworkManager::ConnectToServer(const u8string& hostName,int port){
	if(!InitializeENet()) return false;

	Disconnect();
	m_nNextRetryTime=0;
	isServer=false;

	ENetAddress address;
	if(enet_address_set_host(&address,hostName.c_str())){
		printf("[NetworkManager] Error: Can't resolve host %s!\n",hostName.c_str());
		return false;
	}
	address.port=port;

	memcpy(m_address,&address,sizeof(address));

	m_host=enet_host_create(NULL,1,CHANNELS_USED,0,0);
	if(m_host==NULL){
		printf("[NetworkManager] Error: Can't create ENet client!\n");
		return false;
	}

	enet_host_compress_with_range_coder(m_host);

	m_peer=enet_host_connect(m_host,&address,CHANNELS_USED,0);
	if(m_peer==NULL){
		printf("[NetworkManager] Error: Can't create ENet peer!\n");
		enet_host_destroy(m_host);
		m_host=NULL;
		return false;
	}

	//now wait for connection established
	CreateProgressScreen();
	theApp->ShowToolTip(str(MyFormat(_("Connecting to %s..."))<<hostName));
	ENetEvent event;
	bool ret=false;

	//wait for 15 seconds
	for(int i=0;i<300 && m_peer && !ret;i++){
		SDL_Delay(50);

		while(m_peer && !ret && enet_host_service(m_host,&event,0)>0){
			switch(event.type){
			case ENET_EVENT_TYPE_RECEIVE:
				enet_packet_destroy(event.packet);
				break;
			case ENET_EVENT_TYPE_CONNECT:
				printf("[NetworkManager] Successfully connected to %s\n",hostName.c_str());
				ret=true;
				break;
			case ENET_EVENT_TYPE_DISCONNECT:
				printf("[NetworkManager] Can't connect to %s\n",hostName.c_str());
				m_peer=NULL;
				break;
			}
		}

		if(ret || !progress->DrawAndDoEvents()) break;
	}

	if(ret){
		//download maps, etc.
		SendQueryLevelsPacket(m_peer,0x3);
		enet_host_flush(m_host);

		ret=DownloadLevel(true);

		if(ret) return true;
	}

	enet_host_destroy(m_host);
	m_host=NULL;
	m_peer=NULL;

	return false;
}

bool NetworkManager::DownloadLevel(bool firstTime){
	ENetEvent event;
	bool ret=false;

	theApp->ShowToolTip(_("Downloading maps..."));

	CreateProgressScreen();

	while(m_peer && !ret){
		SDL_Delay(50);

		while(m_peer && !ret && enet_host_service(m_host,&event,0)>0){
			switch(event.type){
			case ENET_EVENT_TYPE_RECEIVE:
				{
					MySerializer ar0;
					ar0.SetData(event.packet->data,event.packet->dataLength);

					const unsigned char type=ar0.GetInt8();
					int maxProgress,currentProgress;

					switch(type){
					case PACKET_QUERY_LEVELS:
						ProcessQueryLevelsPacket(ar0,m_peer,this);
						break;
					case PACKET_UPDATE_LEVELS:
						ProcessUpdateLevelsPacket(ar0,&maxProgress,&currentProgress);
						if(maxProgress>0) progress->progress=float(currentProgress)/(float)maxProgress;
						break;
					case PACKET_SET_CURRENT_LEVEL:
						if(firstTime) theApp->StartGame(2);
						ProcessSetCurrentLevelPacket(ar0,m_peer,this);
						ret=true;
						break;
					default:
						printf("[NetworkManager] Warning: Ignore packet type %d during handshake\n",type);
						break;
					}
				}
				enet_packet_destroy(event.packet);
				break;
			case ENET_EVENT_TYPE_DISCONNECT:
				printf("[NetworkManager] Peer %p disconnected during handshake\n",m_peer);
				if(!firstTime) theApp->ShowToolTip(_("Disconnected"));
				m_peer=NULL;
				break;
			}
		}

		if(ret) break;

		if(!progress->DrawAndDoEvents()){
			if(firstTime) enet_peer_disconnect_now(m_peer,0);
			break;
		}
	}

	return ret;
}

void NetworkManager::CreateProgressScreen(){
	if(progress){
		progress->progress=0.0f;
	}else{
		progress=new SimpleProgressScreen();
		progress->Create();
	}
}

void NetworkManager::Disconnect(){
	if(m_host==NULL) return;

	//check if there is peer connected
	if(m_peer){
		CreateProgressScreen();
		theApp->ShowToolTip(_("Disconnecting..."));

		ENetEvent event;

		//try to disconnect peer
		enet_peer_disconnect(m_peer,0);

		//wait for 5 seconds
		for(int i=0;i<100 && m_peer;i++){
			SDL_Delay(50);

			while(enet_host_service(m_host,&event,0)>0){
				switch(event.type){
				case ENET_EVENT_TYPE_RECEIVE:
					enet_packet_destroy(event.packet);
					break;
				case ENET_EVENT_TYPE_DISCONNECT:
					m_peer=NULL;
					break;
				}
			}

			if(!progress->DrawAndDoEvents()) break;
		}

		//forcefully disconnect
		if(m_peer){
			printf("[NetworkManager] Warning: Forcefully disconnect peer %p\n",m_peer);
			enet_peer_disconnect_now(m_peer,0);
		}
	}

	enet_host_destroy(m_host);
	m_host=NULL;
	m_peer=NULL;
}

void NetworkManager::Reset(){
	if(m_host){
		if(m_peer){
			printf("[NetworkManager] Warning: Forcefully disconnect peer %p\n",m_peer);
			enet_peer_disconnect_now(m_peer,0);
		}
		enet_host_destroy(m_host);
		m_host=NULL;
		m_peer=NULL;
	}
}

NetworkManager::~NetworkManager(){
	Reset();
	delete progress;
}

void NetworkManager::OnTimer(bool discardAll){
	if(m_host==NULL) return;

	//try to auto-reconnect
	if(m_peer==NULL && m_nNextRetryTime && !isServer && SDL_GetTicks()>m_nNextRetryTime){
		m_nNextRetryTime+=15000; //retry after 15 sec ???

		ENetAddress *a=(ENetAddress*)m_address;
		ENetPeer *p=enet_host_connect(m_host,a,CHANNELS_USED,0);
		if(p==NULL){
			//FIXME: maybe maximal connection count reached
			printf("[NetworkManager] Error: Can't create ENet peer!\n");
		}

		char s[64];
		enet_address_get_host_ip(a,s,sizeof(s));
		theApp->ShowToolTip(str(MyFormat(_("Connecting to %s..."))<<s));
	}

	ENetEvent event;

	while(enet_host_service(m_host,&event,0)>0){
		switch(event.type){
		case ENET_EVENT_TYPE_RECEIVE:
			if(!discardAll){
				MySerializer ar0;
				ar0.SetData(event.packet->data,event.packet->dataLength);

				const unsigned char type=ar0.GetInt8();

				switch(type){
				case PACKET_QUERY_LEVELS:
					ProcessQueryLevelsPacket(ar0,m_peer,this);
					break;
				case PACKET_UPDATE_LEVELS:
					ProcessUpdateLevelsPacket(ar0);
					DownloadLevel(false);
					break;
				case PACKET_SET_CURRENT_LEVEL:
					ProcessSetCurrentLevelPacket(ar0,m_peer,this);
					break;
				case PACKET_PLAYER_MOVE:
					ProcessPlayerMovePacket(ar0,this);
					break;
				case PACKET_GENERATING_RANDOM_MAP:
					ProcessGeneratingRandomMapPacket(ar0);
					break;
				default:
					printf("[NetworkManager] Warning: Unknown packet type %d\n",type);
					break;
				}
			}
			enet_packet_destroy(event.packet);
			break;
		case ENET_EVENT_TYPE_CONNECT:
			printf("[NetworkManager] Peer %p connected\n",event.peer);
			if(event.peer!=m_peer){
				char s[64];
				enet_address_get_host_ip(&(event.peer->address),s,sizeof(s));
				theApp->ShowToolTip(str(MyFormat(_("Connected to %s"))<<s));

				if(m_peer) enet_peer_disconnect(m_peer,0);
				m_peer=event.peer;

				//send download maps packet (i.e. auto-reconnect feature)
				if(!isServer){
					SendQueryLevelsPacket(m_peer,0x7);
				}
			}
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			printf("[NetworkManager] Peer %p disconnected\n",event.peer);
			if(event.peer==m_peer){
				theApp->ShowToolTip(_("Disconnected"));
				m_peer=NULL;

				//retry after 3 sec ???
				m_nNextRetryTime=SDL_GetTicks()+3000;
			}
			break;
		}
	}
}

void NetworkManager::GetPeerDescription(std::vector<u8string>& desc){
	char c[64];

	{
		u8string s=_("Current Status")+": ";
		if(m_peer){
			enet_address_get_host_ip(&(m_peer->address),c,sizeof(c));
			s+=str(MyFormat(_("Connected to %s"))<<c);
		}else{
			s+=_("Disconnected");
		}
		desc.push_back(s);
	}

	if(m_peer){
		desc.push_back(str(MyFormat(_("Ping: %'d ms"))<<m_peer->roundTripTime));
		desc.push_back(str(MyFormat(_("Downloaded Data: %'d Bytes"))<<m_host->totalReceivedData));
		desc.push_back(str(MyFormat(_("Uploaded Data: %'d Bytes"))<<m_host->totalSentData));
		desc.push_back(str(MyFormat(_("Packet Loss: %0.2f%%"))<<(float(m_peer->packetLoss)/float(ENET_PEER_PACKET_LOSS_SCALE))));
	}
}

void NetworkManager::SendSetCurrentLevel(unsigned char flags){
	if(m_peer) SendSetCurrentLevelPacket(m_peer,flags,m_nSessionID);
}

void NetworkManager::SendPlayerMove(const NetworkMove& move){
	if(m_peer) SendPlayerMovePacket(m_peer,m_nSessionID,move);
}

void NetworkManager::SendUpdateLevels(){
	if(m_peer) SendUpdateLevelsPacket(m_peer,m_nSessionID,NULL);
}

void NetworkManager::SendGeneratingRandomMap(){
	if(m_peer) SendGeneratingRandomMapPacket(m_peer);
}

void NetworkManager::NewSessionID(){
	m_nSessionID=theApp->m_objMainRnd.Rnd();
}

void NetworkManager::SendQueryLevels(unsigned char flags){
	if(m_peer) SendQueryLevelsPacket(m_peer,flags);
}
