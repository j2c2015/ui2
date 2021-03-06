
// ui0Dlg.cpp: 구현 파일
//

#include "stdafx.h"
#include "ui0.h"
#include "ui0Dlg.h"
#include "afxdialogex.h"
#include "J2CEngine_dll.h"
#include "Resource.h"
#include <iostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")

Cui0Dlg *pDlg = 0;
Renderer _realtime=0;
Renderer _enroll=0;
Renderer _identify=0;

const int m_nLineSize = 15;
const int m_nBasic = 95;
const int m_nRepeat = 10;

void setRealTimeRenderer(Renderer r)
{
	_realtime = r;
}
void setEnrollRenderer(Renderer r)
{
	_enroll = r;
}
void setIdentifyRenderer(Renderer r)
{
	_identify = r;
}
void realTimeRender(void *buffer, int width, int height, int depth)
{
	if (_realtime == 0)
		return;

	_realtime(buffer, width, height, depth);
#if 0
	if (pDlg == 0)
		return;
	pDlg->realTimeRender((unsigned char *)buffer);
#endif
}

void enrollRender(void *buffer, int width, int height, int depth)
{
	if (_enroll == 0)
		return;

	_enroll(buffer, width, height, depth);
#if 0
	if (pDlg == 0)
		return;
	pDlg->enrollRender((unsigned char *)buffer);
#endif
}

void identifyRender(void *buffer, int width, int height, int depth)
{
	if (_identify == 0)
		return;
	_identify(buffer, width, height, depth);
#if 0
	if (pDlg == 0)
		return;
	pDlg->identifyRender((unsigned char *)buffer);
#endif
}

void enrollCb( bool success)
{
	if (success == false)
	{
		AfxMessageBox(_T("Enroll failed"), MB_OK);
	}
	else {
		AfxMessageBox(_T("Enroll success"), MB_OK);
	}
}

void identifyCb(char *id, bool success)
{
	if (success == true)
	{
		CString str;
		str.Format(_T("Found. %s"), id);
		AfxMessageBox(str, MB_OK);
	}
	else
	{
		AfxMessageBox(_T("Not found"), MB_OK);
	}
}
// Cui0Dlg 대화 상자



Cui0Dlg::Cui0Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_UI0_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pWndShow = NULL;
	m_bIrisStart = false;
}

Cui0Dlg::~Cui0Dlg()
{
	if (partialWindow_)
	{
		free(partialWindow_);
		partialWindow_ = 0;
	}
}

void Cui0Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB1, m_tabCtrl);
}

BEGIN_MESSAGE_MAP(Cui0Dlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_START, &Cui0Dlg::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &Cui0Dlg::OnBnClickedButtonStop)
	ON_BN_CLICKED(IDC_BUTTON_IDENTIFY, &Cui0Dlg::OnBnClickedButtonIdentify)
	ON_BN_CLICKED(IDC_BUTTON_ENROLL, &Cui0Dlg::OnBnClickedButtonEnroll)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &Cui0Dlg::OnTcnSelchangeTab1)
	ON_BN_CLICKED(IDOK, &Cui0Dlg::OnBnClickedOk)
	ON_WM_VSCROLL()
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_BUTTON_TEST, &Cui0Dlg::OnBnClickedButtonTest)
END_MESSAGE_MAP()


// Cui0Dlg 메시지 처리기

BOOL Cui0Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	theMainDlg = this;

	pDlg = this;
	bool bret;
#if 1
	initD3D9();
	//bret = initRealTimerRenderView();
	//bret =initEnrollRenderView();
	//bret = initIdentifyRenderView();
