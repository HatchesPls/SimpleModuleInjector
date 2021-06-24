#include <Windows.h>
#include <d3d11.h>
#include <shlwapi.h>
#include <tchar.h>
#include <sstream>
#include <iostream>
#include <filesystem>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "Shlwapi.lib")
#include <Tlhelp32.h>
#include "ThirdParty/ImGui/imgui.h"
#include "ThirdParty/ImGui/imgui_impl_win32.h"
#include "ThirdParty/ImGui/imgui_impl_dx11.h"


//Definitions
std::string SMI_BUILD                                       = "1.0.0.2";
std::string MainWindowTitle                                 = "SMI - " + SMI_BUILD + " - https://github.com/HowYouDoinMate/SimpleModuleInjector";
static      ID3D11Device* g_pd3dDevice                      = NULL;
static      IDXGISwapChain* g_pSwapChain                    = NULL;
static      ID3D11DeviceContext* g_pd3dDeviceContext        = NULL;
static      ID3D11RenderTargetView* g_mainRenderTargetView  = NULL;
char*       SelectedModuleFile                              = NULL;
char        TargetProcessNameBufferInput                    [51];
std::string PopupNotificationMessage                        = "";
HWND        MainWindowHandle;
LRESULT     WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool        CreateDirectXDeviceAndSwapChain(HWND hWnd);
void        CleanupDirectXDeviceAndSwapChain();
void        CreateRenderTarget();
void        CleanupRenderTarget();
char*       ShowSelectFileDialogAndReturnPath();
DWORD       GetProcessIDByName(const std::wstring& ProcessName);
bool        FileExists(const std::string& fileName);
void        InjectModule(std::string ModulePath, std::wstring ProcessName, bool CMD = false);


int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR pCmdLine, _In_ int nShowCmd)
{
    LPWSTR* ArgumentList;
    int ArgumentCount;
    ArgumentList = CommandLineToArgvW(GetCommandLine(), &ArgumentCount);
    if (ArgumentList == NULL)
    {
        MessageBoxA(NULL, "CommandLineToArgvW Failed", NULL, MB_OK | MB_ICONERROR);
        return EXIT_SUCCESS;
    }
    if (ArgumentCount == 3)
    {
        std::wstring ModulePath_LPWSTRToWSTR(ArgumentList[1]);
        std::string ModulePathString = std::string(ModulePath_LPWSTRToWSTR.begin(), ModulePath_LPWSTRToWSTR.end());
        InjectModule(ModulePathString, ArgumentList[2], true);
    }
    LocalFree(ArgumentList);


    WNDCLASSEX WindowClass = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("SMI_MainWindow"), NULL };
    RegisterClassEx(&WindowClass);

    std::wstring MainWindowTitleWS(MainWindowTitle.begin(), MainWindowTitle.end());
    MainWindowHandle = CreateWindow(WindowClass.lpszClassName, MainWindowTitleWS.c_str(), WS_CAPTION | WS_SYSMENU, 100, 100, 500, 150, NULL, NULL, WindowClass.hInstance, NULL);

    if (!CreateDirectXDeviceAndSwapChain(MainWindowHandle))
    {
        MessageBoxA(MainWindowHandle, "Failed to create Direct3D device", NULL, MB_OK | MB_ICONERROR);
        CleanupDirectXDeviceAndSwapChain();
        UnregisterClass(WindowClass.lpszClassName, WindowClass.hInstance);
        return 1;
    }

    //Show Window
    ShowWindow(MainWindowHandle, SW_NORMAL);
    UpdateWindow(MainWindowHandle);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    ImGui_ImplWin32_Init(MainWindowHandle);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\ARLRDBD.ttf", 22.f);

    MSG Message;
    ZeroMemory(&Message, sizeof(Message));
    while (Message.message != WM_QUIT)
    {
        if (PeekMessage(&Message, NULL, 0U, 0U, PM_REMOVE)) { TranslateMessage(&Message); DispatchMessage(&Message); continue; }

        //Start ImGui Frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
       
        //Push Styles
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(ImColor(30, 30, 30, 255)));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(ImColor(0, 0, 255, 255)));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImColor(0, 0, 205, 255)));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(ImColor(0, 0, 205, 255)));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(ImColor(50, 50, 50, 255)));
    
        //Show Popup Notification Loop
        if (PopupNotificationMessage != "")
        {
            ImGui::OpenPopup("PopupNotification");
            if (ImGui::BeginPopupModal("PopupNotification", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar))
            {
                ImGui::TextWrapped((char*)PopupNotificationMessage.c_str());
                if (ImGui::Button("OK", ImVec2(400, 0))) { ImGui::CloseCurrentPopup(); PopupNotificationMessage = ""; }
            }
        }

        //ImGui Window Setup
        ImGui::SetNextWindowSize(ImVec2(500, 150));
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::Begin("##MainWindow", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
 
        //Module Select Section
        ImGui::Text("Module Name:");
        ImGui::SameLine();
        if (SelectedModuleFile != NULL) { ImGui::TextWrapped(PathFindFileNameA(SelectedModuleFile)); }
        ImGui::SameLine(ImGui::GetWindowWidth() - 200);
        if (ImGui::SmallButton("Select Module File")) { SelectedModuleFile = ShowSelectFileDialogAndReturnPath(); }

        //Process Name Input Section
        ImGui::Text("Process Name/ID:");
        ImGui::SameLine();
        ImGui::InputText("##ProcessNameInput", TargetProcessNameBufferInput, IM_ARRAYSIZE(TargetProcessNameBufferInput), ImGuiInputTextFlags_CharsNoBlank);
        ImGui::Dummy(ImVec2(0, 5));
        if (ImGui::Button("Inject", ImVec2(465, 0)))
        {
            if (!SelectedModuleFile)
            {
                PopupNotificationMessage = "You must select a module to inject";
            }
            else
            {
                std::string ProcessNameString = TargetProcessNameBufferInput;
                std::wstring ProcessNameWS(ProcessNameString.begin(), ProcessNameString.end());
                InjectModule(SelectedModuleFile, ProcessNameWS);
            }
        }
       

        ImGui::End();
        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        static UINT presentFlags = 0;
        if (g_pSwapChain->Present(1, presentFlags) == DXGI_STATUS_OCCLUDED) 
        {
            presentFlags = DXGI_PRESENT_TEST;
            Sleep(50);
        }
        else 
        {
            presentFlags = 0;
        }
    }


    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDirectXDeviceAndSwapChain();
    DestroyWindow(MainWindowHandle);
    UnregisterClass(WindowClass.lpszClassName, WindowClass.hInstance);
    return 0;
}


