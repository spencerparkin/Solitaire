#include "Application.h"
#include <string>
#include <format>
#include <locale>
#include <codecvt>

Application::Application()
{
	this->windowHandle = NULL;
}

/*virtual*/ Application::~Application()
{
}

bool Application::Setup(HINSTANCE instance, int cmdShow, int width, int height)
{
	if (this->windowHandle != NULL)
		return false;

	WNDCLASSEX windowClass{};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = &Application::WindowProc;
	windowClass.hInstance = instance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = WINDOW_CLASS_NAME;
	ATOM atom = RegisterClassEx(&windowClass);
	if (atom == 0)
	{
		std::string error = std::format("Failed to register window class.  Error code: {}", GetLastError());
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	RECT windowRect{ 0, 0, width, height };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	this->windowHandle = CreateWindow(
		windowClass.lpszClassName,
		TEXT("Spider Solitaire"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,	// Default X position.
		CW_USEDEFAULT,	// Default Y position.
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,		// No parent window.
		nullptr,		// No menu.
		instance,
		this);

	if (this->windowHandle == NULL)
	{
		std::string error = std::format("Failed to create window.  Error code: {}", GetLastError());
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	UINT dxgiFactoryFlags = 0;
	HRESULT result = 0;

#if defined _DEBUG
	ComPtr<ID3D12Debug> debugInterface;
	result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
	if (SUCCEEDED(result))
	{
		debugInterface->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
	else
	{
		std::string error = std::format("Failed to get debug interface.  Error code: {}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}
#endif

	ComPtr<IDXGIFactory4> factory;
	result = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
	if (FAILED(result))
	{
		std::string error = std::format("Failed to create DXGI factory.  Error code: {}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	// Look for a GPU that we can use.
	for (int i = 0; true; i++)
	{
		// Grab the next adapter.
		ComPtr<IDXGIAdapter1> adapter;
		result = factory->EnumAdapters1(i, &adapter);
		if (FAILED(result))
			break;

		// Skip any software-based adapters.  We want a physical GPU.
		DXGI_ADAPTER_DESC1 adapterDesc{};
		adapter->GetDesc1(&adapterDesc);
		if ((adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
			continue;

		// Does this adapter support DirectX 12?
		result = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(result))
		{
			// Yes.  Use it.
			std::string gpuDesc = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(adapterDesc.Description);
			OutputDebugStringA(std::format("Using GPU: {}\n", gpuDesc.c_str()).c_str());
			result = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), &this->device);
			if (FAILED(result))
			{
				std::string error = std::format("Failed to create D3D12 device.  Error code: {}", result);
				MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
				return false;
			}

			break;
		}
	}

	ShowWindow(this->windowHandle, cmdShow);

	return true;
}

void Application::Shutdown(HINSTANCE instance)
{
	// I suppose we don't need to explicitly release anything,
	// and supposedly the order of releases doesn't matter either.

	this->device = nullptr;

	UnregisterClass(WINDOW_CLASS_NAME, instance);
}

int Application::Run()
{
	MSG msg{};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}

void Application::Tick()
{
	//...
}

void Application::Render()
{
	//...
}

/*static*/ LRESULT CALLBACK Application::WindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto app = reinterpret_cast<Application*>(GetWindowLongPtr(windowHandle, GWLP_USERDATA));

	switch (message)
	{
		case WM_CREATE:
		{
			auto createStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
			SetWindowLongPtr(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
			return 0;
		}
		case WM_PAINT:
		{
			if (app)
			{
				app->Tick();
				app->Render();
			}

			return 0;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
	}

	return DefWindowProc(windowHandle, message, wParam, lParam);
}