#include "Header/Main.h"

bool Injector::InjectorFunctions::FileExists(const std::string& fileName)
{
    if (std::filesystem::exists(fileName))
    {
        return true;
    }
    return false;
}

void Injector::InjectorFunctions::InjectModule(std::string ModulePath, std::wstring ProcessName, int ProcessID)
{
    DWORD TargetProcessID = ProcessName.empty() ? ProcessID : GetProcessIDByName(ProcessName);
    if (!TargetProcessID)
    {
        Injector::UI::PopupNotificationMessage = "Invalid Process Name";
        return;
    }
    if (!FileExists(ModulePath))
    {
        Injector::UI::PopupNotificationMessage = "Selected Module Does Not Exist Anymore";
        return;
    }

    //Get a handle to the process
    HANDLE Process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, TargetProcessID);
    if (!Process)
    {
        if (GetLastError() == ERROR_INVALID_PARAMETER)
        {
            Injector::UI::PopupNotificationMessage = "Invalid Process ID";
        }
        else
        {
            Injector::UI::PopupNotificationMessage = "OpenProcess() Failed. Try to run SMI elevated?";
        }
        return;
    }

    //Allocate Memory Space In Target Process
    LPVOID Memory = LPVOID(VirtualAllocEx(Process, nullptr, MAX_PATH, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    if (!Memory)
    {
        Injector::UI::PopupNotificationMessage = "VirtualAllocEx() Failed";
        return;
    }

    //Write Module Name To Target Process
    if (!WriteProcessMemory(Process, Memory, ModulePath.c_str(), MAX_PATH, nullptr))
    {
        Injector::UI::PopupNotificationMessage = "WriteProcessMemory() Failed";
        return;
    }

    //Start Execution Of Injected Module In Target Process
    HANDLE RemoteThreadHandle = CreateRemoteThread(Process, nullptr, NULL, LPTHREAD_START_ROUTINE(LoadLibraryA), Memory, NULL, nullptr);
    if (!RemoteThreadHandle)
    {
        Injector::UI::PopupNotificationMessage = "CreateRemoteThread() Failed";
        return;
    }

    //Close Handles
    CloseHandle(RemoteThreadHandle);
    CloseHandle(Process);

    Injector::UI::PopupNotificationMessage = "Module Successfully Injected";
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