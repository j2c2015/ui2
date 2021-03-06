// CConfigDialog.cpp: 구현 파일
//

#include "stdafx.h"
#include "ui0.h"
#include "CConfigDialog.h"
#include "afxdialogex.h"
#include "ui0Dlg.h"
#include <iostream>
#include "J2CEngine_dll.h"
#include <io.h>

CConfigDialog *pConfigDlg=0;
// CConfigDialog 대화 상자

extern "C" void partialRenderCb(void *buffer, int width, int height, int depth)
{
	pDlg->setPartialWindow((char *) buffer);

	// cscho (2018-12.11)
	//pDlg->m_pixelDialog.render((unsigned char*)buffer);
}

IMPLEMENT_DYNAMIC(CConfigDialog, CDialogEx)

CConfigDialog::CConfigDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CONFIGURATION, pParent)
{
	m_nSrcWidth = 800;
	m_nSrcHeight = 600;
	m_eViewMode = eVIEW_READY;

	m_pBmpFixCrop = NULL;
	m_bPartialShowValue = true;

	m_bSortAscendingCrop = false;
	m_bSortAscendingEnroll = false;
	m_bSortAscendingResize = false;
	_tcscpy(m_szCropFolder, _T("C:\\jtwoc\\crop"));
}

CConfigDialog::~CConfigDialog()
{
	if (m_pBmpFixCrop)
	{
		delete m_pBmpFixCrop;
		m_pBmpFixCrop = NULL;
	}
	
}

void CConfigDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	
}

CRect CConfigDialog::GetPartialRect()
{
	CRect rtPartial(0, 0, 0, 0);
	if (m_wndTest.GetSafeHwnd())
	{
		rtPartial = m_wndTest.GetPartialRect();
	}
	return rtPartial;
}

void CConfigDialog::UpdatePartialValue(CString strValue)
{
	if (m_editPartialValue.GetSafeHwnd())
	{
		m_editPartialValue.SetWindowText(strValue);
	}
}


BEGIN_MESSAGE_MAP(CConfigDialog, CDialogEx)
	ON_WM_LBUTTONDOWN()
	ON_BN_CLICKED(ID_SHOWVALUE_CHECK, OnBnClickedButtonShowValue)
	ON_BN_CLICKED(ID_VIEWMODE_BTN, OnBnClickedButtonViewMode)
	ON_BN_CLICKED(ID_PARTIAL_BTN, OnBnClickedButtonPartial)
	ON_BN_CLICKED(ID_REALSAVE_BTN, OnBnClickedButtonRealSave)
	ON_BN_CLICKED(ID_REALSAVE_BROWSE_BTN, OnBnClickedButtonRealSaveBrowse)
	ON_BN_CLICKED(ID_CROP_LOAD_BTN, OnBnClickedButtonCropLoad)
	ON_BN_CLICKED(ID_CROP_CLEAR_BTN, OnBnClickedButtonCropClear)
	ON_BN_CLICKED(ID_ENROLL_LOAD_BTN, OnBnClickedButtonPartialLoad)
	ON_BN_CLICKED(ID_ENROLL_CLEAR_BTN, OnBnClickedButtonPartialClear)
	ON_BN_CLICKED(ID_RESIZE_LOAD_BTN, OnBnClickedButtonResizeLoad)
	ON_BN_CLICKED(ID_RESIZE_CLEAR_BTN, OnBnClickedButtonResizeClear)
	ON_NOTIFY(NM_DBLCLK, ID_CROP_LIST, OnNMDblclkListCrop)
	ON_NOTIFY(NM_DBLCLK, ID_ENROLL_LIST, OnNMDblclkListEnroll)
	ON_NOTIFY(NM_DBLCLK, ID_RESIZE_LIST, OnNMDblclkListResize)
	ON_NOTIFY(HDN_ITEMCLICK, 0, OnHdnItemclickCropList)
	ON_MESSAGE(UM_FILE_EYEFIND_TEST, OnUmFileEyeFindTest)
END_MESSAGE_MAP()

void CConfigDialog::render(unsigned char *pBuffer)
{
	if (!m_wndTest.GetSafeHwnd())
		return;
	if (m_eViewMode != eVIEW_REAL)
		return;

	HRESULT lRet = E_FAIL;

	if (pDlg->m_pDirect3DSurfaceRender == NULL)
		return;
	
	D3DLOCKED_RECT d3d_rect;
	lRet = pDlg->m_pDirect3DSurfaceRender->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
	if (FAILED(lRet))
		return;

	byte *pSrc = pBuffer;
	byte * pDest = (BYTE *)d3d_rect.pBits;
	int stride = d3d_rect.Pitch;
	unsigned long nIndex = 0;

	memset(pDest, 0x80, (stride)*HEIGHT_EYE_VIEW*BPP / 8);
	for (nIndex = 0; nIndex < HEIGHT_EYE_VIEW; nIndex++)
	{
		memcpy(pDest + nIndex * stride, pSrc + nIndex * WIDTH_EYE_VIEW, WIDTH_EYE_VIEW);
	}

	lRet = pDlg->m_pDirect3DSurfaceRender->UnlockRect();
	if (FAILED(lRet))
		return;

	if (pDlg->m_pDirect3DDevice == NULL)
		return;

	pDlg->m_pDirect3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	pDlg->m_pDirect3DDevice->BeginScene();
	IDirect3DSurface9 * pBackBuffer = NULL;

	pDlg->m_pDirect3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	pDlg->m_pDirect3DDevice->StretchRect(pDlg->m_pDirect3DSurfaceRender, NULL, pBackBuffer, &pDlg->m_RealTimeRenderViewport, D3DTEXF_LINEAR);
	
	//DrawRectangle(pDlg->m_pDirect3DDevice);

	pDlg->m_pDirect3DDevice->EndScene();
	pDlg->m_pDirect3DDevice->Present(NULL, NULL, NULL, NULL);

	m_wndTest.Invalidate();

	// partial view
	CRect rtPartial = m_wndTest.GetPartialRect();
	int nPartialX = rtPartial.left, nPartialY = rtPartial.top;
	char* start = (char *)pBuffer;
	char* dst = pDlg->getPartialWindow();
	start += (800 * nPartialY);
	start += nPartialX;
	char* pszPartial = dst;

	int nPartialWidth = pDlg->m_nPartialWidth;
	int nPartialHeight = pDlg->m_nPartialHeight;
	for (int i = 0; i < nPartialHeight; i++)
	{
		memcpy(dst, start, nPartialWidth);
		dst += nPartialWidth;
		start += 800;
	}
	renderPartial((unsigned char *)pszPartial);
}

