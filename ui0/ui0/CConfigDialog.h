#pragma once

#include <d3d9.h>
#include "CTestWnd.h"
#include "CCropWnd.h"
#include "CPartialWnd.h"

#define ID_TEST_WND				(101)
#define ID_CROP_LIST			(102)
#define ID_ENROLL_LIST			(103)
#define ID_CROP_WND				(104)
#define ID_PARTIAL_WND			(105)
#define ID_CROP_LOAD_BTN		(106)
#define ID_CROP_CLEAR_BTN		(107)
#define ID_ENROLL_LOAD_BTN		(108)
#define ID_ENROLL_CLEAR_BTN		(109)
#define ID_PARTIAL_BTN			(110)
#define ID_VIEWMODE_BTN			(111)
#define ID_SHOWVALUE_CHECK		(112)
#define ID_SHOWVALUE_EDIT		(113)
#define ID_RESIZE_LOAD_BTN		(114)
#define ID_RESIZE_CLEAR_BTN		(115)
#define ID_RESIZE_LIST			(116)
#define ID_REALSAVE_BTN			(117)
#define ID_REALSAVE_EDIT		(118)
#define ID_REALSAVE_BROWSE_BTN	(119)

#define RENDER_OFFSET_X			(312)
#define RENDER_OFFSET_Y			(30)
#define DEF_LIST_WIDTH			(400)

// CConfigDialog 대화 상자

class CConfigDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CConfigDialog)

public:
	CConfigDialog(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CConfigDialog();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CONFIGURATION };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	bool initView(CWnd* pWnd, RECT& rt, IDirect3DDevice9*& pDirect3DDevice, IDirect3DSurface9*& pDirect3DSurfaceRender, int nSurfaceWidth, int nSurfaceHeight);
	void render(unsigned char *buffer);
	void renderPartial(unsigned char *pBuffer);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);	
	
	int		m_nSrcWidth;
	int		m_nSrcHeight;
	CFont m_fontControl;

	BOOL InitControl();
	void DrawRectangle(IDirect3DDevice9* pDirect3DDeviceForPixel);

	struct LISTSORTPARAM
	{
		int nSortColumn;
		CListCtrl* pWndList;
		bool bSortAscending;
	};
	bool m_bSortAscendingCrop;
	bool m_bSortAscendingEnroll;
	bool m_bSortAscendingResize;

	enum eVIEW_MODE
	{
		eVIEW_READY = 0,
		eVIEW_REAL,
		eVIEW_IMAGE,
		eVIEW_MAX,
	};
	eVIEW_MODE m_eViewMode;
	eVIEW_MODE GetViewMode() { return m_eViewMode; }
	void UpdateViewMode(eVIEW_MODE eViewMode) { m_eViewMode = eViewMode; UpdateViewModeButton(); }
	CButton m_btnViewMode;
	CButton m_btnPartial;
	CButton m_chkRealSave;
	CButton m_btnRealSaveBrowse;
	CEdit m_editRealSave;
	TCHAR m_szCropFolder[_MAX_PATH];
	CTestWnd m_wndTest;

	CButton m_btnLoadCrop;
	CButton m_btnClearCrop;
	CListCtrl m_listCrop;
	CButton m_btnLoadEnroll;
	CButton m_btnClearEnroll;
	CListCtrl m_listEnroll;

	CButton m_btnLoadResize;
	CButton m_btnClearResize;
	CListCtrl m_listResize;

	CCropWnd m_wndCropView;	
	CButton m_chkShowValue;
	bool m_bPartialShowValue;
	bool GetPartialShowValue() { return m_bPartialShowValue; }
	void UpdatePartialValue(CString strValue);
	CRect GetPartialRect();
	CEdit m_editPartialValue;
	CPartialWnd m_wndPartialView;

	void LoadListData(CListCtrl* pList, TCHAR* pszFindExt, int nSortColumn, bool& bSortAscending);
	void ClearListData(CListCtrl* pList, TCHAR* pszFindExt);
	static int CALLBACK ListSortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	void UpdateViewModeButton();
	void CallPartailRender();
	void DrawPartial(IDirect3DDevice9* pDirect3DDeviceForPixel, unsigned char* pBuffer);
	void UpdateAllView(TCHAR* pszPath);
	void LoadFixModeImage(TCHAR* pszImagePath);
	void EyeFindTest(TCHAR* pszPath);
	void EyeFindTestDir(TCHAR* pszPath);

	Bitmap* m_pBmpFixCrop;	
	Bitmap* GetFixCropBmp() { return m_pBmpFixCrop; }
	Bitmap* m_pBmpFixPartial;
	Bitmap* GetFixPartialBmp() { return m_pBmpFixPartial; }
	
	afx_msg void OnBnClickedButtonShowValue();
	afx_msg void OnBnClickedButtonViewMode();
	afx_msg void OnBnClickedButtonPartial();
	afx_msg void OnBnClickedButtonRealSave();
	afx_msg void OnBnClickedButtonRealSaveBrowse();
	afx_msg void OnBnClickedButtonCropLoad();
	afx_msg void OnBnClickedButtonCropClear();
	afx_msg void OnBnClickedButtonResizeLoad();
	afx_msg void OnBnClickedButtonResizeClear();
	afx_msg void OnBnClickedButtonPartialLoad();
	afx_msg void OnBnClickedButtonPartialClear();
	afx_msg void OnNMDblclkListCrop(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkListEnroll(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkListResize(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnHdnItemclickCropList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT OnUmFileEyeFindTest(WPARAM wParam, LPARAM lParam);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};

struct EyeFindParam
{
	CConfigDialog* pThis;
	TCHAR szPath[_MAX_PATH];
	EyeFindParam()
	{
		pThis = NULL;
		memset(szPath, 0x00, sizeof(szPath));
	}
};

extern CConfigDialog *pConfigDlg;
