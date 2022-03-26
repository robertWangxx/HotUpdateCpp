/*
	大概原理和顺序
	重点是原函数地址和目的函数地址获取的方法
	dumpbin的作用（反汇编exe 得到函数入口的地址(实际运行时地址会变但相对地址不变) 函数入口的特征码(运行时也保持不变 属于代码段内容)）
	release模式下某些函数会被优化掉 使用dumpbin反汇编无法得到函数入口  设置函数导出时则无法优化掉函数入口地址 可用该函数获取基址
	x86和x64下的函数跳转汇编指令实现区别
	通过扫描特征码获取函数地址适用x86 扫描范围较小
	增量链接启动：函数入口在@ILT处 通过增量链接表来跳转
	增量链接关闭：函数入口直接在定义处
*/
#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <windows.h>
#include "fun1.h"

//#pragma comment(linker, "/entry:testfun")

#define JMPCODE_LENGTH 5            //x86 平坦内存模式下，绝对跳转指令长度  
#define JMPCMD_LENGTH  1            //机械码0xe9长度  
#define JMPCMD         0xe9         //对应汇编的jmp指令 


//写jmp的x64版本函数
bool WriteJMP_x64(LPCVOID dwFrom, LPCVOID dwTo)
{
	if (dwFrom == dwTo)
		return false;
	if (!dwFrom || !dwTo)
		return false;

	DWORD_PTR dwAdr = (DWORD_PTR)dwFrom;
	DWORD_PTR dwAdrTo = (DWORD_PTR)dwTo;
	DWORD   ProtectVar;              // 保护属性变量
	MEMORY_BASIC_INFORMATION MemInfo;    //内存分页属性信息

	// 取得对应内存的原始属性
	if (0 != VirtualQuery(dwFrom, &MemInfo, sizeof(MEMORY_BASIC_INFORMATION)))
	{
		if (VirtualProtect(MemInfo.BaseAddress, MemInfo.RegionSize, PAGE_EXECUTE_READWRITE, &MemInfo.Protect))
		{

			*(BYTE*)dwAdr = 0x68;
			dwAdr += 1;
			*(DWORD32*)dwAdr = DWORD32(dwAdrTo & 0xffffffff);
			dwAdr += 4;
			*(DWORD32*)dwAdr = DWORD32(0x042444c7);
			dwAdr += 4;
			*(DWORD32*)dwAdr = DWORD32(dwAdrTo >> 32);
			dwAdr += 4;
			*(BYTE*)dwAdr = 0xc3;

			// 改回原属性
			VirtualProtect(MemInfo.BaseAddress, MemInfo.RegionSize, MemInfo.Protect, &ProtectVar);

			// 修改后，还需要刷新cache
			//FlushInstructionCache(GetCurrentProcess(), dwFrom, JMPCODE_LENGTH);

			return true;
		}
	}

	return false;
	/*
	push 地址的低32位
	mov dword ptr ss:[rsp+4],地址的高32位
	ret
	*/

	//14 bytes
}

//jmp xxxx(该指令一共占用 5 bytes)  jmp指令占 1 byte   相对地址占 4 bytes  
//写jmp的x86版本函数  因为x86下(pNewFunc - pOrigFunc)的值不超过4字节   x64下(pNewFunc - pOrigFunc)值可能超过4字节大小
bool WriteJMP_x86(LPCVOID pOrigFunc, LPCVOID pNewFunc)
{
	if (pOrigFunc == pNewFunc)
		return false;

	if (pOrigFunc && pNewFunc)
	{
		DWORD   ProtectVar;              // 保护属性变量
		MEMORY_BASIC_INFORMATION MemInfo;    //内存分页属性信息

		// 取得对应内存的原始属性
		if (0 != VirtualQuery(pOrigFunc, &MemInfo, sizeof(MEMORY_BASIC_INFORMATION)))
		{
			if (VirtualProtect(MemInfo.BaseAddress, MemInfo.RegionSize, PAGE_EXECUTE_READWRITE, &MemInfo.Protect))
			{
				// 备份原数据，防止自身需要使用memcpy，不能使用类似接口
				//__inner_memcpy((unsigned char*)str_instruct_back, (unsigned char*)pOrigFunc, JMPCODE_LENGTH);

				// 修改目标地址指令为 jmp pNewFunc
				*(unsigned char*)pOrigFunc = JMPCMD;                                      //拦截API，在函数代码段前面注入jmp xxxx  
				*(DWORD*)((unsigned char*)pOrigFunc + JMPCMD_LENGTH) = (DWORD)pNewFunc - (DWORD)pOrigFunc - JMPCODE_LENGTH;

				// 改回原属性
				VirtualProtect(MemInfo.BaseAddress, MemInfo.RegionSize, MemInfo.Protect, &ProtectVar);

				// 修改后，还需要刷新cache
				FlushInstructionCache(GetCurrentProcess(), pOrigFunc, JMPCODE_LENGTH);

				return true;
			}
		}
	}
	return false;
}