void CConfigDialog::DrawRectangle(IDirect3DDevice9* pDirect3DDeviceForPixel)
{
	pDirect3DDeviceForPixel->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

	pDirect3DDeviceForPixel->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	pDirect3DDeviceForPixel->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	pDirect3DDeviceForPixel->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

	int x = 200;
	int y = 200;
	int w = 200;
	int h = 200;
	D3DCOLOR color1 = D3DCOLOR_ARGB(64, 255, 255, 0);
	D3DCOLOR color2 = D3DCOLOR_ARGB(64, 255, 255, 0);
	D3DCOLOR color3 = D3DCOLOR_ARGB(64, 255, 255, 0);
	D3DCOLOR color4 = D3DCOLOR_ARGB(64, 255, 255, 0);

	struct
	{
		float x, y, z, w;
		D3DCOLOR color;
	} vert[4] = {
		x - 0.5f, y - 0.5f, 1.f, 1.f, color1,
		x + w - 0.5f, y - 0.5f, 1.f, 1.f, color2,
		x - 0.5f, y + h - 0.5f, 1.f, 1.f, color3,
		x + w - 0.5f, y + h - 0.5f, 1.f, 1.f, color4
	};
	HRESULT hr = pDirect3DDeviceForPixel->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vert, 20);
}

void CConfigDialog::renderPartial(unsigned char *pBuffer)
{
	if (!m_wndPartialView.GetSafeHwnd())
		return;
	if (m_eViewMode != eVIEW_REAL)
		return;

	HRESULT lRet = E_FAIL;

	if (pDlg->m_pDirect3DSurfaceRenderRealPartial == NULL)
		return;

	D3DLOCKED_RECT d3d_rect;
	lRet = pDlg->m_pDirect3DSurfaceRenderRealPartial->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
	if (FAILED(lRet))
		return;

	byte *pSrc = pBuffer;
	byte * pDest = (BYTE *)d3d_rect.pBits;
	int stride = d3d_rect.Pitch;
	unsigned long nIndex = 0;
	
	size_t size = (stride)*pDlg->m_nPartialHeight*BPP / 8;
	memset(pDest, 0x80, size);
	for (nIndex = 0; nIndex < pDlg->m_nPartialHeight; nIndex++)
	{
		memcpy(pDest + nIndex * stride, pSrc + nIndex * pDlg->m_nPartialWidth, pDlg->m_nPartialWidth);
	}

	lRet = pDlg->m_pDirect3DSurfaceRenderRealPartial->UnlockRect();
	if (FAILED(lRet))
		return;

	if (pDlg->m_pDirect3DDevice == NULL)
		return;
	
	pDlg->m_pDirect3DDeviceRealPartial->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	pDlg->m_pDirect3DDeviceRealPartial->BeginScene();
	IDirect3DSurface9 * pBackBuffer = NULL;

	pDlg->m_pDirect3DDeviceRealPartial->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	
	//pDlg->m_pDirect3DDeviceRealPartial->StretchRect(pDlg->m_pDirect3DSurfaceRenderRealPartial, NULL, pBackBuffer, &pDlg->m_RealTimeRenderViewportRealPartial, D3DTEXF_POINT);
	DrawPartial(pDlg->m_pDirect3DDeviceRealPartial, pBuffer);

	pDlg->m_pDirect3DDeviceRealPartial->EndScene();
	pDlg->m_pDirect3DDeviceRealPartial->Present(NULL, NULL, NULL, NULL);

	//m_wndCropView.Invalidate();
}

void CConfigDialog::DrawPartial(IDirect3DDevice9* pDirect3DDeviceForPixel, unsigned char* pBuffer)
{
	pDirect3DDeviceForPixel->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	
	int x = 0;
	int y = 0;
	int nPartialWidth = theMainDlg->GetPartialWidth();
	int nPartialHeight = theMainDlg->GetPartialHeight();
	int nWndWidth = pDlg->m_RealTimeRenderViewportRealPartial.right - pDlg->m_RealTimeRenderViewportRealPartial.left;
	int nWndHeight = pDlg->m_RealTimeRenderViewportRealPartial.bottom - pDlg->m_RealTimeRenderViewportRealPartial.top;
	int w = nWndWidth / nPartialWidth;
	int h = nWndHeight / nPartialHeight;
	for (int nH = 0; nH < nPartialHeight; nH++)
	{
		for (int nW = 0; nW < nPartialWidth; nW++)
		{
			int nIndex = nW + (nH * nPartialWidth);
			byte color = (byte)pBuffer[nIndex];
			D3DCOLOR color1 = D3DCOLOR_XRGB(color, color, color);
			D3DCOLOR color2 = D3DCOLOR_XRGB(color, color, color);
			D3DCOLOR color3 = D3DCOLOR_XRGB(color, color, color);
			D3DCOLOR color4 = D3DCOLOR_XRGB(color, color, color);

			struct Vertex
			{
				float x, y, z, w;
				D3DCOLOR color;
			};
			Vertex vert[4] = 
			{
				x - 0.5f, y - 0.5f, 1.f, 1.f, color1,
				x + w - 0.5f, y - 0.5f, 1.f, 1.f, color2,
				x - 0.5f, y + h - 0.5f, 1.f, 1.f, color3,
				x + w - 0.5f, y + h - 0.5f, 1.f, 1.f, color4
			};
			HRESULT hr = pDirect3DDeviceForPixel->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vert, sizeof(Vertex));

			x += w;
		}
		y += h;
		x = 0;
	}
}

bool CConfigDialog::initView(CWnd* pWnd, RECT& rt, IDirect3DDevice9*& pDirect3DDevice, IDirect3DSurface9*& pDirect3DSurfaceRender, int nSurfaceWidth, int nSurfaceHeight)
{
	HWND hwnd = pWnd->GetSafeHwnd();

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

		::GetClientRect(hwnd, &rt);
		hrSuccess = pDlg->m_pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDirect3DDevice);
		if (FAILED(hrSuccess))
		{
			FUNCTION_NAME_IS(_T("CConfigDialog::initView"));
			LOG_PUTS(0, _T("Failed to create D3D device."));
			__leave;
		}

		Format = (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2');	// J2C
		hrSuccess = pDirect3DDevice->CreateOffscreenPlainSurface(nSurfaceWidth, nSurfaceHeight, Format, D3DPOOL_DEFAULT, &pDirect3DSurfaceRender, NULL);
		if (FAILED(hrSuccess))
		{
			FUNCTION_NAME_IS(_T("CConfigDialog::initView"));
			LOG_PRINTF(0, _T("Failed to create D3D PlainSurface. (Width=%d, Height=%d)"), WIDTH_EYE_VIEW, HEIGHT_EYE_VIEW);
			__leave;
		}

		bSuccess = true;
	}
	__finally
	{
		if (false == bSuccess)
		{
			pDlg->cleanup();
		}
	}

	return bSuccess;
}
// CConfigDialog 메시지 처리기
void configRender(void *buffer,int width ,int height, int depth)
{
	pConfigDlg->render((unsigned char *)buffer);
}