#endif

	TCHAR* pszConfPath = theApp.GetAppConfPath();
	if (_taccess(pszConfPath, 0) == 0)
	{
		m_nPartialWidth = GetPrivateProfileInt(_T("COMMON"), _T("PARTIAL_WIDTH"), PARTIAL_WINDOW_WIDTH, pszConfPath);
		m_nPartialHeight = GetPrivateProfileInt(_T("COMMON"), _T("PARTIAL_HEIGHT"), PARTIAL_WINDOW_HEIGHT, pszConfPath);

		partialWindow_ = (char*)calloc(sizeof(char), m_nPartialWidth*m_nPartialHeight);
	}
	initDllEngine(m_nPartialWidth, m_nPartialHeight);
	saveEyeSelectFrames(true, "c:\\jtwoc\\enrollImage");

	CString tabOne = _T("Configuration");
	CString tabTwo = _T("Enrollment");
	CString tabThree = _T("Identify");
	CString tabFour = _T("Pixel");

	m_tabCtrl.InsertItem(1, tabOne);
	m_tabCtrl.InsertItem(2, tabTwo);
	m_tabCtrl.InsertItem(3, tabThree);
	m_tabCtrl.InsertItem(4, tabFour);
	
	CRect rect;
	m_tabCtrl.GetClientRect(&rect);

	m_configDialog.Create(IDD_DIALOG_CONFIGURATION, &m_tabCtrl);
	m_configDialog.SetWindowPos(NULL, 1, 22, rect.Width()-1, rect.Height()-22, SWP_SHOWWINDOW | SWP_NOZORDER);
	m_configDialog.InitControl();
	m_pWndShow = &m_configDialog;
	
	m_enrollDialog.Create(IDD_DIALOG_ENROLLMENT, &m_tabCtrl);
	m_enrollDialog.SetWindowPos(NULL, 1, 22, rect.Width()-1, rect.Height()-22, SWP_NOZORDER);

	m_identifyDialog.Create(IDD_DIALOG_IDENTIFY, &m_tabCtrl);
	m_identifyDialog.SetWindowPos(NULL, 1, 22, rect.Width()-1, rect.Height()-22, SWP_NOZORDER);

	m_pixelDialog.Create(IDD_DIALOG_ANALYZE, &m_tabCtrl);
	m_pixelDialog.SetWindowPos(NULL, 1, 22, rect.Width()-1, rect.Height()-22, SWP_NOZORDER);

	UpdateButtonState();
	SetFocus();

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void Cui0Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR Cui0Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

bool Cui0Dlg::initD3D9()
{
	bool	bSuccess = true;
	try
	{
		m_pDirect3D9 = Direct3DCreate9(D3D_SDK_VERSION);
		if (m_pDirect3D9 == NULL)
		{
			//std::cout << "Direct3D9 init failed" << std::endl;
			FUNCTION_NAME_IS(_T("Cui0Dlg::initD3D9"));
			LOG_PUTS(0, _T("Failed to init Direct3D9."));
			bSuccess = false;
		}
	}
	catch (...) 
	{
		return false;
	}
	return bSuccess;
}


bool Cui0Dlg::initRealTimerRenderView()
{
	HWND hwnd = GetDlgItem(IDC_REALTIME_RENDER_VIEW)->m_hWnd;
	bool	bSuccess = false;
	HRESULT hrSuccess;

	D3DPRESENT_PARAMETERS d3dpp;
	D3DFORMAT Format;
	__try
	{
		cleanup();
		m_pDirect3D9 = Direct3DCreate9(D3D_SDK_VERSION);
		if (m_pDirect3D9 == 0)
			__leave;

		ZeroMemory(&d3dpp, sizeof(d3dpp));
		d3dpp.Windowed = TRUE;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

		::GetClientRect(hwnd, &m_RealTimeRenderViewport);
		hrSuccess = m_pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &m_pDirect3DDevice);
		if (FAILED(hrSuccess))
		{
			__leave;
		}

		Format = (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2');	// J2C
		hrSuccess = m_pDirect3DDevice->CreateOffscreenPlainSurface(WIDTH_EYE_VIEW, HEIGHT_EYE_VIEW, Format, D3DPOOL_DEFAULT, &m_pDirect3DSurfaceRender, NULL);
		if (FAILED(hrSuccess))
		{
			__leave;
		}

		bSuccess = true;
	}
	__finally
	{
		if (false == bSuccess)
		{
			cleanup();
		}
	}

	return bSuccess;
}

