#include "Application.h"
#include <string>
#include <format>

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

	ShowWindow(this->windowHandle, cmdShow);

	return true;
}

void Application::Shutdown(HINSTANCE instance)
{
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