/*extern "C" _declspec(dllexport)*/ void fun1()
{
	int n = 1 + 1;
	int m = n * 6;
	std::cout << "123" << std::endl;
}

extern "C" _declspec(dllexport) void fun_base()
{
	std::cout << "zzz" << std::endl;
}



/*extern "C" _declspec(dllexport)*/ void fun2()
{
	std::cout << "456" << std::endl;
	//fun1();		// 测试fun2拦截fun1后 循环递归的风险问题
	fun_base();
}

typedef void(*pFun)();

class C
{
public:
	void f() {}
};
int func(int unused, ...)
{
	va_list args;
	va_start(args, unused);
	return va_arg(args, int);
}


typedef void(Fun::*pf)(int, int);

typedef void(Fun_son::*pf_son)(int, int);

int testfun()
{
	pFun oldFun = &fun1;
	pFun newFun = &fun2;
	/*
	pf oldFun = &Fun::fun;
	pf newFun = &Fun::fun_v1;

	int old = func(0, &Fun::fun);
	int ne = func(0, &Fun::fun_v1);

	pf_son of1 = &Fun_son::fun;
	pf_son of2 = &Fun_son::fun_v1;

	int f1 = func(0, &Fun_son::fun);
	int f2 = func(0, &Fun_son::fun_v1);
	*/

	pFun baseFun = &fun_base;

	fun1();

	LPCVOID pfun1 = (LPCVOID)((INT64)baseFun + (0x140001140 - 0x140001023));
	LPCVOID pfun2 = (LPCVOID)((INT64)baseFun + (0x14000115E - 0x140001023));

	WriteJMP_x86(pfun1, pfun2);

	fun1();
	fun1();

	return 0;
}



// ScanAddress 适用x86(扫描范围相对小) x64范围太大
uintptr_t hanshu_dizhi; //记录特征码对应的地址
uintptr_t ScanAddress(HANDLE process, char *markCode, int nOffset, unsigned long dwReadLen = 4, uintptr_t StartAddr = 0x00400000, uintptr_t EndAddr = 0x7FFFFFFF, int InstructionLen = 0)
{
	//************处理特征码，转化成字节*****************
	if (strlen(markCode) % 2 != 0) return 0;
	//特征码长度
	int len = strlen(markCode) / 2;  //获取代码的字节数

	//将特征码转换成byte型 保存在m_code 中
	BYTE *m_code = new BYTE[len];
	for (int i = 0; i < len; i++)
	{
		//定义可容纳单个字符的一种基本数据类型。
		char c[] = { markCode[i * 2], markCode[i * 2 + 1], '\0' };
		//将参数nptr字符串根据参数base来转换成长整型数
		m_code[i] = (BYTE)::strtol(c, NULL, 16);
	}
	//每次读取游戏内存数目的大小
	const DWORD pageSize = 4096;

	// 查找特征码
	//每页读取4096个字节
	BYTE *page = new BYTE[pageSize];
	uintptr_t tmpAddr = StartAddr;
	//定义和特征码一样长度的标识
	int compare_one = 0;

	while (tmpAddr <= EndAddr)
	{
		::ReadProcessMemory(process, (LPCVOID)tmpAddr, page, pageSize, 0); //读取0x400000的内存数据，保存在page，长度为pageSize

		//在该页中查找特征码
		for (int i = 0; i < pageSize; i++)
		{
			if (m_code[0] == page[i])//有一个字节与特征码的第一个字节相同，则搜索
			{
				for (int j = 0; j<len - 1; j++)
				{
					if (m_code[j + 1] == page[i + j + 1])//比较每一个字节的大小，不相同则退出
					{
						compare_one++;
					}
					else
					{
						compare_one = 0;
						break;
					}//如果下个对比的字节不相等，则退出，减少资源被利用
				}

				if ((compare_one + 1) == len)
				{
					// 找到特征码处理
					//赋值时要给初始值，避免冲突
					uintptr_t dwAddr = tmpAddr + i + nOffset;
					uintptr_t ullRet = 0;
					::ReadProcessMemory(process, (void*)dwAddr, &ullRet, dwReadLen, 0);
					//cout<<dwAddr<<endl;
					//这里的dwAddr已经对应的是搜索到的地址
					//地址输出的也是10进制    需要转化为16进制 
					hanshu_dizhi = dwAddr;//记录地址
					if (InstructionLen)
					{
						ullRet += dwAddr + dwReadLen;
					}

					return ullRet;
				}
			}
		}

		tmpAddr = tmpAddr + pageSize - len;//下一页搜索要在前一页最后长度len 开始查找，避免两页交接中间有特征码搜索不出来
	}

	return 0;
}

