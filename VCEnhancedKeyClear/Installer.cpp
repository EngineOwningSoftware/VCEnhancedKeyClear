#include "Installer.h"
#include "defines.h"
#include <ShlObj.h>
#include <AclAPI.h>
#include <taskschd.h>

std::wstring CInstaller::GetTargetExecutable()
{
	auto& TargetPath = GetTargetPath(true);

	if (TargetPath.empty())
	{
		return L"";
	}

	return TargetPath + L"\\" VCEKC_EXENAME;
}

HRESULT CInstaller::GetTaskFolderAndService(void** ppTaskFolder, void** ppTaskService)
{
    // https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/TaskSchd/time-trigger-example--c---.md
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (FAILED(hr))
    {
        return hr;
    }

    hr = CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        0,
        NULL);

    if (FAILED(hr))
    {
        CoUninitialize();
        return hr;
    }


    //  ------------------------------------------------------
    //  Create an instance of the Task Service. 
    ITaskService* pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);

    if (FAILED(hr))
    {
        CoUninitialize();
        return hr;
    }

    //  Connect to the task service.
    hr = pService->Connect(VARIANT(), VARIANT(), VARIANT(), VARIANT());

    if (FAILED(hr))
    {
        pService->Release();
        CoUninitialize();
        return hr;
    }

    //  ------------------------------------------------------
    //  ------------------------------------------------------
    //  Get the pointer to the root task folder.  This folder will hold the
    //  new task that is registered.
    ITaskFolder* pRootFolder = NULL;
    hr = pService->GetFolder(L"\\", &pRootFolder);
    if (FAILED(hr))
    {
        pService->Release();
        CoUninitialize();
        return hr;
    }

    *ppTaskFolder = pRootFolder;
    *ppTaskService = pService;

    return hr;
}

