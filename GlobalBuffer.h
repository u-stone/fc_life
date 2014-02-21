#pragma once
#include <vector>
#include "SafeLock.h"

using namespace std;

//������ṩ�ڹ�������洢�Զ����ʽ�����ݣ����ṩ����key��ѯ��Ӧ��Ϣ�Ĺ���
#define VALUE_LENGTH 2084
#define HEARTNAME_LEN 128
#define BUFFER_HEAD_SIZE 4

#define NAME_MAPPING_FILE _T("urladdr_telmeeting")
#define GLOBAL_SPACE_SIZE (1024*100)


struct WebDlgInfo 
{
	HWND hThis;             //�洢�����ڵľ��
	TCHAR strHeartName[HEARTNAME_LEN]; //�����ڵľ��
	TCHAR strUniqueTag[VALUE_LENGTH];    //�洢��ʶ��ͬ����ı��
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
	//���û�����ָ����Tag��ֵ���ͷ��ؿ��ַ���
	static CString getValue(CString strUniqueTag);
	//���¹����ڴ��е����ݣ�����ֵ��ʾ�Ƿ���ԭ����ֵ��ͬ�������ͬҲ����˵��Ҫ������ô����TRUE�����򷵻�FALSE
	//ע���������ֻ���޸����е����ݣ���������µ�����
	static BOOL updateValue(CString strUniqueTag, CString strValue);
	static size_t getCount(){
		return s_Buffer.size();
	}
private:
	//�����������������ڱ����ڴ�͹����ڴ�֮����ж�д
	//�����ڴ��е����ݸ�ʽ�������ģ�
	//00--32 bit����¼һ���ж�����WebDlgInfo��Ϣ
	//32--end   ����¼����WebDlgInfo�ľ�����Ϣ
	static BOOL BufferFromMemory();
	static BOOL BufferToMemory();

	static void PrintBuffer();

private:
	static std::vector<WebDlgInfo> s_Buffer;
	static CRITICAL_SECTION s_cs;
	static SafeLocker s_Locker;
};