BOOL CConfigDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	pConfigDlg = this;
	
	/*
	BOOL bRet = FALSE;
	BOOL bRetCtrl = InitControl();
	if (bRetCtrl)
	{
		bool bRet = initView();
		if (bRet == false)
		{
			std::cout << "****** init config view failed ******" << std::endl;
		}

		setRealTimeRenderer(::configRender);
		bRet = TRUE;
	}
	*/
	{
		Graphics graph(GetDC()->m_hDC);
		Gdiplus::Font font(_T("System"), 9, FontStyleRegular);
		LOGFONTW lf;
		font.GetLogFontW(&graph, &lf);
		m_fontControl.CreateFontIndirect(&lf);
	}

	return TRUE;
}

BOOL CConfigDialog::InitControl()
{
	CString strTestClassName = _T("");
	CString strTestWndName = _T("");

	CRect rtDlg;
	GetWindowRect(&rtDlg);
	int nDlgWidth = rtDlg.Width();
	int nDlgHeight = rtDlg.Height();

	// view mode button
	int nBtnX = 0;
	int nBtnY = 2;
	int nBtnWidth = 100;
	int nBtnHeight = 22;
	DWORD dwBtnStyle = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
	CRect rtViewModeBtn(nBtnX, nBtnY, nBtnX + nBtnWidth, nBtnY + nBtnHeight);
	if (m_btnViewMode.Create(_T("View [Ready]"), dwBtnStyle, rtViewModeBtn, this, ID_VIEWMODE_BTN) == FALSE)
	{
		FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
		LOG_PUTS(0, _T("Failed to create view-mode button window."));
		return FALSE;
	}
	m_btnViewMode.SetFont(&m_fontControl);
	m_btnViewMode.GetWindowRect(&rtViewModeBtn);
	ScreenToClient(&rtViewModeBtn);

	// partial button
	nBtnX = rtViewModeBtn.right + 5;
	dwBtnStyle = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
	CRect rtPartialBtn(nBtnX, nBtnY, nBtnX + nBtnWidth, nBtnY + nBtnHeight);
	if (m_btnPartial.Create(_T("Partial"), dwBtnStyle, rtPartialBtn, this, ID_PARTIAL_BTN) == FALSE)
	{
		FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
		LOG_PUTS(0, _T("Failed to create partial button window."));
		return FALSE;
	}
	m_btnPartial.SetFont(&m_fontControl);
	m_btnPartial.GetWindowRect(&rtPartialBtn);
	ScreenToClient(&rtPartialBtn);

	// real save button
	nBtnX = rtPartialBtn.right + 5;
	dwBtnStyle = WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX;
	CRect rtRealSaveBtn(nBtnX, nBtnY, nBtnX + nBtnWidth, nBtnY + nBtnHeight);
	if (m_chkRealSave.Create(_T("Real-Save"), dwBtnStyle, rtRealSaveBtn, this, ID_REALSAVE_BTN) == FALSE)
	{
		FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
		LOG_PUTS(0, _T("Failed to create real save button window."));
		return FALSE;
	}
	m_chkRealSave.SetFont(&m_fontControl);
	m_chkRealSave.SetCheck(false);
	m_chkRealSave.GetWindowRect(&rtRealSaveBtn);
	ScreenToClient(&rtRealSaveBtn);
		
	// real view wnd
	int nRealViewHeight = 512;
	int nRealViewWidth = (nRealViewHeight*m_nSrcWidth) / m_nSrcHeight;

	strTestClassName.Format(_T("RealView_%X%X%X"),
		GetTickCount(),
		GetCurrentProcessId(),
		GetCurrentThreadId());
	if (RegisterWindowClass(AfxGetInstanceHandle(), strTestClassName))
	{
		int nSX = 0;
		int nSY = rtPartialBtn.bottom + 3;
		int nSW = nRealViewWidth;
		int nSH = nRealViewHeight;

		strTestWndName = _T("RealViewWnd");

		CRect rt(nSX, nSY, nSX + nSW, nSY + nSH);
		int nID = ID_TEST_WND;
		if (m_wndTest.Create(strTestClassName, strTestWndName, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, rt, this, nID) == FALSE)
		{
			FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
			LOG_PUTS(0, _T("Failed to create real-view window."));
			return FALSE;
		}
	}	
	CRect rtRealView;
	m_wndTest.GetWindowRect(&rtRealView);
	ScreenToClient(&rtRealView);

	// real save browse button
	nBtnX = rtRealView.right - 40;
	nBtnY = rtPartialBtn.top;
	nBtnWidth = 40;
	dwBtnStyle = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
	CRect rtRealSaveBrowseBtn(nBtnX, nBtnY, nBtnX + nBtnWidth, nBtnY + nBtnHeight);
	if (m_btnRealSaveBrowse.Create(_T("..."), dwBtnStyle, rtRealSaveBrowseBtn, this, ID_REALSAVE_BROWSE_BTN) == FALSE)
	{
		FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
		LOG_PUTS(0, _T("Failed to create real save button window."));
		return FALSE;
	}
	m_btnRealSaveBrowse.SetFont(&m_fontControl);
	m_btnRealSaveBrowse.GetWindowRect(&rtRealSaveBrowseBtn);
	ScreenToClient(&rtRealSaveBrowseBtn);

	// real save edit
	nBtnX = rtRealSaveBtn.right + 3;
	nBtnY = 2;
	nBtnWidth = (rtRealSaveBrowseBtn.left - nBtnX) - 5;
	nBtnHeight = 22;
	dwBtnStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT;
	CRect rtRealSaveEdit(nBtnX, nBtnY, nBtnX + nBtnWidth, nBtnY + nBtnHeight);
	if (m_editRealSave.Create(dwBtnStyle, rtRealSaveEdit, this, ID_REALSAVE_EDIT) == FALSE)
	{
		FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
		LOG_PUTS(0, _T("Failed to create real save edit window."));
		return FALSE;
	}
	m_editRealSave.SetFont(&m_fontControl);
	m_editRealSave.SetWindowText(m_szCropFolder);
	m_editRealSave.GetWindowRect(&rtRealSaveEdit);
	ScreenToClient(&rtRealSaveEdit);
		
	// partial view wnd
	int nPartialViewHeight = 512;// rtRealView.Height();
	int nPartialViewWidth = 512;// nPartialViewHeight;
	int nPartialStartX = rtRealView.right + 5;
	int nPartialEndX = nDlgWidth - 5;
	int nPartialTotal = (nPartialEndX - nPartialStartX);
	int nPartialSX = nPartialStartX + ((nPartialTotal - nPartialViewWidth) / 2);
	int nPartialEX = nPartialSX + nPartialViewWidth;
	int nPartialSY = rtRealView.top;	

	strTestClassName.Format(_T("PartialView_%X%X%X"),
		GetTickCount(),
		GetCurrentProcessId(),
		GetCurrentThreadId());
	if (RegisterWindowClass(AfxGetInstanceHandle(), strTestClassName))
	{
		int nSX = nPartialSX;
		int nSY = nPartialSY;
		int nSW = nPartialViewWidth;
		int nSH = nPartialViewHeight;

		strTestWndName = _T("PartialViewWnd");

		CRect rt(nSX, nSY, nSX + nSW, nSY + nSH);
		int nID = ID_PARTIAL_WND;
		if (m_wndPartialView.Create(strTestClassName, strTestWndName, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, rt, this, nID) == FALSE)
		{
			FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
			LOG_PUTS(0, _T("Failed to create partial-view window."));
			return FALSE;
		}
	}
	CRect rtPartialView;
	m_wndPartialView.GetWindowRect(&rtPartialView);
	ScreenToClient(&rtPartialView);

	// partial value check
	nBtnX = rtPartialView.left;
	nBtnY = 2;
	nBtnWidth = 100;
	nBtnHeight = 22;
	dwBtnStyle = WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX;
	CRect rtPartialCheckBtn(nBtnX, nBtnY, nBtnX + nBtnWidth, nBtnY + nBtnHeight);
	if (m_chkShowValue.Create(_T("Show Value"), dwBtnStyle, rtPartialCheckBtn, this, ID_SHOWVALUE_CHECK) == FALSE)
	{
		FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
		LOG_PUTS(0, _T("Failed to create show-value check window."));
		return FALSE;
	}
	m_chkShowValue.SetFont(&m_fontControl);
	m_chkShowValue.SetCheck(m_bPartialShowValue);
	m_chkShowValue.GetWindowRect(&rtPartialCheckBtn);
	ScreenToClient(&rtPartialCheckBtn);

	// partial value edit
	nBtnX = rtPartialCheckBtn.right;
	nBtnY = 2;
	nBtnWidth = (rtPartialView.right - nBtnX);
	nBtnHeight = 22;
	dwBtnStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT;
	CRect rtPartialValueEdit(nBtnX, nBtnY, nBtnX + nBtnWidth, nBtnY + nBtnHeight);
	if (m_editPartialValue.Create(dwBtnStyle, rtPartialValueEdit, this, ID_SHOWVALUE_EDIT) == FALSE)
	{
		FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
		LOG_PUTS(0, _T("Failed to create value static window."));
		return FALSE;
	}
	m_editPartialValue.SetFont(&m_fontControl);
	m_editPartialValue.SetWindowText(_T("Value: "));
	m_editPartialValue.GetWindowRect(&rtPartialValueEdit);
	ScreenToClient(&rtPartialValueEdit);

	// crop load button
	nBtnX = rtViewModeBtn.left;
	nBtnY = rtPartialView.bottom + 3;
	nBtnWidth = 100;
	nBtnHeight = 22;
	dwBtnStyle = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
	CRect rtCropBtn(nBtnX, nBtnY, nBtnX + nBtnWidth, nBtnY + nBtnHeight);
	if (m_btnLoadCrop.Create(_T("Crop [Load]"), dwBtnStyle, rtCropBtn, this, ID_CROP_LOAD_BTN) == FALSE)
	{
		FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
		LOG_PUTS(0, _T("Failed to create load button window."));
		return FALSE;
	}
	m_btnLoadCrop.SetFont(&m_fontControl);
	m_btnLoadCrop.GetWindowRect(&rtCropBtn);
	ScreenToClient(&rtCropBtn);

	// crop clear button
	nBtnX = rtCropBtn.right + 5;
	nBtnWidth = 100;
	nBtnHeight = 22;	
	CRect rtCropClearBtn(nBtnX, nBtnY, nBtnX + nBtnWidth, nBtnY + nBtnHeight);
	if (m_btnClearCrop.Create(_T("Crop [Clear]"), dwBtnStyle, rtCropClearBtn, this, ID_CROP_CLEAR_BTN) == FALSE)
	{
		FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
		LOG_PUTS(0, _T("Failed to create clear button window."));
		return FALSE;
	}
	m_btnClearCrop.SetFont(&m_fontControl);
	m_btnClearCrop.GetWindowRect(&rtCropClearBtn);
	ScreenToClient(&rtCropClearBtn);

	// crop list
	int nListX = rtRealView.left;
	int nListY = rtCropBtn.bottom + 3;
	int nListEY = nDlgHeight - 5;
	int nListWidth = DEF_LIST_WIDTH;
	int nListHeight = nListEY - nListY;
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_ALIGNLEFT | LVS_SINGLESEL;
	CRect rtCropList(nListX, nListY, nListX + nListWidth, nListY + nListHeight);
	if (m_listCrop.Create(dwStyle, rtCropList, this, ID_CROP_LIST) == FALSE)
	{
		FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
		LOG_PUTS(0, _T("Failed to create 'Crop' list window."));
		return FALSE;
	}
	else
	{
		CString strTitle = _T("Name");
		m_listCrop.InsertColumn(0, strTitle, LVCFMT_LEFT, 120);
		strTitle = _T("Size");
		m_listCrop.InsertColumn(1, strTitle, LVCFMT_RIGHT, 70);
		strTitle = _T("Date modified");
		m_listCrop.InsertColumn(2, strTitle, LVCFMT_LEFT, 170);

		m_listCrop.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
		SetWindowTheme(m_listCrop.GetSafeHwnd(), L"Explorer", NULL);
	}
	m_listCrop.GetWindowRect(&rtCropList);
	ScreenToClient(&rtCropList);

	// enroll load button
	nBtnX = rtCropList.right + 5;
	nBtnY = rtCropBtn.top;
	nBtnWidth = 100;
	nBtnHeight = 22;
	CRect rtEnrollLoadBtn(nBtnX, nBtnY, nBtnX + nBtnWidth, nBtnY + nBtnHeight);
	if (m_btnLoadEnroll.Create(_T("Enroll [Load]"), dwBtnStyle, rtEnrollLoadBtn, this, ID_ENROLL_LOAD_BTN) == FALSE)
	{
		FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
		LOG_PUTS(0, _T("Failed to create load button window."));
		return FALSE;
	}
	m_btnLoadEnroll.SetFont(&m_fontControl);
	m_btnLoadEnroll.GetWindowRect(&rtEnrollLoadBtn);
	ScreenToClient(&rtEnrollLoadBtn);

	// enroll clear button
	nBtnX = rtEnrollLoadBtn.right + 5;
	CRect rtEnrollClearBtn(nBtnX, nBtnY, nBtnX + nBtnWidth, nBtnY + nBtnHeight);
	if (m_btnClearEnroll.Create(_T("Enroll [Clear]"), dwBtnStyle, rtEnrollClearBtn, this, ID_ENROLL_CLEAR_BTN) == FALSE)
	{
		FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
		LOG_PUTS(0, _T("Failed to create clear button window."));
		return FALSE;
	}
	m_btnClearEnroll.SetFont(&m_fontControl);
	m_btnClearEnroll.GetWindowRect(&rtEnrollClearBtn);
	ScreenToClient(&rtEnrollClearBtn);

	// enroll list
	nListX = rtEnrollLoadBtn.left;
	nListY = rtCropList.top;
	CRect rtEnrollList(nListX, nListY, nListX + nListWidth, nListY + nListHeight);
	if (m_listEnroll.Create(dwStyle, rtEnrollList, this, ID_ENROLL_LIST) == FALSE)
	{
		FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
		LOG_PUTS(0, _T("Failed to create 'Enroll' list window."));
		return FALSE;
	}
	else
	{
		CString strTitle = _T("Name");
		m_listEnroll.InsertColumn(0, strTitle, LVCFMT_LEFT, 120);
		strTitle = _T("Size");
		m_listEnroll.InsertColumn(1, strTitle, LVCFMT_RIGHT, 70);
		strTitle = _T("Date modified");
		m_listEnroll.InsertColumn(2, strTitle, LVCFMT_LEFT, 170);

		m_listEnroll.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
		SetWindowTheme(m_listEnroll.GetSafeHwnd(), L"Explorer", NULL);
	}
	m_listEnroll.GetWindowRect(&rtEnrollList);
	ScreenToClient(&rtEnrollList);

	// resize load button
	nBtnX = rtEnrollList.right + 5;
	nBtnY = rtCropBtn.top;
	nBtnWidth = 100;
	nBtnHeight = 22;
	CRect rtResizeLoadBtn(nBtnX, nBtnY, nBtnX + nBtnWidth, nBtnY + nBtnHeight);
	if (m_btnLoadResize.Create(_T("Resize [Load]"), dwBtnStyle, rtResizeLoadBtn, this, ID_RESIZE_LOAD_BTN) == FALSE)
	{
		FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
		LOG_PUTS(0, _T("Failed to create load button window."));
		return FALSE;
	}
	m_btnLoadResize.SetFont(&m_fontControl);
	m_btnLoadResize.GetWindowRect(&rtResizeLoadBtn);
	ScreenToClient(&rtResizeLoadBtn);

	// resize clear button
	nBtnX = rtResizeLoadBtn.right + 5;
	CRect rtResizeClearBtn(nBtnX, nBtnY, nBtnX + nBtnWidth, nBtnY + nBtnHeight);
	if (m_btnClearResize.Create(_T("Resize [Clear]"), dwBtnStyle, rtResizeClearBtn, this, ID_RESIZE_CLEAR_BTN) == FALSE)
	{
		FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
		LOG_PUTS(0, _T("Failed to create clear button window."));
		return FALSE;
	}
	m_btnClearResize.SetFont(&m_fontControl);
	m_btnClearResize.GetWindowRect(&rtResizeClearBtn);
	ScreenToClient(&rtResizeClearBtn);

	// resize list
	nListX = rtResizeLoadBtn.left;
	nListY = rtCropList.top;
	CRect rtResizeList(nListX, nListY, nListX + nListWidth, nListY + nListHeight);
	if (m_listResize.Create(dwStyle, rtResizeList, this, ID_RESIZE_LIST) == FALSE)
	{
		FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
		LOG_PUTS(0, _T("Failed to create 'Resize' list window."));
		return FALSE;
	}
	else
	{
		CString strTitle = _T("Name");
		m_listResize.InsertColumn(0, strTitle, LVCFMT_LEFT, 120);
		strTitle = _T("Size");
		m_listResize.InsertColumn(1, strTitle, LVCFMT_RIGHT, 70);
		strTitle = _T("Date modified");
		m_listResize.InsertColumn(2, strTitle, LVCFMT_LEFT, 170);

		m_listResize.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
		SetWindowTheme(m_listResize.GetSafeHwnd(), L"Explorer", NULL);
	}
	m_listResize.GetWindowRect(&rtResizeList);
	ScreenToClient(&rtResizeList);
/*
	// crop view wnd
	int nCropSX = rtCropList.right + 5;
	int nCropEX = nDlgWidth - 5;
	int nCropSY = rtView.top;
	int nCropViewWidth = (nCropEX - nCropSX);
	int nCropViewHeight = rtCropList.Height();

	strTestClassName.Format(_T("CropView_%X%X%X"),
		GetTickCount(),
		GetCurrentProcessId(),
		GetCurrentThreadId());
	if (RegisterWindowClass(AfxGetInstanceHandle(), strTestClassName))
	{
		int nSX = nCropSX;
		int nSY = nCropSY;
		int nSW = nCropViewWidth;
		int nSH = nCropViewHeight;

		strTestWndName = _T("CropViewWnd");

		CRect rt(nSX, nSY, nSX + nSW, nSY + nSH);
		int nID = ID_CROP_WND;
		if (m_wndCropView.Create(strTestClassName, strTestWndName, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, rt, this, nID) == FALSE)
		{
			FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
			LOG_PUTS(0, _T("Failed to create crop-view window."));
			return FALSE;
		}
	}	
*/

	bool bRet = initView(&m_wndTest, pDlg->m_RealTimeRenderViewport, pDlg->m_pDirect3DDevice, pDlg->m_pDirect3DSurfaceRender, WIDTH_EYE_VIEW, HEIGHT_EYE_VIEW);
	if (bRet == false)
	{
		FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
		LOG_PUTS(0, _T("Failed to init config view Direct3D."));
	}
	bRet = initView(&m_wndPartialView, pDlg->m_RealTimeRenderViewportRealPartial, pDlg->m_pDirect3DDeviceRealPartial, pDlg->m_pDirect3DSurfaceRenderRealPartial, pDlg->m_nPartialWidth, pDlg->m_nPartialHeight);
	if (bRet == false)
	{
		FUNCTION_NAME_IS(_T("CConfigDialog::InitControl"));
		LOG_PUTS(0, _T("Failed to init config view Direct3D."));
	}

	setRealTimeRenderer(::configRender);

	m_btnLoadCrop.PostMessage(BM_CLICK, 0, 0);
	m_btnLoadEnroll.PostMessage(BM_CLICK, 0, 0);
	m_btnLoadResize.PostMessage(BM_CLICK, 0, 0);

	return TRUE;
}

void CConfigDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	
	int x = point.x - RENDER_OFFSET_X;
	int y = point.y - RENDER_OFFSET_Y;
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;

	int width = pDlg->m_RealTimeRenderViewport.right - pDlg->m_RealTimeRenderViewport.left;
	int height = pDlg->m_RealTimeRenderViewport.bottom - pDlg->m_RealTimeRenderViewport.top;
//	std::cout << "Width " << width << " Height " << height;
	float xratio = (float)x / width;
	float yratio = (float)y / height;
	//std::cout << "Ratio " << xratio << " " << yratio << std::endl;
	int X = (int)(xratio * WIDTH_EYE_VIEW);
	int Y = (int)(yratio * HEIGHT_EYE_VIEW);
	//std::cout << "Mouse " << X << " " << Y << std::endl;
	partialRender(X, Y, partialRenderCb);

	CDialogEx::OnLButtonDown(nFlags, point);
}

void CConfigDialog::ClearListData(CListCtrl* pList, TCHAR* pszFindExt)
{
	_tfinddata_t c_file;

	intptr_t hFile = 0;
	
	hFile = _tfindfirst(pszFindExt, &c_file);
	if (hFile == -1L)
		return;

	pList->DeleteAllItems();

	TCHAR szTemp[_MAX_PATH] = _T("");
	TCHAR szDir[_MAX_PATH] = _T("");
	TCHAR* pch = _tcsrchr(pszFindExt, _T('\\'));
	if (pch)
	{
		_tcsncpy(szDir, pszFindExt, pch - pszFindExt);
	}

	int nIndex = 0;
	while (TRUE)
	{
		_stprintf(szTemp, _T("%s\\%s"), szDir, c_file.name);
		_tunlink(szTemp);

		if (_tfindnext(hFile, &c_file) == -1)
		{
			_findclose(hFile);
			break;
		}
		nIndex++;
	}

	pList->DeleteAllItems();
}

