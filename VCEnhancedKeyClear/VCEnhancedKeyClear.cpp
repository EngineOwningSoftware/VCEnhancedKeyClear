#include "stdafx.h"
#include "resource.h"
#include "defines.h"
#include "Installer.h"

#include <Windows.h>
#include <wtsapi32.h>
#include <strsafe.h>

NOTIFYICONDATA nid{};

bool VeraCryptBuildDeviceList()
{
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	bool bRet = false;

	if (hSCManager != NULL)
	{
		SC_HANDLE hService = OpenService(hSCManager, TC_SYSTEM_FAVORITES_SERVICE_NAME, SERVICE_ALL_ACCESS);

		if (hService != NULL)
		{
			SERVICE_STATUS serviceStatus{};

			if (ControlService(hService, VC_SERVICE_CONTROL_BUILD_DEVICE_LIST, &serviceStatus))
			{
				bRet = true;
			}

			CloseServiceHandle(hService);
		}

		CloseServiceHandle(hSCManager);
	}

	return bRet;
}

bool SetClearKeysFlag(bool state)
{
	HKEY hkey = NULL;
	DWORD configMap = 0;
	DWORD type = 0;
	DWORD size = sizeof(DWORD);

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\veracrypt"), 0, KEY_READ | KEY_WRITE, &hkey) != ERROR_SUCCESS)
	{
		return false;
	}

	if (RegQueryValueEx(hkey, _T("VeraCryptConfig"), NULL, &type, reinterpret_cast<BYTE *>(&configMap), &size) != ERROR_SUCCESS)
	{
		RegCloseKey(hkey);
		return false;
	}

	if (state)
		configMap |= VC_DRIVER_CONFIG_CLEAR_KEYS_ON_NEW_DEVICE_INSERTION;
	else
		configMap &= ~VC_DRIVER_CONFIG_CLEAR_KEYS_ON_NEW_DEVICE_INSERTION;

	bool result = true;

	if (RegSetValueEx(hkey, _T("VeraCryptConfig"), NULL, type, reinterpret_cast<BYTE *>(&configMap), size) != ERROR_SUCCESS)
	{
		result = false;
	}

	RegCloseKey(hkey);
	return result;
}

LRESULT CALLBACK VcekcWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_WTSSESSION_CHANGE:
	{
		bool resSetConfig = true;
		bool resBuildDeviceList = true;

		switch (wParam)
		{
		default: break;
		case WTS_SESSION_UNLOCK:
			resSetConfig = SetClearKeysFlag(false);
			break;
		case WTS_SESSION_LOCK:
			resBuildDeviceList = VeraCryptBuildDeviceList();
			resSetConfig = SetClearKeysFlag(true);
			break;
		}

		if (!resSetConfig)
		{
			MessageBox(NULL, _T("Failed to change VC_DRIVER_CONFIG_CLEAR_KEYS_ON_NEW_DEVICE_INSERTION"), VCEKC_MSGTITLE, MB_ICONEXCLAMATION | MB_OK);
		}

		if (!resBuildDeviceList)
		{
			MessageBox(NULL, _T("Failed to send VC_SERVICE_CONTROL_BUILD_DEVICE_LIST"), VCEKC_MSGTITLE, MB_ICONEXCLAMATION | MB_OK);
		}

		break;
	}
	case VCEKC_SHELLICONMSG:
	{
		switch (lParam)
		{
		case WM_RBUTTONDOWN:
		case WM_CONTEXTMENU:
			if (MessageBox(NULL, _T("Are you sure you want to quit VeraCrypt Enhanced Key Clear?"), VCEKC_MSGTITLE, MB_ICONQUESTION | MB_YESNO) == IDYES)
			{
				PostQuitMessage(0);
			}
			break;
		}

		break;
	}
	case WM_DESTROY:
	{
		Shell_NotifyIcon(NIM_DELETE, &nid);
		break;
	}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

