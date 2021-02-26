#include "fun1.h"
#include <iostream>

void Fun::fun_v1(int n, int m)
{
	//std::cout << "new fun_v1" << std::endl;
	std::cout << "old fun_v1" << std::endl;
}

void Fun::fun(int n, int m)
{
	std::cout << "old fun" << std::endl;
	//std::cout << "new fun" << std::endl;
}

void Fun_son::fun_v1(int n, int m)
{
	std::cout << "old Fun_son::fun_v1" << std::endl;
}

void Fun_son::fun(int n, int m)
{
	std::cout << "old Fun_son::fun" << std::endl;
}

/*
void Fun::fun_v1(int n, int m)
{
	//std::cout << "new fun_v1" << std::endl;
	std::cout << "old fun_v1" << std::endl;
}

void Fun::fun(int n, int m)
{
	std::cout << "old fun" << std::endl;
	//std::cout << "new fun" << std::endl;
}
*/
