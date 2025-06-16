#pragma once
#include <Windows.h>
#include <string>

class CInstaller
{
private:

	std::wstring m_szExePath;
	std::wstring m_szTargetPath;

	std::wstring GetTargetExecutable();

	HRESULT GetTaskFolderAndService(void** ppTaskFolder, void** ppTaskService);

	DWORD UninstallFiles();

	DWORD UninstallTask();

public:

	std::wstring& GetTargetPath(bool Create = false);

	DWORD Install();

	DWORD Uninstall();

	CInstaller(const wchar_t* szExePath);

};