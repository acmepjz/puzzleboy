#pragma once

//timing function

class clsTiming{
private:
	double t1,t2;
	double freq;
	bool run;
public:
	clsTiming();
	void Clear();
	void Start();
	void Stop();
	double GetMs();
};

