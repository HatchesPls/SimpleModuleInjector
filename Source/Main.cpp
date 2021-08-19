#include "Header/Main.h"

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR pCmdLine, _In_ int nShowCmd)
{
    WNDCLASSEX WindowClass = { sizeof(WNDCLASSEX), CS_CLASSDC, Injector::UI::WndProc, 0L, 0L, GetModuleHandle(NULL), LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)), NULL, NULL, NULL, _T("SMI_MainWindow"), NULL };
    RegisterClassEx(&WindowClass);

    Injector::UI::MainWindowHandle = CreateWindow(WindowClass.lpszClassName, L"Simple Module Injector", WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 100, 100, 500, 188, NULL, NULL, WindowClass.hInstance, NULL);

    if (!Injector::UI::CreateDirectXDeviceAndSwapChain(Injector::UI::MainWindowHandle))
    {
        MessageBoxA(Injector::UI::MainWindowHandle, "Failed to create Direct3D device", NULL, MB_OK | MB_ICONERROR);
        Injector::UI::CleanupDirectXDeviceAndSwapChain();
        UnregisterClass(WindowClass.lpszClassName, WindowClass.hInstance);
        return EXIT_FAILURE;
    }

    //Show Window
    ShowWindow(Injector::UI::MainWindowHandle, SW_NORMAL);

    //Initialize imGui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    ImGui_ImplWin32_Init(Injector::UI::MainWindowHandle);
    ImGui_ImplDX11_Init(Injector::UI::g_pd3dDevice, Injector::UI::g_pd3dDeviceContext);
    io.Fonts->AddFontFromFileTTF(strcat(getenv("SystemDrive"), "\\Windows\\Fonts\\Verdana.ttf"), 23.f);

    //Window Loop
    MSG Message = { 0 };
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
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(ImColor(0, 0, 205, 255)));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(ImColor(0, 0, 0, 255)));

        //Show Popup Notification Loop
        if (!Injector::UI::PopupNotificationMessage.empty())
        {
            ImGui::OpenPopup("###PopupNotification");
            if (ImGui::BeginPopupModal("###PopupNotification", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar))
            {
                ImGui::TextWrapped(Injector::UI::PopupNotificationMessage.c_str());
                if (ImGui::Button("OK", ImVec2(400, 0))) 
                {
                    Injector::UI::PopupNotificationMessage.clear();
                    ImGui::CloseCurrentPopup();
                }
            }
        }

        //ImGui Window Setup
        ImGui::SetNextWindowSize(ImVec2(500, 149));
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::Begin("MainWindow", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
 
        //Module Select Section
        ImGui::Text("Module Name:");
        ImGui::SameLine();
        if (Injector::UI::SelectedModuleFile != NULL) { ImGui::TextWrapped(PathFindFileNameA(Injector::UI::SelectedModuleFile)); }
        ImGui::SameLine(ImGui::GetWindowWidth() - 160);
        if (ImGui::SmallButton("Select Module")) 
        {
            char* SelectedFile = Injector::UI::ShowSelectFileDialogAndReturnPath();
            if (SelectedFile != NULL)
            {
                if (Injector::InjectorFunctions::FileHasDOSSignature(SelectedFile))
                {
                    Injector::UI::SelectedModuleFile = SelectedFile;
                }
                else
                {
                    Injector::UI::PopupNotificationMessage = "Selected file is not an injectable & valid NT executable";
                }
            }
        }

        //Process Name Input Section
        ImGui::Text("Process Name/ID:");
        ImGui::SameLine();
        ImGui::PushItemWidth(292);
        ImGui::InputText("##ProcessNameOrIDInput", Injector::UI::TargetProcessNameOrIDBufferInput, IM_ARRAYSIZE(Injector::UI::TargetProcessNameOrIDBufferInput), ImGuiInputTextFlags_CharsNoBlank);
        ImGui::Dummy(ImVec2(0, 5));
        if (ImGui::Button("Inject module", ImVec2(470, 35)))
        {
            if (!Injector::UI::SelectedModuleFile)
            {
               Injector::UI::PopupNotificationMessage = "You must select a module to inject";
            }
            else
            {
                std::string TargetProcessNameString(Injector::UI::TargetProcessNameOrIDBufferInput);
                if (!TargetProcessNameString.empty())
                {
                    if (std::all_of(TargetProcessNameString.begin(), TargetProcessNameString.end(), isdigit))
                    {
                        Injector::InjectorFunctions::InjectModule(Injector::UI::SelectedModuleFile, L"", std::stoi(TargetProcessNameString));
                    }
                    else
                    {
                        std::wstring ProcessNameWS(TargetProcessNameString.begin(), TargetProcessNameString.end());
                        Injector::InjectorFunctions::InjectModule(Injector::UI::SelectedModuleFile, ProcessNameWS, NULL);
                    }
                }
                else
                {
                    Injector::UI::PopupNotificationMessage = "You must provide a process name/id";
                }
            }
        }
        std::string VersionFooter = "v" + SMI_BUILD;
        ImGui::Text(VersionFooter.c_str());
        ImGui::SameLine(ImGui::GetWindowWidth() - 85);
        if (ImGui::Button("About"))
        {
            ImGui::OpenPopup("About SMI###AboutPopup");
        }
        if (ImGui::BeginPopupModal("About SMI###AboutPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar))
        {
            std::string AuthorText = "Author: HatchesPls\nCompiled: " + (std::string)__DATE__ + " " + (std::string)__TIME__ + "\nGithub: https://git.io/J0HCq";
            ImGui::TextWrapped(AuthorText.c_str());
            if (ImGui::Button("Close", ImVec2(500, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::End();
        ImGui::Render();
        Injector::UI::g_pd3dDeviceContext->OMSetRenderTargets(1, &Injector::UI::g_mainRenderTargetView, NULL);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        static UINT presentFlags = 0;
        if (Injector::UI::g_pSwapChain->Present(1, presentFlags) == DXGI_STATUS_OCCLUDED)
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
    Injector::UI::CleanupDirectXDeviceAndSwapChain();
    DestroyWindow(Injector::UI::MainWindowHandle);
    UnregisterClass(WindowClass.lpszClassName, WindowClass.hInstance);
    return EXIT_SUCCESS;
}