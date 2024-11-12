#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12.h>
#include <d3d12sdklayers.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

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

	struct SwapFrame;

	void Tick();
	void Render();
	void WaitForGPUIdle();
	void StallUntilFrameCompleteIfNecessary(SwapFrame& frame);

	struct SwapFrame
	{
		ComPtr<ID3D12Resource> renderTarget;
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		HANDLE fenceEvent;
		ComPtr<ID3D12Fence> fence;
		UINT64 count;
	};

	HWND windowHandle;
	ComPtr<ID3D12Device> device;
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<IDXGISwapChain3> swapChain;
	SwapFrame swapFrame[2];
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	UINT rtvDescriptorSize;
	ComPtr<ID3D12GraphicsCommandList> commandList;
};