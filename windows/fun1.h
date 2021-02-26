#pragma once

class Fun
{
public:
	Fun() { n = 10; }
	_declspec(dllexport) virtual void fun(int n, int m);
	virtual void fun_v1(int n, int m);
	int getn() { return n; }
private:
	int n;
};


class Fun_son : public Fun
{
public:
	 virtual void fun(int n, int m);
	 virtual void fun_v1(int n, int m);
};

/*
class Fun
{
public:
	Fun() { n = 10; }
	void fun(int n, int m);
	void fun_v1(int n, int m);
private:
	int n;
};
*/