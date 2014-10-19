#pragma once

#include "UTF8-16.h"
#include <vector>
#include <queue>

struct NetworkMove{
	unsigned char type;
	int x;
	int y;
};

class SimpleProgressScreen;

class NetworkManager{
public:
	NetworkManager();
	~NetworkManager();

	static bool InitializeENet();
	static void DestroyENet();

	//create enet server (without GUI)
	bool CreateServer(int port=14285);
	//connect to enet server (with GUI)
	bool ConnectToServer(const u8string& hostName,int port=14285);
	//destroy enet server or disconnect from enet server (with GUI)
	void Disconnect();
	//forcefully reset connection (without GUI)
	void Reset();

	void OnTimer(bool discardAll=false);

	bool IsNetworkMultiplayer() const{return _host!=NULL;}
	bool IsConnected() const{return _peer!=NULL;}
	bool IsServer() const{return isServer;}
	void GetPeerDescription(std::vector<u8string>& desc);

	void SendQueryLevels(unsigned char flags);
	void SendSetCurrentLevel(unsigned char flags);
	void SendPlayerMove(const NetworkMove& move);
	void SendUpdateLevels();
	void SendGeneratingRandomMap();

	void NewSessionID();

	void AddReceivedMove(const NetworkMove& move){
		m_receivedMoves.push(move);
	}

	bool GetNextReceivedMove(NetworkMove& move){
		if(m_receivedMoves.empty()) return false;

		move=m_receivedMoves.front();
		m_receivedMoves.pop();

		return true;
	}

	void ClearReceivedMove(){
		while(!m_receivedMoves.empty()) m_receivedMoves.pop();
	}
private:
	void CreateProgressScreen();

	//show modal download level screen
	bool DownloadLevel(bool firstTime);
public:
	int m_nSessionID;
private:
	SimpleProgressScreen* progress;
	void *_host,*_peer;
	unsigned char m_address[20];
	unsigned int m_nNextRetryTime;
	bool isServer;
	std::queue<NetworkMove> m_receivedMoves;
private:
	static bool enetInitialized;
};

extern NetworkManager *netMgr;