bool CreateDirectXDeviceAndSwapChain(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL FeatureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, FeatureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
    {
        return false;
    }

    CreateRenderTarget();
    return true;
}

void CleanupDirectXDeviceAndSwapChain()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) { return true; }

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}



char* ShowSelectFileDialogAndReturnPath()
{
    LPSTR filebuff = new char[256];
    OPENFILENAMEA open = { 0 };
    open.lStructSize = sizeof(OPENFILENAMEA);
    open.hwndOwner = MainWindowHandle;
    open.lpstrFilter = NULL;
    open.lpstrCustomFilter = NULL;
    open.lpstrFile = filebuff;
    open.lpstrFile[0] = '\0';
    open.nMaxFile = MAX_PATH;
    open.nFilterIndex = 1;
    open.lpstrInitialDir = NULL;
    open.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;

    if (GetOpenFileNameA(&open))
    {
        delete[] filebuff;
        return open.lpstrFile;
    }
    else
    {
        return NULL;
    }
}
DWORD GetProcessIDByName(const std::wstring& ProcessName)
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
bool FileExists(const std::string& fileName)
{
    struct stat buffer;
    return (stat(fileName.c_str(), &buffer) == 0);
}
void InjectModule(std::string ModulePath, std::wstring ProcessName, bool CMD)
{
    //Get Process ID from ProcessName
    DWORD processID = GetProcessIDByName(ProcessName);
    if (!processID) 
    { 
        if (CMD)
        {
            //MessageBoxA(NULL, "No Process ID with that Process Name exists", NULL, MB_OK | MB_ICONERROR);
            std::cout << "[!] Invalid process name" << std::endl;
        }
        else
        {
            PopupNotificationMessage = "No Process ID with that Process Name exists";
        }
        return;    
    } 
    if (!FileExists(ModulePath))  
    { 
        if (CMD)
        {
            MessageBoxA(NULL, "Target Module Does Not Exist", NULL, MB_OK | MB_ICONERROR);
        }
        else
        {
            PopupNotificationMessage = "Selected Module Does Not Exist Anymore";
        }
        return; 
    }

    //Get a handle to the process
    HANDLE Process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (!Process) 
    { 
        if (CMD)
        {
            MessageBoxA(NULL, "OpenProcess Failed. Try to run SMI elevated?", NULL, MB_OK | MB_ICONERROR);
        }
        else
        {
            PopupNotificationMessage = "OpenProcess Failed. Try to run SMI elevated?";
        }
        return;   
    }

    //Allocate Memory Space In Target Process
    LPVOID Memory = LPVOID(VirtualAllocEx(Process, nullptr, MAX_PATH, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    if (!Memory) 
    { 
        if (CMD)
        {
            MessageBoxA(NULL, "VirtualAllocEx Failed", NULL, MB_OK | MB_ICONERROR);
        }
        else
        {
            PopupNotificationMessage = "VirtualAllocEx Failed";
        }
        return; 
    }

    //Write Module Name To Target Process
    if (!WriteProcessMemory(Process, Memory, ModulePath.c_str(), MAX_PATH, nullptr)) 
    { 
        if (CMD)
        {
            MessageBoxA(NULL, "WriteProcessMemory Failed", NULL, MB_OK | MB_ICONERROR);
        }
        else
        {
            PopupNotificationMessage = "WriteProcessMemory Failed";
        }
        return; 
    }

    //Start Execution Of Injected Module In Target Process
    HANDLE RemoteThreadHandle = CreateRemoteThread(Process, nullptr, NULL, LPTHREAD_START_ROUTINE(LoadLibraryA), Memory, NULL, nullptr);
    if (!RemoteThreadHandle) 
    { 
        if (CMD)
        {
            MessageBoxA(NULL, "CreateRemoteThread Failed", NULL, MB_OK | MB_ICONERROR);
        }
        else
        {
            PopupNotificationMessage = "CreateRemoteThread Failed";
        }       
        return; 
    }

    //Close Target Process Handle
    CloseHandle(Process);

    //Free the allocated memory.
    VirtualFreeEx(Process, LPVOID(Memory), 0, MEM_RELEASE);

    if (!CMD)
    {
        PopupNotificationMessage = "Module Successfully Injected In Target Process";
    }
    else
    {
        std::exit(EXIT_SUCCESS);
    }
}