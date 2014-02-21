#ifndef _LIVE_MANAGER_H_
#define _LIVE_MANAGER_H_
//�÷��ǣ�����Ҫ��¼�������ڵĿ�ʼ����Createinstance�������ٻ��߲���Ҫ�����������ڵ�ʱ�����DestroyInstance��

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
	//Parameter: hWnd��Ҫ����Ĵ��ڵľ��
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
	CString m_HeartName;//��¼�������������
};

#endif