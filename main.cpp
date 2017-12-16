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

void CspEnumProviders(HWND hListBox);
void CspEnumContainers(HWND hListBox, LPCTSTR pszProvider, DWORD dwProvType);

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
					DWORD cchProvider;
					LPTSTR pszProvider;
					DWORD dwProvType;
					int uSelectedItem;
					uSelectedItem = ListBox_GetCurSel(hListBox);
					if (uSelectedItem != LB_ERR) {	
						cchProvider = ListBox_GetTextLen(hListBox, uSelectedItem);
						pszProvider = new TCHAR[cchProvider + 1];
						ListBox_GetText(hListBox, uSelectedItem, pszProvider);
						if (_tcscmp(pszProvider, TEXT("..")) == 0) {
							SetWindowText(hWnd, WND_TITLE);
							CspEnumProviders(hListBox);
							break;
						}
						dwProvType = ListBox_GetItemData(hListBox, uSelectedItem);
						DEBUG_INFO(pszProvider, dwProvType)
						CspEnumContainers(hListBox, pszProvider, dwProvType);
						delete pszProvider;
					}
				} catch (win32::win32_error& ex) {
					HANDLE_ERROR(ex.what(), ex.code())
				}
			}
		}
		break;
	  case WM_DESTROY:
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

void CspEnumContainers(HWND hListBox, LPCTSTR pszProvider, DWORD dwProvType) {
	HCRYPTPROV hProv;
	if (! CryptAcquireContext(&hProv, NULL, pszProvider, dwProvType, CRYPT_VERIFYCONTEXT))
		throw win32::win32_error("CryptAcquireContext");
	
	DWORD cbName;
	LPBYTE pbName;
	LPTSTR pszName;
	
	if (! CryptGetProvParam(hProv, PP_NAME, NULL, &cbName, 0)) {
		throw win32::win32_error("CryptGetProvParam");
	}
	
	if (!(pbName = (LPBYTE)LocalAlloc(LMEM_ZEROINIT, cbName)))
		throw win32::win32_error("LocalAlloc");
	
	if (!(pszName = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, cbName * sizeof(TCHAR))))
		throw win32::win32_error("LocalAlloc");
	
	if (! CryptGetProvParam(hProv, PP_NAME, pbName, &cbName, 0)) {
		throw win32::win32_error("CryptGetProvParam");
	}
	
	MultiByteToWideChar(CP_ACP, 0,
		(LPCSTR)pbName, -1, pszName, cbName);
	
	HWND hWnd = GetParent(hListBox);
	SetWindowText(hWnd, pszName);
	
	LocalFree(pbName);
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
	
	if (!(pbName = (LPBYTE)LocalAlloc(LMEM_ZEROINIT, cbName)))
		throw win32::win32_error("LocalAlloc");
	
	if (!(pszName = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, cbName * sizeof(TCHAR))))
		throw win32::win32_error("LocalAlloc");
	
	if (! CryptGetProvParam(hProv, PP_ENUMCONTAINERS, pbName, &cbName, CRYPT_FIRST)) {
		if (GetLastError() == ERROR_NO_MORE_ITEMS) {
			LocalFree(pbName);
			LocalFree(pszName);
			CryptReleaseContext(hProv, 0);
			return;
		}
		throw win32::win32_error("CryptGetProvParam");
	}
	
	do {
		MultiByteToWideChar(CP_ACP, 0,
			(LPCSTR)pbName, -1, pszName, cbName);
		ListBox_AddString(hListBox, pszName);
	} while (CryptGetProvParam(hProv, PP_ENUMCONTAINERS, pbName, &cbName, CRYPT_NEXT));
	
	if (GetLastError() != ERROR_NO_MORE_ITEMS) {
		throw win32::win32_error("CryptGetProvParam");
	}
	
	LocalFree(pbName);
	LocalFree(pszName);
	
	CryptReleaseContext(hProv, 0);
}