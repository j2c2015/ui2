#pragma once
#include <d3d9.h>

// CEnrollDialog 대화 상자

class CEnrollDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CEnrollDialog)

public:
	CEnrollDialog(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CEnrollDialog();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_ENROLLMENT };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLbnSelchangeList1();
	afx_msg void OnBnClickedButtonEnroll();
	virtual BOOL OnInitDialog();
	afx_msg void OnEnChangeEditEnroll();

	bool initView();
	void render(unsigned char *buffer);
	IDirect3D9 *		m_pDirect3D9 = 0;
};

extern CEnrollDialog *pEnrollDlg;
