#pragma once

class SimpleProgressScreen{
public:
	SimpleProgressScreen();
	~SimpleProgressScreen();

	void Create();
	void Destroy();

	void ResetTimer();
	bool DrawAndDoEvents();
private:
	unsigned int m_Tex;
	unsigned int m_LastTime;
	int m_x;
public:
	float progress;
};
