#pragma once
#include <tchar.h>

#pragma comment(lib, "Wtsapi32.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")
#pragma comment(lib, "credui.lib")

// VeraCrypt defines
#define TC_SYSTEM_FAVORITES_SERVICE_NAME _T("VeraCryptSystemFavorites")
#define VC_DRIVER_CONFIG_CLEAR_KEYS_ON_NEW_DEVICE_INSERTION	0x400
#define VC_SERVICE_CONTROL_BUILD_DEVICE_LIST 128

// VCEKC defines
#define VCEKC_CLASSNAME _T("VCEnhancedKeyClear_WndClass")
#define VCEKC_WINDOWNAME _T("VCEnhancedKeyClear_Wnd")
#define VCEKC_MSGTITLE _T("VeraCrypt Enhanced Key Clear")
#define VCEKC_TOOLTIPMSG _T("VeraCrypt Enhanced Key Clear is active. Right-click to exit.")
#define VCEKC_MUTEX _T("VCEnhancedKeyClear_Mutex")
#define VCEKC_INSTALLPATH _T("VeraCryptEnhancedKeyClear")
#define VCEKC_EXENAME _T("VCEnhancedKeyClear.exe")
#define VCEKC_TASKNAME _T("VeraCrypt Enhanced Key Clear")
#define VCEKC_SHELLICONMSG 0xBEFC
