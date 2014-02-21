#include "StdAfx.h"
#include ".\globalbuffer.h"
#include <algorithm>

std::vector<WebDlgInfo> GlobalBuffer::s_Buffer;
CRITICAL_SECTION GlobalBuffer::s_cs;
SafeLocker GlobalBuffer::s_Locker;


BOOL WraperLoopFunc(BOOL (*pFunc)()){
	return pFunc();
	//int count = 0;
	//while (++count < 100){
	//	if (pFunc()){
	//		OutputDebugString(_T("ѭ��ִ��   �ɹ�"));
	//		return TRUE;
	//	}
	//	Sleep(10);
	//	OutputDebugString(_T("ѭ��ִ��  failed once"));
	//}
	//OutputDebugString(_T("ѭ��ִ��   ʧ��"));
	return FALSE;
}


GlobalBuffer::GlobalBuffer(void)
{
}

GlobalBuffer::~GlobalBuffer(void)
{
}

void GlobalBuffer::push(HWND hNewDlg, CString strHeartName, CString strUniqueTag, CString strValue){
	//��������֮ǰ��Ҫ�Ƚ��ڴ��е����ݸ��Ƴ������жԱȣ�
	//����Ѿ�������ô�Ͳ���Ҫ��Ĳ������ݣ����û����ô�Ͳ������ݣ������ڲ������ݵĹ�����ҩ�������򱣻�
	if (strUniqueTag.IsEmpty()){
		return ;
	}
	OutputDebugString(_T("[GlobalBuffer::push]"));
	if (!WraperLoopFunc(BufferFromMemory))
		return ;

	char* pbuf = strUniqueTag.GetBuffer();
	for (std::vector<WebDlgInfo>::iterator iter = s_Buffer.begin(); iter != s_Buffer.end(); ++iter){
		if (memcmp(iter->strUniqueTag, pbuf, min(VALUE_LENGTH, strUniqueTag.GetLength()) * sizeof(TCHAR)) == 0){
			strUniqueTag.ReleaseBuffer();
			OutputDebugString(iter->strUniqueTag);
			return ;
		}
	}
	strUniqueTag.ReleaseBuffer();
	WebDlgInfo info;
	info.hThis = hNewDlg;
	memcpy(info.strHeartName, strHeartName, min(HEARTNAME_LEN, strHeartName.GetLength()) * sizeof(TCHAR));
	memcpy(info.strUniqueTag, strUniqueTag, min(VALUE_LENGTH, strUniqueTag.GetLength()) * sizeof(TCHAR));
	memcpy(info.strValue, strValue, min(VALUE_LENGTH, strValue.GetLength()) * sizeof(TCHAR));
	s_Buffer.push_back(info);
	WraperLoopFunc(BufferToMemory);
	PrintBuffer();
}

BOOL GlobalBuffer::getHandle(CString strUniqueTag, HWND& hWnd){
	OutputDebugString(_T("[GlobalBuffer::getHandle]"));
	hWnd = NULL;
	if (strUniqueTag.IsEmpty()){
		OutputDebugString(_T("Ψһ��ʾΪ��"));
		return FALSE;
	}
	if (!WraperLoopFunc(BufferFromMemory))
		return FALSE;
	PrintBuffer();
	char* pbuf = strUniqueTag.GetBuffer();
	for (std::vector<WebDlgInfo>::iterator iter = s_Buffer.begin(); iter != s_Buffer.end(); ++iter){
		if(memcmp(iter->strUniqueTag, pbuf, min(VALUE_LENGTH, strUniqueTag.GetLength()) * sizeof(TCHAR)) == 0){
			strUniqueTag.ReleaseBuffer();
			OutputDebugString(iter->strUniqueTag);
			hWnd = iter->hThis;
			break;
		}
	}
	strUniqueTag.ReleaseBuffer();
	return TRUE;
}

void GlobalBuffer::remove(CString strUniqueTag){
	OutputDebugString(_T("ɾ���ڴ��еģ�") + strUniqueTag);
	if (!WraperLoopFunc(BufferFromMemory)) 
		return ;
	for (std::vector<WebDlgInfo>::iterator iter = s_Buffer.begin(); iter != s_Buffer.end(); ++iter){
		if(memcmp(iter->strUniqueTag, strUniqueTag, min(VALUE_LENGTH, strUniqueTag.GetLength()) * sizeof(TCHAR)) == 0){
			s_Buffer.erase(iter);
			break;
		}
	}
	WraperLoopFunc(BufferToMemory);
	PrintBuffer();
	OutputDebugString(_T("�ɹ��˳� [GlobalBuffer::remove]"));
}
CString GlobalBuffer::getValue(CString strUniqueTag)
{
	OutputDebugString(_T("[GlobalBuffer::getValue]") + strUniqueTag);
	if (!WraperLoopFunc(BufferFromMemory)){
		OutputDebugString(_T("[BufferFromMemory] ʧ��"));
		DWORD dwErr = GetLastError();
		CString str;
		str.Format(_T("%d"), dwErr);
		OutputDebugString(str);
		return _T("");
	}
	PrintBuffer();
	for (std::vector<WebDlgInfo>::iterator iter = s_Buffer.begin(); iter != s_Buffer.end(); ++iter){
		if(memcmp(iter->strUniqueTag, strUniqueTag, min(VALUE_LENGTH, strUniqueTag.GetLength()) * sizeof(TCHAR)) == 0){
			return iter->strValue;
		}
	}
	return _T("");
}
BOOL GlobalBuffer::updateValue(CString strUniqueTag, CString strValue)
{
	OutputDebugString(_T("��������: ") + strValue);
	BOOL bNeedUpdate = FALSE;
	if (!WraperLoopFunc(BufferFromMemory)){
		return FALSE;
	}
	//���µĸ��µ�����
	for (std::vector<WebDlgInfo>::iterator iter = s_Buffer.begin(); iter != s_Buffer.end(); ++iter){
		if(memcmp(iter->strUniqueTag, strUniqueTag, min(VALUE_LENGTH, strUniqueTag.GetLength()) * sizeof(TCHAR)) == 0){
			if (memcmp(iter->strValue, strValue, min(VALUE_LENGTH, strValue.GetLength()) * sizeof(TCHAR)) != 0){
				bNeedUpdate = TRUE;
			}
			memset(iter->strValue, 0, VALUE_LENGTH * sizeof(TCHAR));
			memcpy(iter->strValue, strValue, min(VALUE_LENGTH, strValue.GetLength()) * sizeof(TCHAR));
			break;
		}
	}
	WraperLoopFunc(BufferToMemory);
	PrintBuffer();
	return bNeedUpdate;
}

