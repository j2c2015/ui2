// CFileDropTarget.cpp : implementation file
//

#include "stdafx.h"
#include "ui0.h"
#include "CFileDropTarget.h"


// CFileDropTarget


CFileDropTarget::CFileDropTarget()
{
}

CFileDropTarget::~CFileDropTarget()
{
}

BOOL CFileDropTarget::OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	if (pDataObject->IsDataAvailable(CF_HDROP))
	{
		STGMEDIUM stgMed;

		FORMATETC fmte = { CF_HDROP, (DVTARGETDEVICE FAR*)NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		if (pDataObject->GetData(CF_HDROP, &stgMed, &fmte))
		{
			HDROP hDrop = (HDROP)stgMed.hGlobal;

			UINT nCntFiles = ::DragQueryFile(hDrop, (UINT)-1, NULL, 0);
			if (nCntFiles <= 0)
				return FALSE;

			TCHAR tszFile[MAX_PATH] = _T("");
			if (pWnd && pWnd->GetSafeHwnd())
			{
				pWnd->SendMessage(UM_INIT_FILEADD, (WPARAM)0, (LPARAM)0);
			}
			for (UINT ui = 0; ui < nCntFiles; ui++)
			{
				::DragQueryFile(hDrop, ui, tszFile, sizeof(tszFile));

				if (pWnd && pWnd->GetSafeHwnd())
				{
					pWnd->SendMessage(UM_FILE_DROP, (WPARAM)0, (LPARAM)tszFile);
				}
			}
		}

		return TRUE;
	}

	return COleDropTarget::OnDrop(pWnd, pDataObject, dropEffect, point);
}

DROPEFFECT CFileDropTarget::OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	return GetDropEffect(pDataObject);

}

DROPEFFECT CFileDropTarget::OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	return GetDropEffect(pDataObject);
}

DROPEFFECT CFileDropTarget::GetDropEffect(COleDataObject* pDataObject)
{
	DROPEFFECT dropEffect = DROPEFFECT_NONE;

	if (pDataObject->IsDataAvailable(CF_HDROP))
	{
		dropEffect = DROPEFFECT_COPY;
	}

	return dropEffect;
}
