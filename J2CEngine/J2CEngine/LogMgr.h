#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tchar.h>
#include <io.h>

#define UM_LOGMGR_BASE					(WM_USER + 200)

#define	MAX_LOG_FNAME					(33)

#define	SECTION_LOG						_T("LOG")
#define	KEYWORD_LOG_LEVEL				_T("LOG_LEVEL")
#define	KEYWORD_LOG_FNAME				_T("LOG_FNAME")
#define	KEYWORD_LOG_KEEP_DAY			_T("LOG_KEEP_DAY")
#define	KEYWORD_LOG_WIN					_T("LOG_WIN")
#define KEYWORD_LOG_BACKUP_SIZE			_T("LOG_BACKUP_SIZE")

#define TRACEWND_CLASSNAME				_T("MfxTraceWindow")

BOOL RUN_MODULE_NAME(HANDLE hInstance, TCHAR* pszPath, TCHAR* pszFName, TCHAR* pszExt);
void RELtoAB_PATH(HANDLE hInstance, TCHAR* pszRelPath);
void TRIM_WS_FBWARD(TCHAR* pszString);

BOOL IsEunYearL(long lYear);
long DATEtoLONG(long lYear, long lMonth, long lDay);
long AtoL(TCHAR* pszData, short nLen);

BOOL RegisterWindowClass(HINSTANCE hInstance, LPCTSTR pszClassName);
void UnregisterWindowClass(LPCTSTR pszClassName);

void FUNCTION_NAME_IS(TCHAR* pszFuncName);

void LOG_INIT(HANDLE hInstance);
void LOG_PUTS(short nLogLevel, TCHAR* pszLogMsg);
void LOG_PRINTF(TCHAR _nLogLevel, TCHAR* pszFormat, ...);

void CheckDeleteLog();
void DeleteOldLog(long lCurDate, TCHAR* pszFName, int nPos);

#pragma pack(push, 1)
class CEllapsedTime
{
public:
	CEllapsedTime();
	virtual ~CEllapsedTime();

protected:
	LARGE_INTEGER	m_liFrequency;
	LARGE_INTEGER	m_liCounter1;
	LARGE_INTEGER	m_liCounter2;
	double			m_fEllapsedTime;

public:
	void			StartEllapsedTime();
	double			EndEllapsedTime();
	double			GetEllapsedTime() { return m_fEllapsedTime; }
};
#pragma pack(pop)