bool Cui0Dlg::initEnrollRenderView()
{
	HWND hwnd = GetDlgItem(IDC_ENROLL_RENDER_VIEW)->m_hWnd;
	bool	bSuccess = false;
	HRESULT hrSuccess;

	D3DPRESENT_PARAMETERS d3dpp;
	D3DFORMAT Format;
	__try
	{

		ZeroMemory(&d3dpp, sizeof(d3dpp));
		d3dpp.Windowed = TRUE;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

		::GetClientRect(hwnd, &m_EnrollRenderViewport);
		hrSuccess = m_pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &m_pDirect3DDeviceForEnroll);
		if (FAILED(hrSuccess))
		{
			__leave;
		}

		Format = (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2');	// J2C
		hrSuccess = m_pDirect3DDevice->CreateOffscreenPlainSurface(WIDTH_ENROLL_EYE_VIEW, HEIGHT_ENROLL_EYE_VIEW, Format, D3DPOOL_DEFAULT, &m_pDirect3DSurfaceRenderForEnroll, NULL);
		if (FAILED(hrSuccess))
		{
			__leave;
		}

		bSuccess = true;
	}
	__finally
	{
		if (false == bSuccess)
		{
			cleanup();
		}
	}

	return bSuccess;
}

bool Cui0Dlg::initIdentifyRenderView()
{
	HWND hwnd = GetDlgItem(IDC_IDENTIFY_RENDER_VIEW)->m_hWnd;
	bool	bSuccess = false;
	HRESULT hrSuccess;

	D3DPRESENT_PARAMETERS d3dpp;
	D3DFORMAT Format;
	__try
	{

		ZeroMemory(&d3dpp, sizeof(d3dpp));
		d3dpp.Windowed = TRUE;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

		::GetClientRect(hwnd, &m_IdentifyRenderViewport);
		hrSuccess = m_pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &m_pDirect3DDeviceForIdentify);
		if (FAILED(hrSuccess))
		{
			__leave;
		}

		Format = (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2');	// J2C
		hrSuccess = m_pDirect3DDevice->CreateOffscreenPlainSurface(WIDTH_IDENTIFY_EYE_VIEW, HEIGHT_IDENTIFY_EYE_VIEW, Format, D3DPOOL_DEFAULT, &m_pDirect3DSurfaceRenderForIdentify, NULL);
		if (FAILED(hrSuccess))
		{
			__leave;
		}

		bSuccess = true;
	}
	__finally
	{
		if (false == bSuccess)
		{
			cleanup();
		}
	}

	return bSuccess;
}