void CConfigDialog::LoadListData(CListCtrl* pList, TCHAR* pszFindExt, int nSortColumn, bool& bSortAscending)
{
	_tfinddata_t c_file;

	intptr_t hFile = 0;

	hFile = _tfindfirst(pszFindExt, &c_file);
	if (hFile == -1L)
		return;

	pList->DeleteAllItems();

	int nIndex = 0;
	while (TRUE)
	{
		CString str = _T("");
		str.Format(_T("%s"), c_file.name);
		int nRow = pList->InsertItem(nIndex, str);
		float nFileSize = (float)c_file.size / 1024.0;
		if (nFileSize > 0.0f && nFileSize < 1.0f)
			nFileSize = 1;
		str.Format(_T("%d KB"), (int)nFileSize);
		pList->SetItem(nIndex, 1, LVIF_TEXT, str, 0, 0, 0, 0);

		TCHAR szTime[64];
		_tctime_s(szTime, _countof(szTime), &c_file.time_access);
		pList->SetItem(nIndex, 2, LVIF_TEXT, szTime, 0, 0, 0, 0);

		pList->SetItemData(nIndex, (DWORD_PTR)nRow);

		if (_tfindnext(hFile, &c_file) == -1)
		{
			_findclose(hFile);
			break;
		}
		nIndex++;
	}

	LISTSORTPARAM sortParam;
	sortParam.bSortAscending = bSortAscending;
	sortParam.pWndList = pList;
	sortParam.nSortColumn = nSortColumn;
	bSortAscending = !bSortAscending;
	pList->SortItems(ListSortFunc, (LPARAM)&sortParam);
}

