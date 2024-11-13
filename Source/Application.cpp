#include "Application.h"
#include <string>
#include <format>
#include <locale>
#include <codecvt>
#include <assert.h>

Application::Application()
{
	this->windowHandle = NULL;
	this->rtvDescriptorSize = 0;

	for (int i = 0; i < _countof(this->swapFrame); i++)
	{
		SwapFrame& frame = this->swapFrame[i];
		frame.count = 0L;
		frame.fenceEvent = NULL;
	}
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
		std::string error = std::format("Failed to register window class.  Error code: {:x}", GetLastError());
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	RECT windowRect{ 0, 0, width, height };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	this->windowHandle = CreateWindow(
		windowClass.lpszClassName,
		TEXT("Solitaire"),
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
		std::string error = std::format("Failed to create window.  Error code: {:x}", GetLastError());
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
		std::string error = std::format("Failed to get debug interface.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}
#endif

	ComPtr<IDXGIFactory4> factory;
	result = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
	if (FAILED(result))
	{
		std::string error = std::format("Failed to create DXGI factory.  Error code: {:x}", result);
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
				std::string error = std::format("Failed to create D3D12 device.  Error code: {:x}", result);
				MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
				return false;
			}

			break;
		}
	}

	if (!this->device.Get())
	{
		MessageBox(NULL, "Failed to find a D3D12-compatible device.", "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	result = this->device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&this->commandQueue));
	if (FAILED(result))
	{
		std::string error = std::format("Failed to create the command queue.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	ComPtr<IDXGISwapChain1> swapChain1;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = _countof(this->swapFrame);
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;
	result = factory->CreateSwapChainForHwnd(
		this->commandQueue.Get(),
		this->windowHandle,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1);
	if (FAILED(result))
	{
		std::string error = std::format("Failed to create swap-chain.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	result = swapChain1.As(&this->swapChain);
	if (FAILED(result))
	{
		std::string error = std::format("Failed to cast swap-chain.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	rtvHeapDesc.NumDescriptors = swapChainDesc.BufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	result = this->device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&this->rtvHeap));
	if (FAILED(result))
	{
		std::string error = std::format("Failed to create RTV descriptor.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	this->rtvDescriptorSize = this->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(this->rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (int i = 0; i < rtvHeapDesc.NumDescriptors; i++)
	{
		SwapFrame& frame = this->swapFrame[i];

		result = this->swapChain->GetBuffer(i, IID_PPV_ARGS(&frame.renderTarget));
		if (FAILED(result))
		{
			std::string error = std::format("Failed to get render target for frame {}.  Error code: {:x}", i, result);
			MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
			return false;
		}

		this->device->CreateRenderTargetView(frame.renderTarget.Get(), nullptr, rtvHandle);

		rtvHandle.Offset(1, this->rtvDescriptorSize);

		result = this->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.commandAllocator));
		if (FAILED(result))
		{
			std::string error = std::format("Failed to create command allocator for frame {}.  Error code: {:x}", i, result);
			MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
			return false;
		}

		result = this->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frame.fence));
		if (FAILED(result))
		{
			std::string error = std::format("Failed to create fence for frame {}.  Error code: {:x}", i, result);
			MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
			return false;
		}

		frame.fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (frame.fenceEvent == nullptr)
		{
			std::string error = std::format("Failed to create fence event for frame {}.  Error code: {:x}", i, GetLastError());
			MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
			return false;
		}
	}

	// TODO: Make the root signature.
	// TODO: Make the PSO.

	// Note that the command-allocator we pass in here may not really matter, because we
	// reset the command list to the desired command-allocator before each GPU submission.
	result = this->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, this->swapFrame[0].commandAllocator.Get(), nullptr, IID_PPV_ARGS(&this->commandList));
	if (FAILED(result))
	{
		std::string error = std::format("Failed to create command list.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	this->commandList->Close();

	// TODO: Wait on GPU here to upload vertex buffers and textures into GPU memory.

	ShowWindow(this->windowHandle, cmdShow);

	return true;
}

void Application::Shutdown(HINSTANCE instance)
{
	// We don't really need to explicitly release resources here,
	// but I guess I'm just doing it anyway.  We do, however, need
	// to wait for the GPU to finish before any resources are released
	// either here or when the destructors are called.
	this->WaitForGPUIdle();
	
	for (int i = 0; i < _countof(this->swapFrame); i++)
	{
		SwapFrame& frame = this->swapFrame[i];
		frame.renderTarget = nullptr;
		frame.commandAllocator = nullptr;

		if (frame.fenceEvent != NULL)
		{
			CloseHandle(frame.fenceEvent);
			frame.fenceEvent = NULL;
		}
	}

	this->swapChain = nullptr;
	this->commandQueue = nullptr;
	this->rtvHeap = nullptr;
	this->commandList = nullptr;
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
	HRESULT result = 0;

	// We want to render frame "i" of the swap-chain.
	UINT i = this->swapChain->GetCurrentBackBufferIndex();
	SwapFrame& frame = this->swapFrame[i];

	// Wait if necessary for frame "i" to finish being rendered with the commands we issued last time around.
	this->StallUntilFrameCompleteIfNecessary(frame);

	// Note that we would not want to do this if the GPU was still using the commands.
	result = frame.commandAllocator->Reset();
	assert(SUCCEEDED(result));
	
	// Have our command list take memory for commands from the frame's command allocator.
	// This also opens the command list for recording.
	result = this->commandList->Reset(frame.commandAllocator.Get(), nullptr);
	assert(SUCCEEDED(result));

	// Indicate the back-buffer usage as a render target.  I think that the GPU can stall itself here in such cases if the resource was being used in a different way.
	// For example, maybe a shadow buffer is being used as a render target and then some other command list wants to use it as a shader resource.  I have no idea.
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame.renderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	this->commandList->ResourceBarrier(1, &barrier);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(this->rtvHeap->GetCPUDescriptorHandleForHeapStart(), i, this->rtvDescriptorSize);

	const float clearColor[] = { 0.0f, 0.5f, 0.0f, 1.0f };
	this->commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// Indicate that the back-buffer can now be used to present.
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame.renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	this->commandList->ResourceBarrier(1, &barrier);

	// We're done issuing commands, so close the list.
	result = this->commandList->Close();
	assert(SUCCEEDED(result));

	// Tell the GPU to start executing the command list.
	ID3D12CommandList* comandListArray[] = { this->commandList.Get() };
	this->commandQueue->ExecuteCommandLists(_countof(comandListArray), comandListArray);

	// Tell the driver that frame "i" can now be presented as far as we're concerned.  However, the GPU
	// might not actually be done rendering the frame, so I'm not sure how that's handled internally.
	// In any case, this will cause our next call to GetCurrentBackBufferIndex() to return the next frame
	// in the swap chain, I believe.  Note that this must be done BEFORE we issue the signal-fence command
	// that follows, or else waiting on that fence does not necessarily ensure that the GPU is idle, at
	// least as far as this frame is concerned.  One possible reason may be that the present call here
	// actually queues up one or more commands.  Note that we did need to give the swap-chain a pointer
	// to the command-queue when we created it.
	result = this->swapChain->Present(1, 0);
	assert(SUCCEEDED(result));

	// Now schedual a fence event to occur once the command list finishes.
	this->commandQueue->Signal(frame.fence.Get(), ++frame.count);
}

void Application::StallUntilFrameCompleteIfNecessary(SwapFrame& frame)
{
	// Do we need to stall here waiting for the GPU?
	UINT64 currentFenceValue = frame.fence->GetCompletedValue();
	if (currentFenceValue < frame.count)
	{
		HRESULT result = frame.fence->SetEventOnCompletion(frame.count, frame.fenceEvent);
		assert(SUCCEEDED(result));

		// Let our thread go dormant until we're woken up by completion of the event.
		WaitForSingleObjectEx(frame.fenceEvent, INFINITE, FALSE);

		currentFenceValue = frame.fence->GetCompletedValue();
		assert(currentFenceValue == frame.count);
	}
}

void Application::WaitForGPUIdle()
{
	for (int i = 0; i < _countof(this->swapFrame); i++)
		this->StallUntilFrameCompleteIfNecessary(this->swapFrame[i]);
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