//#define BPP 12
void	Cui0Dlg::realTimeRender(unsigned char *pBuffer)
{
	HRESULT lRet;

	if (m_pDirect3DSurfaceRender == NULL)
		return;

	D3DLOCKED_RECT d3d_rect;
	lRet = m_pDirect3DSurfaceRender->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
	if (FAILED(lRet))
		return;

	byte *pSrc = pBuffer;
	byte * pDest = (BYTE *)d3d_rect.pBits;
	int stride = d3d_rect.Pitch;
	unsigned long nIndex = 0;

	memset(pDest, 0x80, (stride)*HEIGHT_EYE_VIEW*BPP / 8);
	for (nIndex = 0; nIndex < HEIGHT_EYE_VIEW; nIndex++) {
		memcpy(pDest + nIndex * stride, pSrc + nIndex * WIDTH_EYE_VIEW, WIDTH_EYE_VIEW);
	}

	lRet = m_pDirect3DSurfaceRender->UnlockRect();
	if (FAILED(lRet))
		return;

	if (m_pDirect3DDevice == NULL)
		return;

	m_pDirect3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	m_pDirect3DDevice->BeginScene();
	IDirect3DSurface9 * pBackBuffer = NULL;

	m_pDirect3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	m_pDirect3DDevice->StretchRect(m_pDirect3DSurfaceRender, NULL, pBackBuffer, &m_RealTimeRenderViewport, D3DTEXF_LINEAR);
	m_pDirect3DDevice->EndScene();
	m_pDirect3DDevice->Present(NULL, NULL, NULL, NULL);
}
void	Cui0Dlg::enrollRender(unsigned char *pBuffer)
{
	HRESULT lRet;

	if (m_pDirect3DSurfaceRenderForEnroll == NULL)
		return;

	D3DLOCKED_RECT d3d_rect;
	lRet = m_pDirect3DSurfaceRenderForEnroll->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
	if (FAILED(lRet))
		return;

	byte *pSrc = pBuffer;
	byte * pDest = (BYTE *)d3d_rect.pBits;
	int stride = d3d_rect.Pitch;
	unsigned long nIndex = 0;

	memset(pDest, 0x80, (stride)*HEIGHT_ENROLL_EYE_VIEW*BPP / 8);
	for (nIndex = 0; nIndex < HEIGHT_ENROLL_EYE_VIEW; nIndex++) {
		memcpy(pDest + nIndex * stride, pSrc + nIndex * WIDTH_ENROLL_EYE_VIEW, WIDTH_ENROLL_EYE_VIEW);
	}

	lRet = m_pDirect3DSurfaceRenderForEnroll->UnlockRect();
	if (FAILED(lRet))
		return;

	if (m_pDirect3DDeviceForEnroll == NULL)
		return;

	m_pDirect3DDeviceForEnroll->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	m_pDirect3DDeviceForEnroll->BeginScene();
	IDirect3DSurface9 * pBackBuffer = NULL;

	m_pDirect3DDeviceForEnroll->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	m_pDirect3DDeviceForEnroll->StretchRect(m_pDirect3DSurfaceRenderForEnroll, NULL, pBackBuffer, &m_EnrollRenderViewport, D3DTEXF_LINEAR);
	m_pDirect3DDeviceForEnroll->EndScene();
	m_pDirect3DDeviceForEnroll->Present(NULL, NULL, NULL, NULL);
}
void	Cui0Dlg::identifyRender(unsigned char *pBuffer)
{
	HRESULT lRet;

	if (m_pDirect3DSurfaceRenderForIdentify == NULL)
		return;

	D3DLOCKED_RECT d3d_rect;
	lRet = m_pDirect3DSurfaceRenderForIdentify->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
	if (FAILED(lRet))
		return;

	byte *pSrc = pBuffer;
	byte * pDest = (BYTE *)d3d_rect.pBits;
	int stride = d3d_rect.Pitch;
	unsigned long nIndex = 0;

	memset(pDest, 0x80, (stride)*HEIGHT_IDENTIFY_EYE_VIEW*BPP / 8);
	for (nIndex = 0; nIndex < HEIGHT_IDENTIFY_EYE_VIEW; nIndex++) {
		memcpy(pDest + nIndex * stride, pSrc + nIndex * WIDTH_IDENTIFY_EYE_VIEW, WIDTH_IDENTIFY_EYE_VIEW);
	}

	lRet = m_pDirect3DSurfaceRenderForIdentify->UnlockRect();
	if (FAILED(lRet))
		return;

	if (m_pDirect3DDeviceForIdentify == NULL)
		return;

	m_pDirect3DDeviceForIdentify->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	m_pDirect3DDeviceForIdentify->BeginScene();
	IDirect3DSurface9 * pBackBuffer = NULL;

	m_pDirect3DDeviceForIdentify->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	m_pDirect3DDeviceForIdentify->StretchRect(m_pDirect3DSurfaceRenderForIdentify, NULL, pBackBuffer, &m_IdentifyRenderViewport, D3DTEXF_LINEAR);
	m_pDirect3DDeviceForIdentify->EndScene();
	m_pDirect3DDeviceForIdentify->Present(NULL, NULL, NULL, NULL);
}

void Cui0Dlg::initDllEngine(int nPartialWidth, int nPartialHeight)
{
	initJ2CEngine(nPartialWidth, nPartialHeight);
	registerRealTimeRenderCb(::realTimeRender);
	registerEnrollRenderCb(::enrollRender);
	registerIdentifyRenderCb(::identifyRender);
	
}

