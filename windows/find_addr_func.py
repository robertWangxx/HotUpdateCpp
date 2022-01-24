import sys
import re
import os

#是否是增量链接编译的
ILT = 0
#没考虑重载函数
#exe反汇编文件查询函数地址
deslst = ['fun_base', 'fun_v1@Fun@']
#dll导出文件查询函数符号
dllsymbollst = ['fun_v1@Fun@', 'fun@Fun_son@']

# 匹配算法
def match(src, des):
    global ILT
    src = src.strip()
    des = des.strip()

    index = src.find(des)
    obj = re.search(r':$', src, re.I)

    if index >= 0 and obj:
        if ILT == 1:
            if re.search(r'^@ILT', src, re.I):
                return True
        else:
            return True

    return False


def matchlst(src):
    global deslst
    for ele in deslst:
       if match(src, ele):
           deslst.remove(ele)
           return True

    return False

# 读取文件
def getfunaddr(cmd):
    global ILT
    global deslst
    if len(deslst) <= 0:
        return

    '''
    fo = os.popen(cmd)
    while (1):
        srcStr = fo.readline()
        if not srcStr:
            break
        if srcStr.find("@ILT") >= 0:
            ILT = 1
            break
    fo.close()
    '''
    fo = os.popen(cmd)

    srcstr = ''
    flag = True
    while (1):
        if flag:
            srcstr = fo.readline()
        else:
            flag = True

        if not srcstr:
            break
        elif matchlst(srcstr):
            tempstr = srcstr.strip()
            srcstr = fo.readline()
            flag = False
            lst = srcstr.split(':', 1)
            if (len(lst) > 0):
                tmp = lst[0].strip()
                print('{}0x{}'.format(tempstr, re.split(r'^0+', tmp)[1]))

        if len(deslst) <= 0:
            break

    fo.close()

'''
------------------------------------------------------------------------------------------------------------------------
'''

def matchsymbol(src, des):
    src = src.strip()
    des = des.strip()

    index = src.find(des)
    if index >= 1:
        print(src[(index - 1):].strip())
        return True
    return False

def matchsymbollst(src):
    global dllsymbollst
    for ele in dllsymbollst:
        if matchsymbol(src, ele):
            dllsymbollst.remove(ele)
            return True
    return False

def getfunsymbol(cmd):
    global dllsymbollst
    if len(dllsymbollst) <= 0:
        return
    fo = os.popen(cmd)
    srcstr = ''
    while (1):
        srcstr = fo.readline()
        if not srcstr:
            break
        else:
            matchsymbollst(srcstr)
        if len(dllsymbollst) <= 0:
            break
    fo.close()

'''
------------------------------------------------------------------------------------------------------------------------
'''

if __name__ == '__main__':
    cmd = r'D:\progaram\vs2013\VC\bin\amd64\dumpbin /exports fun.dll'
    getfunsymbol(cmd)
    print('--------------------------------------------------------------------------')
    cmd = r'D:\progaram\vs2013\VC\bin\amd64\dumpbin /disasm test_hotmore_dll.exe'
    getfunaddr(cmd)