//�����������������ڱ����ڴ�͹����ڴ�֮����ж�д
//�����ڴ��е����ݸ�ʽ�������ģ�
//00--32 bit����¼һ���ж�����WebDlgInfo��Ϣ
//32--end   ����¼����WebDlgInfo�ľ�����Ϣ
BOOL GlobalBuffer::BufferFromMemory(){
	OutputDebugString(_T("[GlobalBuffer::BufferFromMemory]"));
	s_Locker.Lock();
	//�����ݴӹ����ڴ��ж�ȡ����
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = NULL;

	HANDLE hMapFile = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, NAME_MAPPING_FILE);
	if (hMapFile == NULL){
		s_Locker.Unlock();
		OutputDebugString(_T("[OpenFileMapping] ʧ��"));
		return FALSE;
	}
	LPVOID pOldData = MapViewOfFile(hMapFile, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, GLOBAL_SPACE_SIZE);
	if (pOldData == NULL){
		CloseHandle(hMapFile);
		hMapFile = NULL;
		s_Locker.Unlock();
		OutputDebugString(_T("[MapViewOfFile] ʧ��"));
		return FALSE;
	}
	int nSize = *(int*)pOldData;
	if (nSize == 0){//û�����ݲ���Ҫд
		UnmapViewOfFile(pOldData);
		CloseHandle(hMapFile);
		s_Locker.Unlock();
		return TRUE;
	}
	s_Buffer.clear();
	WebDlgInfo* pData = (WebDlgInfo*)(((int*)pOldData) + BUFFER_HEAD_SIZE);
	for (int i = 0; i < nSize; ++i){
		WebDlgInfo info;
		info.hThis = pData->hThis;
		memcpy(info.strHeartName, pData->strHeartName, sizeof(info.strHeartName));
		memcpy(info.strUniqueTag, pData->strUniqueTag, sizeof(info.strUniqueTag));
		memcpy(info.strValue, pData->strValue, sizeof(info.strValue));
		s_Buffer.push_back(info);
		pData ++;
	}
	UnmapViewOfFile(pOldData);
	CloseHandle(hMapFile);
	s_Locker.Unlock();
	return TRUE;
}

BOOL GlobalBuffer::BufferToMemory(){
	OutputDebugString(_T("GlobalBuffer::BufferToMemory"));
	s_Locker.Lock();
	//������д�뵽�����ڴ�
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = NULL;

	HANDLE hMapFile = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, NAME_MAPPING_FILE);
	if (hMapFile == NULL){
		s_Locker.Unlock();
		return FALSE;
	}
	LPVOID pOldData = MapViewOfFile(hMapFile, FILE_MAP_READ |FILE_MAP_WRITE, 0, 0, GLOBAL_SPACE_SIZE);
	if (pOldData == NULL){
		UnmapViewOfFile(pOldData);
		CloseHandle(hMapFile);
		hMapFile = NULL;
		s_Locker.Unlock();
		return FALSE;
	}

#pragma warning(disable : 4267)
	*(size_t*)pOldData = s_Buffer.size();
#pragma warning(default : 4267)

	OutputDebugString(_T("vector�е�����:"));
	WebDlgInfo* pData = (WebDlgInfo*)((int*)pOldData + BUFFER_HEAD_SIZE);
	for (std::vector<WebDlgInfo>::const_iterator iter = s_Buffer.begin(); iter != s_Buffer.end(); ++iter){
		*pData = *iter;
		pData ++;
	}
	OutputDebugString(_T("end"));
	UnmapViewOfFile(pOldData);
	CloseHandle(hMapFile);
	s_Locker.Unlock();
	return TRUE;
}


void GlobalBuffer::PrintBuffer()
{
	OutputDebugString(_T("[GlobalBuffer::PrintBuffer] --- begin"));
	for (std::vector<WebDlgInfo>::const_iterator iter = s_Buffer.begin(); iter != s_Buffer.end(); ++iter){
		OutputDebugString((*iter).strUniqueTag);
		OutputDebugString((*iter).strValue);
	}
	OutputDebugString(_T("[GlobalBuffer::PrintBuffer] --- end"));
}