#pragma once

#include <Windows.h>

#define WINDOW_CLASS_NAME		"SpiderSolitaireWindow"

class Application
{
public:
	Application();
	virtual ~Application();

	bool Setup(HINSTANCE instance, int cmdShow, int width, int height);
	void Shutdown(HINSTANCE instance);
	int Run();

private:
	static LRESULT CALLBACK WindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

	void Tick();
	void Render();

	HWND windowHandle;
};