void Cui0Dlg::cleanup()
{
	if(m_pDirect3DSurfaceRender)
		m_pDirect3DSurfaceRender->Release();
	if (m_pDirect3DDevice)
		m_pDirect3DDevice->Release();
	if (m_pDirect3D9)
		m_pDirect3D9->Release();

	m_pDirect3D9 = NULL;
	m_pDirect3DDevice = NULL;
	m_pDirect3DSurfaceRender = NULL;
}

void Cui0Dlg::cleanupForPixel()
{
	if (m_pDirect3DSurfaceRenderForPixel)
		m_pDirect3DSurfaceRenderForPixel->Release();
	if (m_pDirect3DDeviceForPixel)
		m_pDirect3DDeviceForPixel->Release();
	m_pDirect3DDeviceForPixel = NULL;
	m_pDirect3DSurfaceRenderForPixel = NULL;

	if (m_pDirect3DSurfaceRenderForPixel2)
		m_pDirect3DSurfaceRenderForPixel2->Release();
	if (m_pDirect3DDeviceForPixel2)
		m_pDirect3DDeviceForPixel2->Release();
	m_pDirect3DDeviceForPixel2 = NULL;
	m_pDirect3DSurfaceRenderForPixel2 = NULL;
}

void Cui0Dlg::OnBnClickedButtonStart()
{
	if (m_bIrisStart)
	{
		//
	}
	else
	{
		J2CStart();
		m_bIrisStart = true;
		UpdateButtonState();
		m_configDialog.UpdateViewMode(CConfigDialog::eVIEW_REAL);
	}
}

void Cui0Dlg::OnBnClickedButtonStop()
{
	if (m_bIrisStart)
	{
		bool bIrisStop = J2CStop();
		m_bIrisStart = false;
		UpdateButtonState();

		m_configDialog.UpdateViewMode(CConfigDialog::eVIEW_IMAGE);
	}
	else
	{
		//
	}
}

void Cui0Dlg::UpdateButtonState()
{
	bool bIrisDevOpen = J2CCheckDevOpen();
	bool bBtnStop = bIrisDevOpen;
	bool bBtnStart = bIrisDevOpen^1;
	GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(bBtnStop);
	GetDlgItem(IDC_BUTTON_START)->EnableWindow(bBtnStart);
}

void Cui0Dlg::OnBnClickedButtonIdentify()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	identifyRequest(::identifyCb);
}


void Cui0Dlg::OnBnClickedButtonEnroll()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString str;
	GetDlgItemText(IDC_EDIT_ENROLL, str);
	char buffer[1024];
	sprintf(buffer, "%S", str);
	std::cout << "### enroll id " << buffer << "###" << std::endl;
	enrollRequest(buffer, ::enrollCb);
}


void Cui0Dlg::OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	*pResult = 0;
	if (m_pWndShow != NULL)
	{
		m_pWndShow->ShowWindow(SW_HIDE);
		m_pWndShow = NULL;
	}

	int nIndex = m_tabCtrl.GetCurSel();
	switch (nIndex)
	{
	case 0:
		m_configDialog.ShowWindow(SW_SHOW);
		m_pWndShow = &m_configDialog;
		break;
	case 1:
		m_enrollDialog.ShowWindow(SW_SHOW);
		m_pWndShow = &m_enrollDialog;
		break;
	case 2:
		m_identifyDialog.ShowWindow(SW_SHOW);
		m_pWndShow = &m_identifyDialog;
		break;
	case 3:
		m_pixelDialog.ShowWindow(SW_SHOW);
		m_pWndShow = &m_pixelDialog;
	}
}


void Cui0Dlg::OnBnClickedOk()
{
	J2CStop();

	bool bIrisDevOpen = J2CCheckDevOpen();
	if (bIrisDevOpen)
	{
		AfxMessageBox(_T("Failed to stop IRIS device."));
	}

	CDialogEx::OnOK();
	//J2CStop();
}

