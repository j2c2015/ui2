#pragma once
#include <d3d9.h>

// CIdentifyDialog 대화 상자

class CIdentifyDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CIdentifyDialog)

public:
	CIdentifyDialog(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CIdentifyDialog();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_IDENTIFY };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonIdentify();
	virtual BOOL OnInitDialog();

	bool initView();
	void render(unsigned char *buffer);
	void setIdentifyText(CString &str);
	IDirect3D9 *		m_pDirect3D9 = 0;
	afx_msg void OnPaint();
};

extern CIdentifyDialog *pIdentifyDlg;
