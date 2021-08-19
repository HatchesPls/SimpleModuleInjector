#include "Header/Main.h"

ID3D11Device* Injector::UI::g_pd3dDevice = NULL;
IDXGISwapChain* Injector::UI::g_pSwapChain = NULL;
ID3D11DeviceContext* Injector::UI::g_pd3dDeviceContext = NULL;
ID3D11RenderTargetView* Injector::UI::g_mainRenderTargetView = NULL;
std::string Injector::UI::PopupNotificationMessage;

bool Injector::UI::CreateDirectXDeviceAndSwapChain(HWND hWnd)
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
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, FeatureLevelArray, 2, D3D11_SDK_VERSION, &sd, &Injector::UI::g_pSwapChain, &Injector::UI::g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
    {
        return false;
    }
    CreateRenderTarget();
    return true;
}

void Injector::UI::CleanupDirectXDeviceAndSwapChain()
{
    CleanupRenderTarget();
    if (Injector::UI::g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (Injector::UI::g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (Injector::UI::g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void Injector::UI::CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    Injector::UI::g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer)
    {
        Injector::UI::g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &Injector::UI::g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

void Injector::UI::CleanupRenderTarget()
{
    if (Injector::UI::g_mainRenderTargetView) 
    { 
        Injector::UI::g_mainRenderTargetView->Release(); 
        Injector::UI::g_mainRenderTargetView = NULL; 
    }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI Injector::UI::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) { return true; }

    switch (msg)
    {
    case WM_SIZE:
        if (Injector::UI::g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            Injector::UI::CleanupRenderTarget();
            Injector::UI::g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            Injector::UI::CreateRenderTarget();
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(EXIT_SUCCESS);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

char* Injector::UI::ShowSelectFileDialogAndReturnPath()
{
    LPSTR FileBuffer = new char[MAX_PATH];
    OPENFILENAMEA open = { 0 };
    open.lStructSize = sizeof(OPENFILENAMEA);
    open.hwndOwner = Injector::UI::MainWindowHandle;
    open.lpstrFilter = NULL;
    open.lpstrCustomFilter = NULL;
    open.lpstrFile = FileBuffer;
    open.lpstrFile[0] = '\0';
    open.nMaxFile = MAX_PATH;
    open.nFilterIndex = 1;
    open.lpstrInitialDir = NULL;
    open.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;

    if (GetOpenFileNameA(&open))
    {
        return open.lpstrFile;
    }
    else
    {
        return NULL;
    }
}