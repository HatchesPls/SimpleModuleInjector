#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <shlwapi.h>
#include <tchar.h>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <Tlhelp32.h>
#include <io.h>
#include <fcntl.h>
#include "ThirdParty/ImGui/imgui.h"
#include "ThirdParty/ImGui/imgui_impl_win32.h"
#include "ThirdParty/ImGui/imgui_impl_dx11.h"
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "Shlwapi.lib")
#include "resource.h"

static const std::string SMI_BUILD = "1.0.0.3";

namespace Injector
{
	namespace InjectorFunctions
	{
		void Loop();
		DWORD GetProcessIDByName(const std::wstring& ProcessName);
		bool FileOrDirectoryExists(const std::string& fileName);
		void InjectModule(std::string ModulePath, std::wstring ProcessName, int ProcessID);
		bool FileHasDOSSignature(char* TargetFilePath);
	}
	namespace UI
	{
		extern ID3D11Device* g_pd3dDevice;
		extern IDXGISwapChain* g_pSwapChain;
		extern ID3D11DeviceContext* g_pd3dDeviceContext;
		extern ID3D11RenderTargetView* g_mainRenderTargetView;
		extern char* SelectedModuleFile;
		static char TargetProcessNameOrIDBufferInput[51];
		extern std::string PopupNotificationMessage;
		static HWND MainWindowHandle;
		LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		bool CreateDirectXDeviceAndSwapChain(HWND hWnd);
		void CleanupDirectXDeviceAndSwapChain();
		void CreateRenderTarget();
		void CleanupRenderTarget();
		char* ShowSelectFileDialogAndReturnPath();
	}
}