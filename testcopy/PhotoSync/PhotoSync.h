
// PhotoSync.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CPhotoSyncApp:
// �йش����ʵ�֣������ PhotoSync.cpp
//

class CPhotoSyncApp : public CWinApp
{
public:
	CPhotoSyncApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CPhotoSyncApp theApp;