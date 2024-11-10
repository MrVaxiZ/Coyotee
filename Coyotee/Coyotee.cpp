#pragma disable warning C4005 // Macro redefinition warning // It is being thrown multiple times because of libs we are using here

// OS Includes
#include <iostream>
#include <d3d11.h>
#include <tchar.h>
#include <Windows.h>
#include <thread>

// Internal Includes
#include "UsingAliases.h"
#include "KeyHandler.h"
#include "AudioHandler.h"
#include "VideoHandler.h"
#include "StaticData.h"

// External Includes
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

// Static Data To Points
static const bool	bcTrue						= true;
static const bool	bcFalse						= false;
static bool			bTrue						= true;
static bool			bFalse						= false;
static float		windowHeight				= 1200;
static float		windowWidth					= 1200;
static const char*	START_STREAM_BTN_TEXT		= "Start Stream";
static const char*	START_RECORDING_BTN_TEXT	= "Start Recording";

// Data
static ID3D11Device*			g_pd3dDevice			= nullptr;
static ID3D11DeviceContext*		g_pd3dDeviceContext		= nullptr;
static IDXGISwapChain*			g_pSwapChain			= nullptr;
static ID3D11RenderTargetView*	g_mainRenderTargetView	= nullptr;
static bool                     g_SwapChainOccluded		= false;
static UINT                     g_ResizeWidth			= 0;
static UINT                     g_ResizeHeight			= 0;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main(int, char**)
{
	// ICON
	LPCWSTR icon_path = L"Coyotee.ico";
	HICON icon = (HICON)LoadImage(
		NULL,            // No module handle (loading from file, not resources)
		icon_path,       // Path to the icon file
		IMAGE_ICON,      // Specify that we are loading an icon
		0, 0,            // Use default icon size (32x32 for large icons) 
		LR_LOADFROMFILE  // Load icon from file
	);

	// Check if the icon was loaded successfully
	if (icon == NULL) {
		MessageBox(NULL, L"Failed to load icon.", L"Error", MB_OK | MB_ICONERROR);
	}

	// Create application window
	WNDCLASSEXW wc =
	{
		sizeof(wc),                // Size of the WNDCLASSEXW structure in bytes (required).
		CS_CLASSDC,                // Class style (CS_CLASSDC means using a private device context for the window).
		WndProc,                   // Pointer to the window procedure (WndProc is your window message handler function).
		0L,                        // Extra bytes to allocate for this window class (0 means no extra space).
		0L,                        // Extra bytes to allocate for each instance of the window (0 means no extra space).
		GetModuleHandle(nullptr),  // Handle to the application instance (used to identify the application).
		icon,                      // Handle to the icon for the window
		nullptr,                   // Handle to the cursor for the window (nullptr uses the default cursor).
		nullptr,                   // Handle to the background brush (nullptr means no background brush).
		nullptr,                   // Menu resource name (nullptr means no menu).
		L"Coyotee",                // Class name for the window (must match the name used in CreateWindowW).
		nullptr                    // Handle to the small icon (nullptr means no small icon is specified).
	};
	::RegisterClassExW(&wc);        // Registers the window class so it can be used to create a window later.

	HWND hwnd = ::CreateWindowW(
		wc.lpszClassName,           // The name of the window class (must match the class registered earlier, "Coyotee").
		wc.lpszClassName,           // The window's title that will appear in the title bar.
		WS_OVERLAPPEDWINDOW,        // The style of the window (WS_OVERLAPPEDWINDOW means it will be a standard window with a title bar, border, and control buttons).
		100,                        // X-coordinate of the top-left corner of the window (relative to the screen).
		100,                        // Y-coordinate of the top-left corner of the window (relative to the screen).
		windowHeight,               // Width of the window (in pixels)
		windowWidth,                // Height of the window (in pixels)
		nullptr,                    // Handle to the parent window (nullptr means no parent window).
		nullptr,                    // Handle to the menu for the window (nullptr means no menu).
		wc.hInstance,               // Handle to the application instance (same as the one used in WNDCLASSEXW).
		nullptr                     // Pointer to additional window creation data (nullptr means no extra data).
	);

	// Initialize Direct3D
	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	io.Fonts->AddFontFromFileTTF("../resources/Main_Font.ttf", 15.0f);

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// Main loop
	bool done = false;
	while (!done)
	{
		// Poll and handle messages (inputs, window resize, etc.)
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;

		// Handle window being minimized or screen locked
		if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
		{
			::Sleep(10);
			continue;
		}
		g_SwapChainOccluded = false;

		// Handle window resize (we don't resize directly in the WM_SIZE handler)
		if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
		{
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
			g_ResizeWidth = g_ResizeHeight = 0;
			CreateRenderTarget();
		}

		// Start the Dear ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// Query the current size of the application window
		RECT rect;
		if (GetClientRect(hwnd, &rect))
		{
			windowWidth = static_cast<float>(rect.right - rect.left);
			windowHeight = static_cast<float>(rect.bottom - rect.top);
		}

		// Main Window Set-Up
		{
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));  // Set the size to match the application window

			ImGui::Begin("Main", &bTrue, ImGuiWindowFlags_NoDecoration);

			// Set the button position
			ImGui::SetCursorPos(ImVec2(windowWidth - 200, windowHeight - 100));
			ImGui::Button(START_STREAM_BTN_TEXT, ImVec2(100, 20));
			if (ImGui::IsItemClicked()) {
				// Send data through the api
				printf("Stream btn is pressed!\n");
			}

			/* RECORDING BUTTON  */
			ImGui::SetCursorPos(ImVec2(windowWidth - 200, windowHeight - 75));
			ImGui::Button(START_RECORDING_BTN_TEXT, ImVec2(100, 20));
			if (ImGui::IsItemClicked()) {
				printf("Recording button is pressed!\n");

				if (!StaticData::RecordingButtonPressed) {
					StaticData::RecordingButtonPressed = true;

					// Start the audio input capture thread
					std::thread capture_input_thread([]() {
						AudioHandler audioHandler;  // Initialize AudioHandler
						printf("--- STARTING CAPTURING INPUT AUDIO ---\n");
						audioHandler.CaptureAudioInput();
						});
					capture_input_thread.detach();

					// Start the audio output capture thread
					std::thread capture_output_thread([]() {
						AudioHandler audioHandler;  // Initialize AudioHandler
						printf("--- STARTING CAPTURING OUTPUT AUDIO ---\n");
						audioHandler.CaptureAudioOutput();
						});
					capture_output_thread.detach();
				}
				else {
					printf("Recording has been stopped!\n");
					StaticData::RecordingButtonPressed = false;
				}
			}
			ImGui::End();
		}

		// Rendering
		ImGui::Render();
		const float clear_color_with_alpha[4] =
		{
			clear_color.x * clear_color.w,
			clear_color.y * clear_color.w,
			clear_color.z * clear_color.w,
			clear_color.w
		};
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Present
		HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
		g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
	}

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}

// Helper functions
//  How tf this works? - W.K
bool CreateDeviceD3D(HWND hWnd)
{
	// Setup swap chain
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

	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
			return 0;
		g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
		g_ResizeHeight = (UINT)HIWORD(lParam);
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
