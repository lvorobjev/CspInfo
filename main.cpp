/**
 * main.cpp
 * Утилита для просмотра параметров криптопровайдера
 * Copyright (c) 2017 Lev Vorobjev
 *
 * THIS IS EXPERIMENTAL SOFTWARE - USE AT YOUR OWN RISK - NO SUPPORT AVAILABLE
 */

#include <windows.h>
#include <windowsx.h>
#include <wincrypt.h>
#include <stdio.h>
#include <tchar.h>
#include <win32/win32_error.h>

#define IDC_LISTBOX  40050

#define WND_TITLE TEXT("Криптопровайдеры")
#define WND_MENU_NAME NULL
#define MSG_TITLE TEXT("CspInfo")
#define BUFFER_SIZE 512

#define HANDLE_ERROR(lpszFunctionName, dwStatus) \
    MultiByteToWideChar(CP_ACP, 0, \
        lpszFunctionName, -1, lpszBuffer, BUFFER_SIZE); \
    _stprintf(lpszBuffer, TEXT("%s error.\nStatus code: %d"), \
        lpszBuffer, dwStatus); \
    MessageBox(hWnd, lpszBuffer, MSG_TITLE, MB_OK | MB_ICONWARNING);

#ifdef _DEBUG
#define DEBUG_INFO(pszProvider, dwProvType) \
    _stprintf(lpszBuffer, TEXT("Provider: %s\nProv Type: %d"), \
        pszProvider, dwProvType); \
    MessageBox(hWnd, lpszBuffer, MSG_TITLE, MB_OK | MB_ICONINFORMATION);

#define DEBUG_DUMP(lpData, dwLenght) \
    for (int i = 0; i < dwLenght; i++) { \
        _stprintf(lpszBuffer + 3 * i, (i % 16 == 15) ? TEXT("%02X\n") : TEXT("%02X "), ((LPBYTE)lpData)[i]); \
    } \
    MessageBox(hWnd, lpszBuffer, MSG_TITLE, MB_OK | MB_ICONINFORMATION);
#else
#define DEBUG_INFO(pszProvider, dwProvType)
#define DEBUG_DUMP(lpData, dwLenght)
#endif
	
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
ATOM RegMyWindowClass(HINSTANCE, LPCTSTR);

typedef struct _KEY_INFO {
	ALG_ID algId;
	DWORD dwKeyLen;
} KEY_INFO;

typedef struct _CONTAINER_INFO {
	LPTSTR pszProvider;
	DWORD dwProvType;
	LPTSTR pszContainer;
	KEY_INFO kiExchange;
	KEY_INFO kiSignature;
} CONTAINER_INFO;

void CspEnumProviders(HWND hListBox);
void CspEnumContainers(HWND hListBox, LPCTSTR pszProvider, DWORD dwProvType);
void CspContainerDetails(CONTAINER_INFO* pCspi);