int CALLBACK CConfigDialog::ListSortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	LISTSORTPARAM* pSortParam = (LISTSORTPARAM*)lParamSort;
	CListCtrl* pListCtrl = pSortParam->pWndList;
	bool bSortAscending = pSortParam->bSortAscending;
	int nSortColumn = pSortParam->nSortColumn;

	LVFINDINFO info1, info2;
	info1.flags = LVFI_PARAM;
	info1.lParam = lParam1;
	info2.flags = LVFI_PARAM;
	info2.lParam = lParam2;
	int irow1 = pListCtrl->FindItem(&info1, -1);
	int irow2 = pListCtrl->FindItem(&info2, -1);

	CString strItem1 = pListCtrl->GetItemText(irow1, nSortColumn);
	CString strItem2 = pListCtrl->GetItemText(irow2, nSortColumn);
		
	return bSortAscending ? _tcscmp(strItem1, strItem2) : -_tcscmp(strItem1, strItem2);
}

void CConfigDialog::OnBnClickedButtonShowValue()
{
	m_bPartialShowValue = m_chkShowValue.GetCheck();
	if (m_wndPartialView.GetSafeHwnd())
	{
		m_wndPartialView.Invalidate();
	}
}

void CConfigDialog::OnBnClickedButtonViewMode()
{
	switch (m_eViewMode)
	{
	case eVIEW_READY:
	case eVIEW_IMAGE:
		{
			m_eViewMode = eVIEW_REAL;
			theMainDlg->PostMessage(WM_COMMAND, MAKEWPARAM(IDC_BUTTON_START, BN_CLICKED), (LPARAM)0);
		}
		break;
	case eVIEW_REAL:
		{
			m_eViewMode = eVIEW_IMAGE;
		}
		break;
	}
	UpdateViewModeButton();
}

void CConfigDialog::CallPartailRender()
{
	if (m_wndTest.GetSafeHwnd())
	{
		m_wndTest.CallPartailRender();
	}
}

void CConfigDialog::UpdateViewModeButton()
{
	if (m_btnViewMode.GetSafeHwnd())
	{
		CString strCaption = _T("View [Ready]");
		switch (m_eViewMode)
		{
		case eVIEW_REAL:
			{
				strCaption = _T("View [Real]");
			}
			break;
		case eVIEW_IMAGE:
			{
				strCaption = _T("View [Image]");
			}
			break;
		}
		m_btnViewMode.SetWindowText(strCaption);
		m_wndPartialView.UpdateZoomDlg();
	}
}

void CConfigDialog::OnBnClickedButtonPartial()
{
	switch (m_eViewMode)
	{
	case eVIEW_REAL:
		{
			m_wndTest.CallPartailRender();
		}
		break;
	case eVIEW_IMAGE:
		{
			//
		}
		break;
	}
}

int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	switch (uMsg)
	{
	case BFFM_INITIALIZED:
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
		break;
	}
	return 0;
}

