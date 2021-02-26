# HotUpdateCpp

热更新c++函数

windows和linux下原理几乎相同

大概原理:

	使寄存器中指令地址指向新函数入口
	把原函数入口处的指令改为跳转到新函数入口指令
	调用原函数->跳转新函数指令放入寄存器->调用到新函数
	
	查询原函数地址、新函数地址
	修改主程序代码段熟悉为可读写
	在原函数地址入口处改写内存内容为跳转到新函数入口 (跳转汇编指令)

注意：
	1、不要出现循环调用  即fun2替换fun1后 fun2的逻辑中又调用了fun1(实际上就是不停的递归调用fun2)
	2、不要改动类内成员变量
	3、含有静态成员的不可以热更
	4、多线程使用的不要热更

windows:

	重点是原函数地址和目的函数地址获取的方法
	dumpbin的作用（反汇编exe 得到函数入口的地址(实际运行时地址会变但相对地址不变) 函数入口的特征码(运行时也保持不变 属于代码段内容)）
	release模式下某些函数会被优化掉 使用dumpbin反汇编无法得到函数入口  设置函数导出时则无法优化掉函数入口地址 可用该函数获取基址
	x86和x64下的函数跳转汇编指令实现区别
	通过扫描特征码获取函数地址适用x86 扫描范围较小
	增量链接启动：函数入口在@ILT处 通过增量链接表来跳转
	增量链接关闭：函数入口直接在定义处
	
	main.cpp fun1.h fun1.cpp 编译生成.exe
	fun_dll中文件编译生成.dll
	
