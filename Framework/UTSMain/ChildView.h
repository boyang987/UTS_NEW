
// ChildView.h : CChildView ��Ľӿ�
//


#pragma once

// CChildView ����
class CMainFrame;
class CChildView : public CWnd
{
// ����
public:
	CChildView();

// ����
public:
    CMainFrame *m_pMainFrame;

// ����
public:

// ��д
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// ʵ��
public:
	virtual ~CChildView();

	// ���ɵ���Ϣӳ�亯��
protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
public:
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};