BOOL GetFolderBrowse(HWND hWndOwner,
	LPTSTR lpszTitle, LPTSTR lpszRootFolder, LPTSTR lpszStartFolder,
	LPTSTR lpszSelectedFolder, BFFCALLBACK lpfn)
{
	TCHAR szDisplayName[MAX_PATH] = _T("");
	LPITEMIDLIST lpIDList;
	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(BROWSEINFO));

	bi.hwndOwner = hWndOwner;
	if (lpszRootFolder)
	{
		LPITEMIDLIST lpIDListTemp = NULL;
		IShellFolder* pDesktopFolder = NULL;
		TCHAR szPath[MAX_PATH] = _T("");
		WCHAR wszPath[MAX_PATH] = L"";
		ULONG chEaten;
		ULONG dwAttributes;

		_tcscpy(szPath, lpszRootFolder);

		if (SUCCEEDED(SHGetDesktopFolder(&pDesktopFolder)))
		{
#if !defined(_UNICODE)
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szPath, -1, wszPath, MAX_PATH);
#endif
			pDesktopFolder->ParseDisplayName(NULL, NULL, wszPath, &chEaten, &lpIDListTemp, &dwAttributes);
			pDesktopFolder->Release();
		}
		bi.pidlRoot = lpIDListTemp;
	}
	else
	{
		bi.pidlRoot = NULL;
	}
	bi.pidlRoot = NULL;

	bi.pszDisplayName = szDisplayName;
	bi.lpszTitle = lpszTitle;
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT;
	bi.lpfn = BrowseCallbackProc;

	if (lpszStartFolder && _tcslen(lpszStartFolder) > 0)
	{
		_tcscpy(lpszSelectedFolder, lpszStartFolder);
		bi.lParam = (LPARAM)lpszStartFolder;
	}
	else
	{
		bi.lParam = FALSE;
	}
	bi.iImage = NULL;
	lpIDList = SHBrowseForFolder(&bi);

	if (lpIDList != NULL)
	{
		BOOL bRet = SHGetPathFromIDList(lpIDList, szDisplayName);
		if (bRet)
		{
			_tcscpy(lpszSelectedFolder, szDisplayName);
			return TRUE;
		}
	}
	else
	{
		_tcscpy(lpszSelectedFolder, _T(""));
		return  FALSE;
	}

	return FALSE;
}

void CConfigDialog::OnBnClickedButtonRealSaveBrowse()
{
	TCHAR szFolder[_MAX_PATH] = _T("");
	GetFolderBrowse(m_hWnd, _T("Select directory"), _T("C:\\jtwoc\\crop"), m_szCropFolder, szFolder, NULL);
	if (_tcslen(szFolder) > 0)
	{
		_tcscpy(m_szCropFolder, szFolder);
	}
	if (m_editRealSave.GetSafeHwnd())
	{
		m_editRealSave.SetWindowTextW(m_szCropFolder);
	}
}

void CConfigDialog::OnBnClickedButtonRealSave()
{
	if (m_editRealSave.GetSafeHwnd())
	{
		bool bRealSave = m_chkRealSave.GetCheck();

		TCHAR szFolder[_MAX_PATH] = _T("");
		m_editRealSave.GetWindowText(szFolder, _MAX_PATH);
		
		if (::PathIsDirectory(szFolder))
		{
			int cchDestChar = (2 * wcslen(szFolder)) + 1;
			char szCropPath[_MAX_PATH] = "";
			WideCharToMultiByte(CP_ACP, 0, szFolder, -1, (LPSTR)szCropPath, cchDestChar - 1, NULL, NULL);

			saveCameraCropFrames(bRealSave, szCropPath);
		}
		else
		{
			AfxMessageBox(_T("Doesn't exist folder."));
		}
	}
}

void CConfigDialog::OnBnClickedButtonCropLoad()
{
	TCHAR szPath[_MAX_PATH] = _T("C:\\jtwoc\\crop");
	CreateDirectory(szPath, NULL);
	_tcscpy(szPath, _T("C:\\jtwoc\\crop\\*.png"));
	LoadListData(&m_listCrop, szPath, 2, m_bSortAscendingCrop);
}

void CConfigDialog::OnBnClickedButtonCropClear()
{
	if (MessageBox(_T("[Crop] 이미지파일을 모두 삭제하시겠습니까?"), _T("J2C"), MB_YESNO) == IDYES)
	{
		ClearListData(&m_listCrop, _T("C:\\jtwoc\\crop\\*.png"));
	}
}

void CConfigDialog::OnBnClickedButtonPartialLoad()
{
	TCHAR szPath[_MAX_PATH] = _T("C:\\jtwoc\\enroll");
	CreateDirectory(szPath, NULL);
	_tcscpy(szPath, _T("C:\\jtwoc\\enroll\\*.png"));
	LoadListData(&m_listEnroll, szPath, 2, m_bSortAscendingEnroll);
}

void CConfigDialog::OnBnClickedButtonPartialClear()
{
	if (MessageBox(_T("[Enroll] 이미지파일을 모두 삭제하시겠습니까?"), _T("J2C"), MB_YESNO) == IDYES)
	{
		ClearListData(&m_listEnroll, _T("C:\\jtwoc\\enroll\\*.png"));
	}
}

void CConfigDialog::OnBnClickedButtonResizeLoad()
{
	TCHAR szPath[_MAX_PATH] = _T("C:\\jtwoc\\resize");
	CreateDirectory(szPath, NULL);
	_tcscpy(szPath, _T("C:\\jtwoc\\resize\\*.png"));
	LoadListData(&m_listResize, szPath, 2, m_bSortAscendingResize);
}

void CConfigDialog::OnBnClickedButtonResizeClear()
{
	if (MessageBox(_T("[Resize] 이미지파일을 모두 삭제하시겠습니까?"), _T("J2C"), MB_YESNO) == IDYES)
	{
		ClearListData(&m_listResize, _T("C:\\jtwoc\\resize\\*.png"));
	}
}

void CConfigDialog::OnNMDblclkListCrop(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if (pNMItemActivate->iItem >= 0)
	{
		int nRow = pNMItemActivate->iItem;
		CString strName = m_listCrop.GetItemText(nRow, 0);

		TCHAR szPath[_MAX_PATH] = _T("");
		_stprintf(szPath, _T("C:\\jtwoc\\crop\\%s"), strName);
		UpdateAllView(szPath);
	}

	*pResult = 0;
}

void CConfigDialog::OnNMDblclkListEnroll(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if (pNMItemActivate->iItem >= 0)
	{
		int nRow = pNMItemActivate->iItem;
		CString strName = m_listEnroll.GetItemText(nRow, 0);

		TCHAR szPath[_MAX_PATH] = _T("");
		_stprintf(szPath, _T("C:\\jtwoc\\enroll\\%s"), strName);
		UpdateAllView(szPath);
	}

	*pResult = 0;
}

void CConfigDialog::OnNMDblclkListResize(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if (pNMItemActivate->iItem >= 0)
	{
		int nRow = pNMItemActivate->iItem;
		CString strName = m_listResize.GetItemText(nRow, 0);

		TCHAR szPath[_MAX_PATH] = _T("");
		_stprintf(szPath, _T("C:\\jtwoc\\resize\\%s"), strName);
		UpdateAllView(szPath);
	}

	*pResult = 0;
}