DWORD CInstaller::UninstallFiles()
{
    if ((GetFileAttributesW(GetTargetPath().c_str()) & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
        return ERROR_PATH_NOT_FOUND;
    }

    auto TargetExecutable = GetTargetExecutable();
    DWORD dwError = ERROR_SUCCESS;

    if (!DeleteFileW(TargetExecutable.c_str()))
    {
        dwError = GetLastError();
    }

    auto& TargetDir = GetTargetPath();

    if (!RemoveDirectoryW(TargetDir.c_str()))
    {
        dwError = GetLastError();
    }

    return dwError;
}

DWORD CInstaller::UninstallTask()
{
    ITaskFolder* pRootFolder = NULL;
    ITaskService* pService = NULL;
    HRESULT hr = GetTaskFolderAndService(reinterpret_cast<void**>(&pRootFolder), reinterpret_cast<void**>(&pService));

    if (FAILED(hr))
    {
        return HRESULT_CODE(hr);
    }

    hr = pRootFolder->DeleteTask(VCEKC_TASKNAME, 0);

    pRootFolder->Release();
    pService->Release();

    return HRESULT_CODE(hr);
}

std::wstring& CInstaller::GetTargetPath(bool Create /* = false */)
{
	if (m_szTargetPath.empty())
	{
		wchar_t TargetPath[MAX_PATH]{};

		if (SHGetFolderPathAndSubDir(NULL, CSIDL_PROGRAM_FILES | (Create ? CSIDL_FLAG_CREATE : CSIDL_FLAG_DONT_VERIFY), NULL, SHGFP_TYPE_CURRENT, VCEKC_INSTALLPATH, TargetPath) == S_OK)
		{
			m_szTargetPath = TargetPath;
		}
	}

	return m_szTargetPath;
}

DWORD CInstaller::Install()
{
	// Resolve target executable path
	auto TargetExecutable = GetTargetExecutable();

	if (TargetExecutable.empty())
	{
		return ERROR_PATH_NOT_FOUND;
	}

	// Copy current executable to target path
	if (!CopyFileExW(m_szExePath.c_str(), TargetExecutable.c_str(), nullptr, nullptr, nullptr, 0))
	{
		return GetLastError();
	}

	// Create scheduled task
    ITaskFolder* pRootFolder = NULL;
    ITaskService* pService = NULL;
    HRESULT hr = GetTaskFolderAndService(reinterpret_cast<void**>(&pRootFolder), reinterpret_cast<void**>(&pService));

    if (FAILED(hr))
    {
        return HRESULT_CODE(hr);
    }

    //  If the same task exists, remove it.
    pRootFolder->DeleteTask(VCEKC_TASKNAME, 0);

    //  Create the task definition object to create the task.
    ITaskDefinition* pTask = NULL;
    hr = pService->NewTask(0, &pTask);
    
    pService->Release();  // COM clean up.  Pointer is no longer used.
    if (FAILED(hr))
    {
        pRootFolder->Release();
        CoUninitialize();
        return HRESULT_CODE(hr);
    }

    //  ------------------------------------------------------
    //  Get the registration info for setting the identification.
    IRegistrationInfo* pRegInfo = NULL;
    hr = pTask->get_RegistrationInfo(&pRegInfo);
    if (FAILED(hr))
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return HRESULT_CODE(hr);
    }

    hr = pRegInfo->put_Author(VCEKC_TASKNAME);
    pRegInfo->Release();

    if (FAILED(hr))
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return HRESULT_CODE(hr);
    }

    //  ------------------------------------------------------
    //  Create the principal for the task - these credentials
    //  are overwritten with the credentials passed to RegisterTaskDefinition
    IPrincipal* pPrincipal = NULL;
    hr = pTask->get_Principal(&pPrincipal);
    if (FAILED(hr))
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return HRESULT_CODE(hr);
    }

    //  Set up principal logon type to interactive logon
    hr = pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
    if (FAILED(hr))
    {
        pPrincipal->Release();
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return HRESULT_CODE(hr);
    }

    //  Set up principal logon type to interactive logon
    hr = pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
    pPrincipal->Release();
    if (FAILED(hr))
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return HRESULT_CODE(hr);
    }

    //  ------------------------------------------------------
    //  Create the settings for the task
    ITaskSettings* pSettings = NULL;
    hr = pTask->get_Settings(&pSettings);
    if (FAILED(hr))
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return HRESULT_CODE(hr);
    }

    //  Set setting values for the task.  
    hr = pSettings->put_StartWhenAvailable(VARIANT_TRUE);
    pSettings->Release();
    if (FAILED(hr))
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return HRESULT_CODE(hr);
    }

    // Set the idle settings for the task.
    IIdleSettings* pIdleSettings = NULL;
    hr = pSettings->get_IdleSettings(&pIdleSettings);
    if (FAILED(hr))
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return HRESULT_CODE(hr);
    }

    hr = pIdleSettings->put_WaitTimeout(L"PT5M");
    pIdleSettings->Release();
    if (FAILED(hr))
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return HRESULT_CODE(hr);
    }


    //  ------------------------------------------------------
    //  Get the trigger collection to insert the time trigger.
    ITriggerCollection* pTriggerCollection = NULL;
    hr = pTask->get_Triggers(&pTriggerCollection);
    if (FAILED(hr))
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return HRESULT_CODE(hr);
    }

    //  Add the time trigger to the task.
    ITrigger* pTrigger = NULL;
    hr = pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
    pTriggerCollection->Release();
    if (FAILED(hr))
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return HRESULT_CODE(hr);
    }
    
    ILogonTrigger* pLogonTrigger = NULL;
    hr = pTrigger->QueryInterface(IID_ILogonTrigger, (void**)&pLogonTrigger);
    pTrigger->Release();
    
    if (FAILED(hr))
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return HRESULT_CODE(hr);
    }

    pLogonTrigger->put_Id(L"LogonTrigger");
    pLogonTrigger->put_Enabled(VARIANT_TRUE);

    //  ------------------------------------------------------
    //  Add an action to the task. This task will execute notepad.exe.     
    IActionCollection* pActionCollection = NULL;
    
    //  Get the task action collection pointer.
    hr = pTask->get_Actions(&pActionCollection);
    if (FAILED(hr))
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return HRESULT_CODE(hr);
    }

    //  Create the action, specifying that it is an executable action.
    IAction* pAction = NULL;
    hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
    pActionCollection->Release();
    if (FAILED(hr))
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return HRESULT_CODE(hr);
    }

    IExecAction* pExecAction = NULL;
    //  QI for the executable task pointer.
    hr = pAction->QueryInterface(
        IID_IExecAction, (void**)&pExecAction);
    pAction->Release();
    if (FAILED(hr))
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return HRESULT_CODE(hr);
    }

    //  Set the path of the executable to notepad.exe.
    hr = pExecAction->put_Path(BSTR(TargetExecutable.c_str()));
    pExecAction->put_Arguments(L"--autorun");
    pExecAction->Release();
    if (FAILED(hr))
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return HRESULT_CODE(hr);
    }

    //  ------------------------------------------------------
    //  Save the task in the root folder.
    IRegisteredTask* pRegisteredTask = NULL;
    hr = pRootFolder->RegisterTaskDefinition(
        VCEKC_TASKNAME,
        pTask,
        TASK_CREATE_OR_UPDATE,
        VARIANT(),
        VARIANT(),
        TASK_LOGON_INTERACTIVE_TOKEN,
        VARIANT(),
        &pRegisteredTask);
    if (FAILED(hr))
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return HRESULT_CODE(hr);
    }

    // Run the task
    IRunningTask* pRunningTask = NULL;
    hr = pRegisteredTask->Run(VARIANT(), &pRunningTask);

    if (!FAILED(hr))
    {
        pRunningTask->Release();
    }

    //  Clean up.
    pRootFolder->Release();
    pTask->Release();
    pRegisteredTask->Release();
    CoUninitialize();

    if (FAILED(hr))
        return HRESULT_CODE(hr);
    
    return ERROR_SUCCESS;
}

DWORD CInstaller::Uninstall()
{
    // Attempt to uninstall everything even if one step fails
    auto dwErr1 = UninstallFiles();
    auto dwErr2 = UninstallTask();

    if (dwErr1 != ERROR_SUCCESS)
        return dwErr1;

    return dwErr2;
}

CInstaller::CInstaller(const wchar_t* szExePath) : m_szExePath{ szExePath }
{
}