int wWinMain(HINSTANCE hInstance, HINSTANCE /* hPrevInstance */, PWSTR lpCmdLine, int /* nShowCmd */)
{
	// Handle install/uninstall command line parameters
	int argc = 0;
	auto argv = CommandLineToArgvW(lpCmdLine, &argc);

	if (argv != nullptr)
	{
		TCHAR OwnPath[MAX_PATH]{};
		GetModuleFileName(NULL, OwnPath, _countof(OwnPath));

		CInstaller installer(OwnPath);

		for (int i = 0; i < argc; i++)
		{
			DWORD dwStatus = ERROR_SUCCESS;

			if (!_wcsicmp(argv[i], L"--install"))
			{
				dwStatus = installer.Install();
				if (dwStatus == ERROR_SUCCESS)
				{
					wprintf(L"Successfully installed VeraCryptEnhancedKeyClear to %s\n", installer.GetTargetPath().c_str());
					return 0;
				}

				wprintf(L"Failed to install VeraCryptEnhancedKeyClear: 0x%X\n", dwStatus);
				return dwStatus;
			}
			else if (!_wcsicmp(argv[i], L"--uninstall"))
			{
				dwStatus = installer.Uninstall();
				if (dwStatus == ERROR_SUCCESS)
				{
					wprintf(L"Successfully removed VeraCryptEnhancedKeyClear\n");
					return 0;
				}

				wprintf(L"Failed to uninstall VeraCryptEnhancedKeyClear: 0x%X\n", dwStatus);
				return dwStatus;
			}
			else if (!_wcsicmp(argv[i], L"--autorun"))
			{
				STARTUPINFOW startupInfo{};
				PROCESS_INFORMATION processInfo{};

				startupInfo.cb = sizeof(startupInfo);

				if (CreateProcessW(NULL, OwnPath, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo))
				{
					CloseHandle(processInfo.hThread);
					CloseHandle(processInfo.hProcess);
					return ERROR_SUCCESS;
				}

				return GetLastError();
			}
		}

		LocalFree(argv);
	}

	// Check if another instance is running
	SetLastError(0);
	auto mutex = CreateMutex(NULL, TRUE, VCEKC_MUTEX);
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		MessageBox(NULL, _T("Another instance of this program is already running. Make sure to close it first."), VCEKC_MSGTITLE, MB_ICONEXCLAMATION);
		return -1;
	}

	// Create hidden window
	WNDCLASS wndclass{};
	wndclass.style = CS_DBLCLKS | CS_PARENTDC;
	wndclass.lpfnWndProc = VcekcWndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = NULL;
	wndclass.hCursor = LoadCursor(NULL, (LPTSTR)IDC_IBEAM);
	wndclass.hbrBackground = NULL;
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = VCEKC_CLASSNAME;

	if (!RegisterClass(&wndclass))
	{
		MessageBox(NULL, _T("RegisterClass() failed"), VCEKC_MSGTITLE, MB_ICONERROR | MB_OK);
		return -1;
	}

	auto hwnd = CreateWindowEx(
		0,
		VCEKC_CLASSNAME,
		VCEKC_WINDOWNAME,
		0,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (hwnd == NULL)
	{
		UnregisterClass(VCEKC_CLASSNAME, hInstance);
		MessageBox(NULL, _T("CreateWindowEx() failed"), VCEKC_MSGTITLE, MB_ICONERROR | MB_OK);
		return -1;
	}

	// Make sure to receive session notifications
	if (!WTSRegisterSessionNotification(hwnd, NOTIFY_FOR_ALL_SESSIONS))
	{
		DestroyWindow(hwnd);
		UnregisterClass(VCEKC_CLASSNAME, hInstance);
		MessageBox(NULL, _T("WTSRegisterSessionNotification() failed"), VCEKC_MSGTITLE, MB_ICONERROR | MB_OK);
		return -1;
	}
	
	// Add shell icon
	nid.cbSize = sizeof(nid);
	nid.uCallbackMessage = VCEKC_SHELLICONMSG;
	nid.hWnd = hwnd;
	nid.uID = 15;
	nid.uVersion = NOTIFYICON_VERSION;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
	nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	StringCchCopy(nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]), VCEKC_TOOLTIPMSG);

	BOOL IconCreated = FALSE;
	int IconCreateTries = 0;

	do
	{
		IconCreated = Shell_NotifyIcon(NIM_ADD, &nid);
		Sleep(500); // xd
	} while (!IconCreated && ++IconCreateTries < 120);
	// If we didn't manage to create the icon, fuck it. better to run anyway

	// Message loop
	MSG msg{};
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	DestroyWindow(hwnd);
	UnregisterClass(VCEKC_CLASSNAME, hInstance);
	
	return 0;
}
