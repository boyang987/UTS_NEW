// UTOP_ChangeSetting.h : UTOP_ChangeSetting DLL ����ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CUTOP_ChangeSettingApp
// �йش���ʵ�ֵ���Ϣ������� UTOP_ChangeSetting.cpp
//

class CUTOP_ChangeSettingApp : public CWinApp
{
public:
	CUTOP_ChangeSettingApp();

// ��д
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};

extern CUTOP_ChangeSettingApp theApp;