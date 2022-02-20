#include "Header/Main.h"

bool Injector::InjectorFunctions::FileOrDirectoryExists(const std::string& fileName)
{
    if (std::filesystem::exists(fileName))
    {
        return true;
    }
    return false;
}

void Injector::InjectorFunctions::InjectModule(std::string ModulePath, std::wstring ProcessName, int ProcessID)
{
    if (!FileOrDirectoryExists(UI::SelectedModuleFile))
    {
        UI::SelectedModuleFile = NULL;
        UI::PopupNotificationMessage = "Selected module was deleted";
        return;
    }

    DWORD TargetProcessID = ProcessName.empty() ? ProcessID : GetProcessIDByName(ProcessName);
    if (!TargetProcessID)
    {
        Injector::UI::PopupNotificationMessage = "Invalid Process Name";
        return;
    }
  
    HANDLE Process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, TargetProcessID);
    if (!Process)
    {
        if (GetLastError() == ERROR_INVALID_PARAMETER)
        {
            Injector::UI::PopupNotificationMessage = "Process ID does not exist";
        }
        else
        {
            Injector::UI::PopupNotificationMessage = "OpenProcess() Failed. Try to run SMI elevated?";
        }
        return;
    }

    LPVOID Memory = LPVOID(VirtualAllocEx(Process, NULL, size(ModulePath), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    if (!Memory)
    {
        Injector::UI::PopupNotificationMessage = "VirtualAllocEx() Failed";
        return;
    }

    if (!WriteProcessMemory(Process, Memory, ModulePath.c_str(), size(ModulePath), NULL))
    {
        Injector::UI::PopupNotificationMessage = "WriteProcessMemory() Failed";
        return;
    }

    HANDLE RemoteThreadHandle = CreateRemoteThread(Process, NULL, NULL, LPTHREAD_START_ROUTINE(LoadLibraryA), Memory, NULL, NULL);
    if (!RemoteThreadHandle)
    {
        Injector::UI::PopupNotificationMessage = "CreateRemoteThread() Failed";
        return;
    }
    WaitForSingleObject(RemoteThreadHandle, INFINITE);
    DWORD RThreadExitCode = 0;
    GetExitCodeThread(RemoteThreadHandle, &RThreadExitCode);
    CloseHandle(RemoteThreadHandle);

    if (!VirtualFreeEx(Process, Memory, NULL, MEM_RELEASE))
    {
        Injector::UI::PopupNotificationMessage = "VirtualFreeEx() Failed";
        return;
    }
    CloseHandle(Process);

    if (!RThreadExitCode)
    {
        Injector::UI::PopupNotificationMessage = "Injecting Module Failed";
    }
    else
    {
        Injector::UI::PopupNotificationMessage = "Module Successfully Injected";
    }
}

DWORD Injector::InjectorFunctions::GetProcessIDByName(const std::wstring& ProcessName)
{
    PROCESSENTRY32 processInfo;
    processInfo.dwSize = sizeof(processInfo);

    HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (processesSnapshot == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    Process32First(processesSnapshot, &processInfo);
    if (!ProcessName.compare(processInfo.szExeFile))
    {
        CloseHandle(processesSnapshot);
        return processInfo.th32ProcessID;
    }

    while (Process32Next(processesSnapshot, &processInfo))
    {
        if (!ProcessName.compare(processInfo.szExeFile))
        {
            CloseHandle(processesSnapshot);
            return processInfo.th32ProcessID;
        }
    }
    CloseHandle(processesSnapshot);
    return 0;
}

bool Injector::InjectorFunctions::FileHasDOSSignature(char* TargetFilePath)
{
    HANDLE hFile = CreateFileA(TargetFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) 
    { 
        HANDLE hMapObject = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        if (hMapObject)
        {
            LPVOID lpBase = MapViewOfFile(hMapObject, FILE_MAP_READ, 0, 0, 0);
            PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)lpBase;

            if (dosHeader)
            {
                CloseHandle(hFile);
                CloseHandle(hMapObject);
                if (dosHeader->e_magic == IMAGE_DOS_SIGNATURE)
                {  
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }

    }
    return false;
}