#pragma once
#include <vector>
#include "SafeLock.h"

using namespace std;

//这个类提供在公共区域存储自定义格式的数据，并提供给定key查询相应信息的功能
#define VALUE_LENGTH 2084
#define HEARTNAME_LEN 128
#define BUFFER_HEAD_SIZE 4

#define NAME_MAPPING_FILE _T("urladdr_telmeeting")
#define GLOBAL_SPACE_SIZE (1024*100)


struct WebDlgInfo 
{
	HWND hThis;             //存储自身窗口的句柄
	TCHAR strHeartName[HEARTNAME_LEN]; //父窗口的句柄
	TCHAR strUniqueTag[VALUE_LENGTH];    //存储标识不同程序的标记
	TCHAR strValue[VALUE_LENGTH];
	WebDlgInfo(){
		memset(this, 0, sizeof(WebDlgInfo));
	}
	~WebDlgInfo(){
		memset(this, 0, sizeof(WebDlgInfo));
	}
	WebDlgInfo(WebDlgInfo const& obj){
		hThis = obj.hThis;
		memcpy(strHeartName, obj.strHeartName, HEARTNAME_LEN);
		memcpy(strUniqueTag, obj.strUniqueTag, sizeof(strUniqueTag));
		memcpy(strValue, obj.strValue, sizeof(strValue));
	}
	WebDlgInfo& operator = (WebDlgInfo const& obj){
		if (this != &obj){
			hThis = obj.hThis;
			memcpy(strHeartName, obj.strHeartName, HEARTNAME_LEN);
			memcpy(strUniqueTag, obj.strUniqueTag, sizeof(strUniqueTag));
			memcpy(strValue, obj.strValue, sizeof(strValue));
		}
		return *this;
	}
};

class GlobalBuffer
{
public:
	GlobalBuffer(void);
	~GlobalBuffer(void);
	static void push(HWND hNewDlg, CString strHeartName, CString strUniqueTag, CString strValue);
	static BOOL getHandle(CString strUniqueTag, HWND& hWnd);
	static void remove(CString strUniqueTag);
	//如果没有这个指定的Tag的值，就返回空字符串
	static CString getValue(CString strUniqueTag);
	//更新共享内存中的数据，返回值表示是否与原来的值不同，如果不同也就是说需要更新那么返回TRUE，否则返回FALSE
	//注意这个函数只会修改已有的数据，不会添加新的数据
	static BOOL updateValue(CString strUniqueTag, CString strValue);
	static size_t getCount(){
		return s_Buffer.size();
	}
private:
	//下面两个函数负责在本地内存和共享内存之间进行读写
	//共享内存中的数据格式是这样的：
	//00--32 bit：记录一共有多少条WebDlgInfo信息
	//32--end   ：记录各个WebDlgInfo的具体信息
	static BOOL BufferFromMemory();
	static BOOL BufferToMemory();

	static void PrintBuffer();

private:
	static std::vector<WebDlgInfo> s_Buffer;
	static CRITICAL_SECTION s_cs;
	static SafeLocker s_Locker;
};