BOOL Cui0Dlg::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message)
	{
	case WM_KEYDOWN:
		{
			switch (pMsg->wParam)
			{
			case 'C':
			case 'c':
				{
					if (m_pWndShow == &m_configDialog)
					{
						if (GetAsyncKeyState(VK_CONTROL) < 0 && GetAsyncKeyState(VK_SHIFT) < 0)
						{
							m_configDialog.CallPartailRender();
						}
					}
				}
				break;
			}
		}
		break;
	default:
		break;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void Cui0Dlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	CRect rtDlg;
	GetClientRect(&rtDlg);

	ViewWidth_ = rtDlg.Width() + 500;
	ViewHeight_ = rtDlg.Height() + 500;

	int HScrollMax = 0;
	HPageSize_ = 0;

	if (cx < ViewWidth_)
	{
		HScrollMax = ViewWidth_ - 1;
		HPageSize_ = cx;
		HScrollPos_ = min(HScrollPos_, ViewWidth_ - HPageSize_ - 1);
	}

	SCROLLINFO si;
	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
	si.nMin = 0;
	si.nMax = HScrollMax;
	si.nPos = HScrollPos_;
	si.nPage = HPageSize_;
	SetScrollInfo(SB_HORZ, &si, TRUE);

	int VScrollMax = 0;
	VPageSize_ = 0;

	if (cy < ViewHeight_)
	{
		VScrollMax = ViewHeight_ - 1;
		VPageSize_ = cy;
		VScrollPos_ = min(VScrollPos_, ViewHeight_ - VPageSize_ - 1);
	}

	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	si.nMin = 0;
	si.nMax = VScrollMax;
	si.nPos = VScrollPos_;
	si.nPage = VPageSize_;
	SetScrollInfo(SB_VERT, &si, TRUE);
}

void Cui0Dlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	int delta;

	switch (nSBCode)
	{
	case SB_LINEUP:
		delta = -m_nBasic;
		break;
	case SB_PAGEUP:
		delta = -VPageSize_;
		break;
	case SB_THUMBTRACK:
		delta = static_cast<int>(nPos) - VScrollPos_;
		break;
	case SB_PAGEDOWN:
		delta = VPageSize_;
		break;
	case SB_LINEDOWN:
		delta = m_nBasic;
		break;
	default:
		return;
	}

	int scrollpos = VScrollPos_ + delta;
	int maxpos = ViewHeight_ - VPageSize_;
	if (scrollpos < 0)
		delta = -VScrollPos_;
	else
		if (scrollpos > maxpos)
			delta = maxpos - VScrollPos_;

	if (delta != 0)
	{
		VScrollPos_ += delta;
		SetScrollPos(SB_VERT, VScrollPos_, TRUE);
		ScrollWindow(0, -delta);
	}
}



void Cui0Dlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	m_nHdelta = 0;

	switch (nSBCode)
	{
	case SB_LINELEFT:
		m_nHdelta = -m_nBasic;
		break;
	case SB_PAGELEFT:
		m_nHdelta = -HPageSize_;
		break;
	case SB_THUMBTRACK:
		m_nHdelta = static_cast<int>(nPos) - HScrollPos_;
		break;
	case SB_PAGERIGHT:
		m_nHdelta = HPageSize_;
		break;
	case SB_LINERIGHT:
		m_nHdelta = m_nBasic;
		break;
	default:
		return;
	}

	int scrollpos = HScrollPos_ + m_nHdelta;
	int maxpos = ViewWidth_ - HPageSize_;

	if (scrollpos < 0)
		m_nHdelta = -HScrollPos_;
	else
		if (scrollpos > maxpos)
			m_nHdelta = maxpos - HScrollPos_;

	if (m_nHdelta != 0)
	{
		HScrollPos_ += m_nHdelta;
		SetScrollPos(SB_HORZ, HScrollPos_, TRUE);
		ScrollWindow(-m_nHdelta, 0);
	}
}


void Cui0Dlg::OnBnClickedButtonTest()
{
	J2CTest();
}
