#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3d12sdklayers.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

// TODO: Grab: https://github.com/microsoft/DirectX-Headers/blob/main/include/directx/d3dx12.h

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
	ComPtr<ID3D12Device> device;
};