void CConfigDialog::UpdateAllView(TCHAR* pszPath)
{
	/*
	TCHAR* pszFileName = NULL;
	TCHAR* pch = _tcsrchr(pszPath, _T('\\'));
	if (pch)
	{
		pszFileName = pch + 1;
	}

	CString strFName;
	AfxExtractSubString(strFName, pszFileName, 0, _T('.'));
	TCHAR* pszFName = strFName.GetBuffer();

	int nIndexValue = -1;
	TCHAR* pchIndex = _tcsrchr(pszFName, _T('_'));
	if (pchIndex)
	{
		nIndexValue = _ttoi(pchIndex + 1);
	}
	if (nIndexValue <= 0)
	{
		FUNCTION_NAME_IS(_T("CConfigDialog::UpdateAllView"));
		LOG_PRINTF(0, _T("Invalid file path.[%s]"), pszPath);
		return;
	}

	TCHAR szPartialPath[_MAX_PATH] = _T("");
	TCHAR szCropPath[_MAX_PATH] = _T("");
	if (nIndexValue > -1)
	{
		_stprintf(szCropPath, _T("C:\\jtwoc\\crop\\crop_%d.png"), nIndexValue);
		_stprintf(szPartialPath, _T("C:\\jtwoc\\enroll\\partial_%d.png"), nIndexValue);

		if (_taccess(szCropPath, 0) == 0 && _taccess(szPartialPath, 0) == 0)
		{
			//
		}
		else
		{
			FUNCTION_NAME_IS(_T("CConfigDialog::UpdateAllView"));
			if (_taccess(szCropPath, 0) != 0)
				LOG_PRINTF(0, _T("Doesn't exist crop file.[%s]"), szCropPath);
			else if (_taccess(szPartialPath, 0) != 0)
				LOG_PRINTF(0, _T("Doesn't exist partial file.[%s]"), szPartialPath);
			return;
		}
	}
	*/

	m_eViewMode = eVIEW_IMAGE;
	UpdateViewModeButton();

	if (::PathIsDirectory(pszPath))
	{
		EyeFindTestDir(pszPath);
	}
	else
	{
		LoadFixModeImage(pszPath);

		//
		// cscho (2018-12.22)
		//
		EyeFindTest(pszPath);
	}
}

unsigned CALLBACK EyeFindTestThread(void* pvParam)
{
	EyeFindParam* param = (EyeFindParam*)pvParam;
	if (!param)
		return 0;

	CString strDir = param->szPath;
	CFileFind ff;
	BOOL bFound = ff.FindFile(strDir + _T("\\*.*"), 0);
	while (bFound)
	{
		bFound = ff.FindNextFile();
		if (ff.GetFileName() == _T(".") || ff.GetFileName() == _T(".."))
			continue;

		if (ff.IsDirectory())
		{
			//
		}
		else
		{
			CString strFilePath = ff.GetFilePath();
			::SendMessage(param->pThis->GetSafeHwnd(), UM_FILE_EYEFIND_TEST, 0, (LPARAM)strFilePath.GetBuffer());
		}
	}
	ff.Close();

	delete param;

	return 1;
}

void CConfigDialog::EyeFindTestDir(TCHAR* pszPath)
{
	EyeFindParam* param = new EyeFindParam;
	if (param)
	{
		param->pThis = this;
		_tcscpy(param->szPath, pszPath);
	}
	unsigned uThreadId = 0;
	HANDLE uThread = (HANDLE)_beginthreadex(NULL, 0, EyeFindTestThread, (void*)param, 0, &uThreadId);
	if (uThread)
		CloseHandle(uThread);
}

void CConfigDialog::EyeFindTest(TCHAR* pszPath)
{
	int cchDestChar = (2 * wcslen(pszPath)) + 1;
	char szFilePath[_MAX_PATH] = "";
	WideCharToMultiByte(CP_ACP, 0, pszPath, -1, (LPSTR)szFilePath, cchDestChar - 1, NULL, NULL);
	J2CEyeFindTest(szFilePath);
}

void CConfigDialog::LoadFixModeImage(TCHAR* pszImagePath)
{
	// crop bitmap
	if (m_pBmpFixCrop)
	{
		delete m_pBmpFixCrop;
		m_pBmpFixCrop = NULL;
	}
	m_pBmpFixCrop = new Gdiplus::Bitmap(pszImagePath);
	if (m_pBmpFixCrop)
	{
		int nBmpWidth = m_pBmpFixCrop->GetWidth();
		int nBmpHeight = m_pBmpFixCrop->GetHeight();
		FUNCTION_NAME_IS(_T("CConfigDialog::LoadFixModeImage"));
		LOG_PRINTF(0, _T("Load image file.(%d x %d) [%s]"), nBmpWidth, nBmpHeight, pszImagePath);

		m_wndTest.UpdateImage(m_pBmpFixCrop);
		m_wndPartialView.UpdateImage();		
	}
	else
	{
		FUNCTION_NAME_IS(_T("CConfigDialog::LoadFixModeImage"));
		LOG_PRINTF(0, _T("Failed to load image file. [%s]"), pszImagePath);
	}
}

BOOL CConfigDialog::PreTranslateMessage(MSG* pMsg)
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
					if (GetAsyncKeyState(VK_CONTROL) < 0 && GetAsyncKeyState(VK_SHIFT) < 0)
					{
						m_wndTest.CallPartailRender();
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

void CConfigDialog::OnHdnItemclickCropList(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLISTVIEW* pLV = (NMLISTVIEW*)pNMHDR;
	int nItem = pLV->iItem;

	HWND hWndHdr = pLV->hdr.hwndFrom;
	HWND hWndHdrCrop = m_listCrop.GetHeaderCtrl()->GetSafeHwnd();
	HWND hWndHdrEnroll = m_listEnroll.GetHeaderCtrl()->GetSafeHwnd();
	HWND hWndHdrResize = m_listResize.GetHeaderCtrl()->GetSafeHwnd();

	CListCtrl* pListCtrl = NULL;
	LISTSORTPARAM sortParam;
	if (hWndHdr == hWndHdrCrop)
	{
		sortParam.bSortAscending = m_bSortAscendingCrop;
		pListCtrl = &m_listCrop;
		sortParam.pWndList = &m_listCrop;
		sortParam.nSortColumn = nItem;
		m_bSortAscendingCrop = !m_bSortAscendingCrop;
	}
	else if (hWndHdr == hWndHdrEnroll)
	{
		sortParam.bSortAscending = m_bSortAscendingEnroll;
		pListCtrl = &m_listEnroll;
		sortParam.pWndList = &m_listEnroll;
		sortParam.nSortColumn = nItem;
		m_bSortAscendingEnroll = !m_bSortAscendingEnroll;
	}
	else if (hWndHdr == hWndHdrResize)
	{
		sortParam.bSortAscending = m_bSortAscendingResize;
		pListCtrl = &m_listResize;
		sortParam.pWndList = &m_listResize;
		sortParam.nSortColumn = nItem;
		m_bSortAscendingResize = !m_bSortAscendingResize;
	}
	if (pListCtrl)
	{
		pListCtrl->SortItems(ListSortFunc, (LPARAM)&sortParam);
	}
	
	*pResult = 0;
}

LRESULT CConfigDialog::OnUmFileEyeFindTest(WPARAM wParam, LPARAM lParam)
{
	TCHAR* tszFilePath = (TCHAR*)lParam;
	if (!tszFilePath || _tcslen(tszFilePath) <= 0)
		return 0L;

	EyeFindTest(tszFilePath);
}
