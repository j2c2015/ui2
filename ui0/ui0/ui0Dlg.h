
// ui0Dlg.h: 헤더 파일
//

#pragma once
#include <d3d9.h>
#include "CConfigDialog.h"
#include "CEnrollDialog.h"
#include "CIdentifyDialog.h"
#include "CDialogPixel.h"
#include "J2CEngine_dll.h"

#pragma comment(lib,"d3d9.lib")
#define BPP 12
#define PARTIAL_WINDOW_WIDTH   128
#define PARTIAL_WINDOW_HEIGHT	128

// Cui0Dlg 대화 상자
class Cui0Dlg : public CDialogEx
{
// 생성입니다.
public:
	Cui0Dlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	virtual ~Cui0Dlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_UI0_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

	void initDllEngine(int nPartialWidth, int nPartialHeight);
	
	int ViewWidth_;
	int ViewHeight_;
	int HScrollPos_;
	int VScrollPos_;
	int HPageSize_;
	int VPageSize_;
	int delta;
	int m_nHdelta;

public:
	void cleanup();
	void cleanupForPixel();
	bool initD3D9();
	bool initRealTimerRenderView();
	bool initEnrollRenderView();
	bool initIdentifyRenderView();
	
	void	realTimeRender(unsigned char *buffer);
	void	enrollRender(unsigned char *buffer);
	void	identifyRender(unsigned char *buffer);
// 구현입니다.
protected:
public:
	HICON m_hIcon;
	IDirect3D9 *		m_pDirect3D9=0;

	// realtime render view
	IDirect3DDevice9 *	m_pDirect3DDevice=0;
	IDirect3DSurface9 *	m_pDirect3DSurfaceRender=0;
	RECT				m_RealTimeRenderViewport;

	// cscho (2018-12.12)
	IDirect3DDevice9 *	m_pDirect3DDeviceRealPartial = 0;
	IDirect3DSurface9 *	m_pDirect3DSurfaceRenderRealPartial = 0;
	RECT				m_RealTimeRenderViewportRealPartial;

	// enroll render view
	IDirect3DDevice9 *	m_pDirect3DDeviceForEnroll = 0;
	IDirect3DSurface9 *	m_pDirect3DSurfaceRenderForEnroll = 0;
	RECT				m_EnrollRenderViewport;
	

	IDirect3DDevice9 *	m_pDirect3DDeviceForIdentify = 0;
	IDirect3DSurface9 *	m_pDirect3DSurfaceRenderForIdentify = 0;
	RECT				m_IdentifyRenderViewport;

	/////////////////////////////////////////////////////////////////////////////
	// cscho (2018-12.11)
	IDirect3DDevice9*	m_pDirect3DDeviceForPixel = 0;
	IDirect3DSurface9*	m_pDirect3DSurfaceRenderForPixel = 0;
	RECT				m_PixelRenderViewport;

	IDirect3DDevice9*	m_pDirect3DDeviceForPixel2 = 0;
	IDirect3DSurface9*	m_pDirect3DSurfaceRenderForPixel2 = 0;	
	RECT				m_PixelRenderViewport2;
	int					m_nPartialWidth = PARTIAL_WINDOW_WIDTH;
	int					m_nPartialHeight = PARTIAL_WINDOW_HEIGHT;
	int GetPartialWidth() { return m_nPartialWidth; }
	int GetPartialHeight() { return m_nPartialHeight; }
	/////////////////////////////////////////////////////////////////////////////
	
	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonStop();
	afx_msg void OnBnClickedButtonIdentify();
	afx_msg void OnBnClickedButtonEnroll();
	CTabCtrl m_tabCtrl;
	CConfigDialog m_configDialog;
	CEnrollDialog m_enrollDialog;
	CIdentifyDialog m_identifyDialog;
	CDialogPixel	m_pixelDialog;
	CWnd *m_pWndShow;

	afx_msg void OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedOk();
	//char partialWindow_[PARTIAL_WINDOW_WIDTH * PARTIAL_WINDOW_HEIGHT];
	char* partialWindow_ = 0;
	void setPartialWindow(char *src)
	{
		//memcpy(partialWindow_, src, PARTIAL_WINDOW_WIDTH * PARTIAL_WINDOW_HEIGHT);
		memcpy(partialWindow_, src, m_nPartialWidth * m_nPartialHeight);
	}
	char *getPartialWindow()
	{
		return partialWindow_;
	}

	bool m_bIrisStart;
	void UpdateButtonState();
	
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedButtonTest();
};

extern Cui0Dlg *pDlg;
typedef void(*Renderer) (void *buffer, int width, int height, int depth);
void setRealTimeRenderer(Renderer r);
void setEnrollRenderer(Renderer r);
void setIdentifyRenderer(Renderer r);
