// UTOP_FC.h : UTOP_FC DLL ����ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CUTOP_FCApp
// �йش���ʵ�ֵ���Ϣ������� UTOP_FC.cpp
//

class CUTOP_FCApp : public CWinApp
{
public:
	CUTOP_FCApp();

// ��д
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};

extern CUTOP_FCApp theApp;