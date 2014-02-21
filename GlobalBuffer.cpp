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
	//		OutputDebugString(_T("循环执行   成功"));
	//		return TRUE;
	//	}
	//	Sleep(10);
	//	OutputDebugString(_T("循环执行  failed once"));
	//}
	//OutputDebugString(_T("循环执行   失败"));
	return FALSE;
}


GlobalBuffer::GlobalBuffer(void)
{
}

GlobalBuffer::~GlobalBuffer(void)
{
}

void GlobalBuffer::push(HWND hNewDlg, CString strHeartName, CString strUniqueTag, CString strValue){
	//插入数据之前需要先将内存中的数据复制出来进行对比，
	//如果已经有了那么就不需要真的插入数据，如果没有那么就插入数据，并且在操作数据的过程中药进行区域保护
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
		OutputDebugString(_T("唯一标示为空"));
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
	OutputDebugString(_T("删除内存中的：") + strUniqueTag);
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
	OutputDebugString(_T("成功退出 [GlobalBuffer::remove]"));
}
CString GlobalBuffer::getValue(CString strUniqueTag)
{
	OutputDebugString(_T("[GlobalBuffer::getValue]") + strUniqueTag);
	if (!WraperLoopFunc(BufferFromMemory)){
		OutputDebugString(_T("[BufferFromMemory] 失败"));
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
	OutputDebugString(_T("更新数据: ") + strValue);
	BOOL bNeedUpdate = FALSE;
	if (!WraperLoopFunc(BufferFromMemory)){
		return FALSE;
	}
	//将新的更新到缓存
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

//下面两个函数负责在本地内存和共享内存之间进行读写
//共享内存中的数据格式是这样的：
//00--32 bit：记录一共有多少条WebDlgInfo信息
//32--end   ：记录各个WebDlgInfo的具体信息
BOOL GlobalBuffer::BufferFromMemory(){
	OutputDebugString(_T("[GlobalBuffer::BufferFromMemory]"));
	s_Locker.Lock();
	//将数据从共享内存中读取出来
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = NULL;

	HANDLE hMapFile = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, NAME_MAPPING_FILE);
	if (hMapFile == NULL){
		s_Locker.Unlock();
		OutputDebugString(_T("[OpenFileMapping] 失败"));
		return FALSE;
	}
	LPVOID pOldData = MapViewOfFile(hMapFile, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, GLOBAL_SPACE_SIZE);
	if (pOldData == NULL){
		CloseHandle(hMapFile);
		hMapFile = NULL;
		s_Locker.Unlock();
		OutputDebugString(_T("[MapViewOfFile] 失败"));
		return FALSE;
	}
	int nSize = *(int*)pOldData;
	if (nSize == 0){//没有数据不需要写
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
	//将数据写入到共享内存
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

	OutputDebugString(_T("vector中的数据:"));
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