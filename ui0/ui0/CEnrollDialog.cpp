// CEnrollDialog.cpp: 구현 파일
//

#include "stdafx.h"
#include "ui0.h"
#include "CEnrollDialog.h"
#include "afxdialogex.h"
#include "ui0Dlg.h"
#include <iostream>
//#include "resource.h"

CEnrollDialog *pEnrollDlg=0;
// CEnrollDialog 대화 상자

IMPLEMENT_DYNAMIC(CEnrollDialog, CDialogEx)

CEnrollDialog::CEnrollDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_ENROLLMENT, pParent)
{

}

CEnrollDialog::~CEnrollDialog()
{
}

void CEnrollDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CEnrollDialog, CDialogEx)
	ON_LBN_SELCHANGE(IDC_LIST1, &CEnrollDialog::OnLbnSelchangeList1)
	ON_BN_CLICKED(IDC_BUTTON_ENROLL, &CEnrollDialog::OnBnClickedButtonEnroll)
	ON_EN_CHANGE(IDC_EDIT_ENROLL, &CEnrollDialog::OnEnChangeEditEnroll)
END_MESSAGE_MAP()


// CEnrollDialog 메시지 처리기


void CEnrollDialog::OnLbnSelchangeList1()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

void __enrollCb(bool success)
{
	if (success == false)
	{
		AfxMessageBox(_T("Enroll failed"), MB_OK);
	}
	else {
		AfxMessageBox(_T("Enroll success"), MB_OK);
	}
}

void CEnrollDialog::OnBnClickedButtonEnroll()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString str;
	GetDlgItemText(IDC_EDIT_ENROLL, str);
	char buffer[1024];
	sprintf(buffer, "%S", str);
	std::cout << "### enroll id " << buffer << "###" << std::endl;
	enrollRequest(buffer, ::__enrollCb);
}

void CEnrollDialog::render(unsigned char *pBuffer)
{
	HRESULT lRet;

	if (pDlg->m_pDirect3DSurfaceRenderForEnroll == NULL)
		return;

	D3DLOCKED_RECT d3d_rect;
	lRet = pDlg->m_pDirect3DSurfaceRenderForEnroll->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
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

	lRet = pDlg->m_pDirect3DSurfaceRenderForEnroll->UnlockRect();
	if (FAILED(lRet))
		return;

	if (pDlg->m_pDirect3DDeviceForEnroll == NULL)
		return;

	pDlg->m_pDirect3DDeviceForEnroll->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	pDlg->m_pDirect3DDeviceForEnroll->BeginScene();
	IDirect3DSurface9 * pBackBuffer = NULL;

	pDlg->m_pDirect3DDeviceForEnroll->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	pDlg->m_pDirect3DDeviceForEnroll->StretchRect(pDlg->m_pDirect3DSurfaceRenderForEnroll, NULL, pBackBuffer, &pDlg->m_EnrollRenderViewport, D3DTEXF_LINEAR);
	pDlg->m_pDirect3DDeviceForEnroll->EndScene();
	pDlg->m_pDirect3DDeviceForEnroll->Present(NULL, NULL, NULL, NULL);
}


bool CEnrollDialog::initView()
{
	HWND hwnd = GetDlgItem(IDC_ENROLL_RENDER_VIEW)->m_hWnd;
	bool	bSuccess = false;
	HRESULT hrSuccess;

	D3DPRESENT_PARAMETERS d3dpp;
	D3DFORMAT Format;
	__try
	{
		m_pDirect3D9 = Direct3DCreate9(D3D_SDK_VERSION);

		ZeroMemory(&d3dpp, sizeof(d3dpp));
		d3dpp.Windowed = TRUE;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

		::GetClientRect(hwnd, &pDlg->m_EnrollRenderViewport);
		hrSuccess = m_pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDlg->m_pDirect3DDeviceForEnroll);
		if (FAILED(hrSuccess))
		{
			__leave;
		}

		Format = (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2');	// J2C
		hrSuccess = pDlg->m_pDirect3DDeviceForEnroll->CreateOffscreenPlainSurface(WIDTH_ENROLL_EYE_VIEW, HEIGHT_ENROLL_EYE_VIEW, Format, D3DPOOL_DEFAULT, &pDlg->m_pDirect3DSurfaceRenderForEnroll, NULL);
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
			pDlg->cleanup();
		}
	}

	return bSuccess;
}
// CConfigDialog 메시지 처리기
void __enrollRender(void *buffer, int width, int height, int depth)
{
	pEnrollDlg->render((unsigned char *)buffer);
}


BOOL CEnrollDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.
	pEnrollDlg = this;
	initView();
	setEnrollRenderer(::__enrollRender);
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


void CEnrollDialog::OnEnChangeEditEnroll()
{
	// TODO:  RICHEDIT 컨트롤인 경우, 이 컨트롤은
	// CDialogEx::OnInitDialog() 함수를 재지정 
	//하고 마스크에 OR 연산하여 설정된 ENM_CHANGE 플래그를 지정하여 CRichEditCtrl().SetEventMask()를 호출하지 않으면
	// 이 알림 메시지를 보내지 않습니다.

	// TODO:  여기에 컨트롤 알림 처리기 코드를 추가합니다.
}