void getFuncAddr()
{
	/*
	HWND hWnd;
	hWnd = FindWindow(NULL, "Tutorial-i386");

	DWORD PID;
	GetWindowThreadProcessId(hWnd, &PID);

	HANDLE lsProcess;
	lsProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);
	*/
	HMODULE oldhModule = GetModuleHandleA(NULL);
	LPCVOID pfun1 = nullptr;
	LPCVOID pfun2 = nullptr;
	if (oldhModule)
	{
		pfun1 = GetProcAddress(oldhModule, "?fun_v1@Fun_son@@UAEXHH@Z");
		pfun2 = GetProcAddress(oldhModule, "?fun2@@YAXXZ");
	}
	pFun oldFun = &fun_base;

	HANDLE lsProcess = GetCurrentProcess();

	std::cout << ScanAddress(lsProcess, "E97A460000", 0) << std::endl;
	//2983AC0400008D45D4对应下图的0x0042578F地址的特征码
	std::cout << hanshu_dizhi << std::endl;

	std::cout << "---------------------------" << std::endl;
}


int main()
{
	Fun* cA = new(std::nothrow) Fun_son;
	if (!cA)
		return 0;

	
	pFun baseFun = &fun_base;
	/*
	HMODULE oldhModule = GetModuleHandle(NULL);	// 获取本进程句柄
	LPCVOID baseFun = nullptr;
	if (oldhModule)
	{
		baseFun = GetProcAddress(oldhModule, "fun_base");	// 需要导出函数 才能通过GetProcAddress获得函数地址
	}
	*/
	LPCVOID pOldfun = nullptr;
	LPCVOID pOldfun_v1 = nullptr;
	//HANDLE lsProcess = GetCurrentProcess();
	
	//ScanAddress(lsProcess, "E9B3140000", 0);
	//std::cout << hanshu_dizhi << std::endl;
	//pOldfun = (LPCVOID)hanshu_dizhi;
	
	// 根据反汇编文件查找fun_base函数入口地址 再找出需要被替换的旧函数入口地址 计算2者之间的相对偏移 然后根据fun_base实际虚拟内存地址计算出要被替换的旧函数实际虚拟内存地址
	pOldfun = (LPCVOID)((INT64)baseFun + (0x12E0 - 0x1AA0));	

	//ScanAddress(lsProcess, "E9F4150000", 0);
	//std::cout << hanshu_dizhi << std::endl;
	//pOldfun_v1 = (LPCVOID)hanshu_dizhi;

	pOldfun_v1 = (LPCVOID)((INT64)baseFun + (0x12C0 - 0x1AA0));
	/*
	HMODULE oldhModule = GetModuleHandleA(NULL);	// 获取本进程句柄
	if (oldhModule)
	{
		pOldfun = GetProcAddress(oldhModule, "?fun@Fun_son@@UAEXHH@Z");	// 需要导出函数 才能通过GetProcAddress获得函数地址
		pOldfun_v1 = GetProcAddress(oldhModule, "?fun_v1@Fun_son@@UAEXHH@Z");
	}
	*/
	cA->fun(10, 12);
	cA->fun_v1(10, 10);

	HMODULE hModule = LoadLibrary("fun.dll");
	int error = GetLastError();
	if (hModule)
	{
		// 需要设置导出函数 才能通过GetProcAddress获得函数地址 函数名可以用 dumpbin /exports 动态库(执行文件) 来查看  (c++编译器会依据函数名、参数、类名等生成实际的函数名称)
		LPCVOID pNewfun = GetProcAddress(hModule, "?fun@Fun_son@@UAEXHH@Z");
		LPCVOID pNewfun_v1 = GetProcAddress(hModule, "?fun_v1@Fun_son@@UAEXHH@Z");

		WriteJMP_x86(pOldfun, pNewfun);
		WriteJMP_x86(pOldfun_v1, pNewfun_v1);
		//WriteJMP_x64(pOldfun, pNewfun);
		//WriteJMP_x64(pOldfun_v1, pNewfun_v1);	
	}
	cA->fun(10, 12);
	std::cout << cA->getn() << std::endl;
	cA->fun_v1(10, 10);
	std::cout << cA->getn() << std::endl;
	fun_base();
	error = GetLastError();

	FreeLibrary(hModule);
	return 0;
}
