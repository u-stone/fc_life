#ifndef _LIVE_MANAGER_H_
#define _LIVE_MANAGER_H_
//用法是：在需要记录生命周期的开始调用Createinstance，在销毁或者不想要管理生命周期的时候调用DestroyInstance。

class LiveManager;

static LiveManager * s_pObj = NULL;

class LiveManager{
private:
	LiveManager(CString strName){
		m_HeartName = strName;
		m_hWnd = CreateEvent(NULL, TRUE, FALSE, _T("event_") + m_HeartName);
	}
	~LiveManager(){
		if (m_hWnd)
			CloseHandle(m_hWnd);
		m_hWnd = NULL;
	}
public:
	//Parameter: hWnd是要管理的窗口的句柄
	static LiveManager* GetInstance(){
		if (s_pObj == NULL){
			CString str;
			GUID guid;
			CoCreateGuid(&guid);
			str.Format(_T("%08X-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X") 
				,   guid.Data1 
				,   guid.Data2 
				,   guid.Data3 
				,   guid.Data4[0],   guid.Data4[1] 
			,   guid.Data4[2],   guid.Data4[3],   guid.Data4[4],   guid.Data4[5] 
			,   guid.Data4[6],   guid.Data4[7] );

			s_pObj = new LiveManager(str);
		}
		return s_pObj;
	}
	static void ExitInstance(){
		if (s_pObj != NULL)
			delete s_pObj;
	}
	CString GetHeartName(){
		s_pObj = GetInstance();
		if (s_pObj == NULL)
			return _T("");
		return s_pObj->m_HeartName;
	}
private:
	HANDLE m_hWnd;
	CString m_HeartName;//记录跳动心脏的名字
};

#endif