int APIENTRY WinMain(HINSTANCE hInstance,
             HINSTANCE         hPrevInstance,
             LPSTR             lpCmdLine,
             int               nCmdShow) {
    LPCTSTR lpszClass = TEXT("SPAD_KP_CspInfo_Window");
    LPCTSTR lpszTitle = WND_TITLE;
    HWND hWnd;
    MSG msg = {0};
    BOOL status;

    if (!RegMyWindowClass(hInstance, lpszClass))
        return 1;

    hWnd = CreateWindow(lpszClass, lpszTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL);
    if(!hWnd) return 2;

    ShowWindow(hWnd, nCmdShow);

    while ((status = GetMessage(&msg, NULL, 0, 0 )) != 0) {
        if (status == -1) return 3;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

ATOM RegMyWindowClass(HINSTANCE hInst, LPCTSTR lpszClassName) {
    WNDCLASS wcWindowClass = {0};
    wcWindowClass.lpfnWndProc = (WNDPROC)WndProc;
    wcWindowClass.style = CS_HREDRAW|CS_VREDRAW;
    wcWindowClass.hInstance = hInst;
    wcWindowClass.lpszClassName = lpszClassName;
    wcWindowClass.lpszMenuName = WND_MENU_NAME;
    wcWindowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcWindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcWindowClass.hbrBackground = (HBRUSH) ( COLOR_WINDOW + 1);
    wcWindowClass.cbClsExtra = 0;
    wcWindowClass.cbWndExtra = 0;
    return RegisterClass(&wcWindowClass);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
                         WPARAM wParam, LPARAM lParam) {
    HDC hdc;
    HINSTANCE hInst;
    PAINTSTRUCT ps;
	
	static LPTSTR lpszBuffer;
	DWORD dwStatus;
	
	static HWND hListBox;
	RECT rcClient = {0};
	
	static BOOL bListProv;
	static CONTAINER_INFO cspi = {0};
	
	switch (message) {
      case WM_CREATE:
        hInst = (HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
		lpszBuffer = (LPTSTR) LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*BUFFER_SIZE);
		GetClientRect(hWnd, &rcClient);
		hListBox = CreateWindow(TEXT("LISTBOX"), NULL, 
			WS_CHILD | WS_VISIBLE | LBS_STANDARD | LBS_WANTKEYBOARDINPUT, 
			rcClient.left, rcClient.top, rcClient.right, rcClient.bottom,
			hWnd, (HMENU) IDC_LISTBOX, hInst, NULL);
		try {
			CspEnumProviders(hListBox);
			bListProv = TRUE;
		} catch (win32::win32_error& ex) {
			HANDLE_ERROR(ex.what(), ex.code())
		}
		break;
	  case WM_SIZE:
		SetWindowPos(hListBox, HWND_TOP, 0, 0,
            LOWORD(lParam), HIWORD(lParam), SWP_NOMOVE);
		break;
	  case WM_COMMAND:
        if (LOWORD(wParam) == IDC_LISTBOX) {
			if(HIWORD(wParam) == (unsigned)LBN_ERRSPACE) {
				HANDLE_ERROR("LBN_ERRSPACE", LBN_ERRSPACE);
			} else if (HIWORD(wParam) == LBN_DBLCLK) {
				try {
					DWORD cchItemText;
					LPTSTR pszItemText;
					DWORD dwProvType;
					int uSelectedItem;
					uSelectedItem = ListBox_GetCurSel(hListBox);
					if (uSelectedItem != LB_ERR) {	
						cchItemText = ListBox_GetTextLen(hListBox, uSelectedItem);
						pszItemText = new TCHAR[cchItemText + 1];
						ListBox_GetText(hListBox, uSelectedItem, pszItemText);
						if (_tcscmp(pszItemText, TEXT("..")) == 0) {
							SetWindowText(hWnd, WND_TITLE);
							CspEnumProviders(hListBox);
							bListProv = TRUE;
							break;
						}
						if (bListProv) {
							dwProvType = ListBox_GetItemData(hListBox, uSelectedItem);
							DEBUG_INFO(pszItemText, dwProvType)
							CspEnumContainers(hListBox, pszItemText, dwProvType);
							delete cspi.pszProvider;
							cspi.pszProvider = _tcsdup(pszItemText);
							cspi.dwProvType = dwProvType;
							bListProv = FALSE;
						} else {
							// Вывести инф-ю о контейнере ключей
							delete cspi.pszContainer;
							cspi.pszContainer = _tcsdup(pszItemText);
							CspContainerDetails(&cspi);
							int i = _stprintf(lpszBuffer, TEXT("Имя контейнера: %s\n"), cspi.pszContainer);
							if (cspi.kiExchange.algId != 0) {
								ALG_ID algId = cspi.kiExchange.algId;
								i += _stprintf(lpszBuffer + i, TEXT("Ключ обмена: %s (%d бит)\n"), (algId & ALG_TYPE_RSA) ? TEXT("RSA") : (algId & ALG_TYPE_DH) ? TEXT("DH") : TEXT("ANY"), cspi.kiExchange.dwKeyLen);
							}
							if (cspi.kiSignature.algId != 0) {
								ALG_ID algId = cspi.kiSignature.algId;
								i += _stprintf(lpszBuffer + i, TEXT("Ключ подписи: %s (%d бит)\n"), (algId & ALG_TYPE_RSA) ? TEXT("RSA") : (algId & ALG_TYPE_DSS) ? TEXT("DSS") : TEXT("ANY"), cspi.kiSignature.dwKeyLen);
							}
							MessageBox(hWnd, lpszBuffer, MSG_TITLE, MB_OK | MB_ICONINFORMATION);
						}
						delete pszItemText;
					}
				} catch (win32::win32_error& ex) {
					HANDLE_ERROR(ex.what(), ex.code())
				}
			}
		}
		break;
	  case WM_DESTROY:
		delete cspi.pszProvider;
		delete cspi.pszContainer;
        LocalFree(lpszBuffer);
		PostQuitMessage(0);
		break;
	  default:
        return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void CspEnumProviders(HWND hListBox) {
	DWORD dwIndex=0;
	DWORD dwType;
	DWORD cbName;
	LPTSTR pszName;
	DWORD dwItem;

	ListBox_ResetContent(hListBox);
	
	while (CryptEnumProviders(dwIndex, NULL, 0, &dwType, NULL, &cbName))
	{
	  if (!cbName)
		break;
	   
	  if (!(pszName = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, cbName)))
		throw win32::win32_error("LocalAlloc");
	  
	  if (!CryptEnumProviders(dwIndex++, NULL, 0, &dwType, pszName, &cbName))
		throw win32::win32_error("CryptEnumProviders");
	  
	  dwItem = ListBox_AddString(hListBox, pszName);
	  ListBox_SetItemData(hListBox, dwItem, dwType);
	  
	  LocalFree(pszName);
	}
}

LPTSTR CspAsciiToWideChar(LPCSTR pszAscii) {
	LPTSTR pszWide;
	DWORD cchWide;
	
	cchWide = MultiByteToWideChar(CP_ACP, 0,
		(LPCSTR)pszAscii, -1, NULL, 0);
	if (cchWide == 0)
		throw win32::win32_error("MultiByteToWideChar");
	
	if (!(pszWide = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, cchWide * sizeof(TCHAR))))
		throw win32::win32_error("LocalAlloc");
	
	cchWide = MultiByteToWideChar(CP_ACP, 0,
		(LPCSTR)pszAscii, -1, pszWide, cchWide);
	if (cchWide == 0)
		throw win32::win32_error("MultiByteToWideChar");
	
	return pszWide;
}

LPTSTR CspGetProviderName(HCRYPTPROV hProv) {
	DWORD cbName;
	LPBYTE pbName;
	LPTSTR pszName;
	
	if (! CryptGetProvParam(hProv, PP_NAME, NULL, &cbName, 0))
		throw win32::win32_error("CryptGetProvParam");
	
	if (!(pbName = (LPBYTE)LocalAlloc(LMEM_ZEROINIT, cbName)))
		throw win32::win32_error("LocalAlloc");
	
	if (! CryptGetProvParam(hProv, PP_NAME, pbName, &cbName, 0))
		throw win32::win32_error("CryptGetProvParam");
	
	pszName = CspAsciiToWideChar((LPCSTR)pbName);
	LocalFree(pbName);
	return pszName;
}

void CspEnumContainers(HWND hListBox, LPCTSTR pszProvider, DWORD dwProvType) {
	HCRYPTPROV hProv;
	DWORD cbName;
	LPBYTE pbName;
	LPTSTR pszName;
	
	if (! CryptAcquireContext(&hProv, NULL, pszProvider, dwProvType, CRYPT_VERIFYCONTEXT))
		throw win32::win32_error("CryptAcquireContext");
	
	pszName = CspGetProviderName(hProv);
	HWND hWnd = GetParent(hListBox);
	SetWindowText(hWnd, pszName);
	LocalFree(pszName);
	
	ListBox_ResetContent(hListBox);
	ListBox_AddString(hListBox, TEXT(".."));
	
	if (! CryptGetProvParam(hProv, PP_ENUMCONTAINERS, NULL, &cbName, CRYPT_FIRST)) {
		if (GetLastError() == ERROR_NO_MORE_ITEMS) {
			CryptReleaseContext(hProv, 0);
			return;
		}
		throw win32::win32_error("CryptGetProvParam");
	}
	
	if (!(pbName = (LPBYTE)LocalAlloc(LMEM_ZEROINIT, cbName))) {
		CryptReleaseContext(hProv, 0);
		throw win32::win32_error("LocalAlloc");
	}
	
	if (! CryptGetProvParam(hProv, PP_ENUMCONTAINERS, pbName, &cbName, CRYPT_FIRST)) {
		if (GetLastError() == ERROR_NO_MORE_ITEMS) {
			LocalFree(pbName);
			CryptReleaseContext(hProv, 0);
			return;
		}
		throw win32::win32_error("CryptGetProvParam");
	}
	
	do {
		pszName = CspAsciiToWideChar((LPCSTR)pbName);
		ListBox_AddString(hListBox, pszName);
		LocalFree(pszName);
	} while (CryptGetProvParam(hProv, PP_ENUMCONTAINERS, pbName, &cbName, CRYPT_NEXT));
	
	if (GetLastError() != ERROR_NO_MORE_ITEMS) {
		LocalFree(pbName);
		CryptReleaseContext(hProv, 0);
		throw win32::win32_error("CryptGetProvParam");
	}
	
	LocalFree(pbName);
	CryptReleaseContext(hProv, 0);
}

void CspRetrieveKeyInfo(HCRYPTKEY hKey, KEY_INFO* pKi) {
	DWORD dwDataLen;
	
	dwDataLen = sizeof(ALG_ID);
	if (! CryptGetKeyParam(hKey, KP_ALGID, 
		(BYTE*) &(pKi->algId), &dwDataLen, 0)) {
		throw win32::win32_error("CryptGetKeyParam");
	}
	
	dwDataLen = sizeof(DWORD);
	if (! CryptGetKeyParam(hKey, KP_KEYLEN,
		(BYTE*) &(pKi->dwKeyLen), &dwDataLen, 0)) {
		throw win32::win32_error("CryptGetKeyParam");
	}
}

void CspContainerDetails(CONTAINER_INFO* pCspi) {
	HCRYPTPROV hProv;
	HCRYPTKEY hKeyExch;
	HCRYPTKEY hKeySign;
	DWORD dwStatus;
	
	if (! CryptAcquireContext(&hProv, pCspi->pszContainer, 
		pCspi->pszProvider, pCspi->dwProvType, 0))
		throw win32::win32_error("CryptAcquireContext");
	
	if (! CryptGetUserKey(hProv, AT_KEYEXCHANGE, &hKeyExch)) {
		dwStatus = GetLastError();
		if (dwStatus == NTE_NO_KEY || dwStatus == NTE_BAD_ALGID) {
			pCspi->kiExchange.algId = 0;
			hKeyExch = NULL;
		} else {
			CryptReleaseContext(hProv, 0);
			throw win32::win32_error("CryptGetUserKey", dwStatus);
		}
	}
	
	if (hKeyExch != NULL) {
		CspRetrieveKeyInfo(hKeyExch, &(pCspi->kiExchange));
		CryptDestroyKey(hKeyExch);
	}
	
	if (! CryptGetUserKey(hProv, AT_SIGNATURE, &hKeySign)) {
		dwStatus = GetLastError();
		if (dwStatus == NTE_NO_KEY || dwStatus == NTE_BAD_ALGID) {
			pCspi->kiSignature.algId = 0;
			hKeySign = NULL;
		} else {
			CryptReleaseContext(hProv, 0);
			throw win32::win32_error("CryptGetUserKey", dwStatus);
		}
	}
	
	if (hKeySign != NULL) {
		CspRetrieveKeyInfo(hKeySign, &(pCspi->kiSignature));
		CryptDestroyKey(hKeySign);
	}
	
	CryptReleaseContext(hProv, 0);
}