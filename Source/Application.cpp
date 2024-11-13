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
	this->generalFenceEvent = NULL;
	this->generalCount = 0L;

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

	result = this->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&this->generalCommandAllocator));
	if (FAILED(result))
	{
		std::string error = std::format("Failed to create general command allocator.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	result = this->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->generalFence));
	if (FAILED(result))
	{
		std::string error = std::format("Failed to create general fence.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	this->generalFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (this->generalFenceEvent == nullptr)
	{
		std::string error = std::format("Failed to create general fence event.  Error code: {:x}", GetLastError());
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	result = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error);
	if (FAILED(result))
	{
		std::string error = std::format("Failed to serialize root signature.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	result = this->device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&this->rootSignature));
	if (FAILED(result))
	{
		std::string error = std::format("Failed to create root signature.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	ComPtr<ID3DBlob> vertexShaderBlob;
	ComPtr<ID3DBlob> pixelShaderBlob;
	ComPtr<ID3DBlob> errorBlob;

#if defined _DEBUG
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	std::filesystem::path shaderFolder;
	if (!this->FindAssetDirectory("Shaders", shaderFolder))
	{
		std::string error = std::format("Failed to find shader asset folder.");
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	result = D3DCompileFromFile((shaderFolder / "CardShader.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShaderBlob, &errorBlob);
	if (FAILED(result))
	{
		std::string error = "Failed to compile vertex shader.  Error: " + this->GetErrorMessageFromBlob(errorBlob.Get());
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	result = D3DCompileFromFile((shaderFolder / "CardShader.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShaderBlob, nullptr);
	if (FAILED(result))
	{
		std::string error = "Failed to compile pixel shader.  Error: " + this->GetErrorMessageFromBlob(errorBlob.Get());
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = this->rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	
	result = this->device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&this->pipelineState));
	if (FAILED(result))
	{
		std::string error = std::format("Failed to create pipeline state object.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	// Note that the command list is created in the recording state.  We'll take advantage of this
	// by subsequently using the command list to upload textures and vertex buffers into GPU memory.
	result = this->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, this->generalCommandAllocator.Get(), this->pipelineState.Get(), IID_PPV_ARGS(&this->commandList));
	if (FAILED(result))
	{
		std::string error = std::format("Failed to create command list.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	// This loads textures into CPU memory, then populates the command list with commands to upload those textures into GPU memory.
	if (!this->LoadCardTextures())
		return false;
	
	this->commandList->Close();

	// TODO: Execute the command list.

	this->WaitForGPUIdle();

	ShowWindow(this->windowHandle, cmdShow);

	return true;
}

bool Application::LoadCardTextures()
{
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
		frame.fence = nullptr;

		if (frame.fenceEvent != NULL)
		{
			CloseHandle(frame.fenceEvent);
			frame.fenceEvent = NULL;
		}
	}

	if (this->generalFenceEvent != NULL)
	{
		CloseHandle(this->generalFenceEvent);
		this->generalFenceEvent = NULL;
	}

	this->generalFence = nullptr;
	this->generalCommandAllocator = nullptr;
	this->cardTextureMap.clear();
	this->swapChain = nullptr;
	this->commandQueue = nullptr;
	this->rtvHeap = nullptr;
	this->commandList = nullptr;
	this->srvHeap = nullptr;
	this->pipelineState = nullptr;
	this->rootSignature = nullptr;
	this->device = nullptr;

	UnregisterClass(WINDOW_CLASS_NAME, instance);
}

std::string Application::GetErrorMessageFromBlob(ID3DBlob* errorBlob)
{
	SIZE_T errorBufferSize = errorBlob->GetBufferSize();
	std::unique_ptr<char> errorBuffer(new char[errorBufferSize]);
	memcpy_s(errorBuffer.get(), errorBufferSize, errorBlob->GetBufferPointer(), errorBufferSize);
	return std::string(errorBuffer.get());
}

bool Application::FindAssetDirectory(const std::string& folderName, std::filesystem::path& folderPath)
{
	char moduleFileName[MAX_PATH];
	if (GetModuleFileNameA(NULL, moduleFileName, sizeof(moduleFileName)) == 0)
		return false;

	std::filesystem::path path(moduleFileName);
	path = path.parent_path();
	while (true)
	{
		int numComponents = std::distance(path.begin(), path.end());
		if (numComponents == 0)
			break;

		folderPath = path / folderName;
		if (std::filesystem::exists(folderPath))
			return true;

		path = path.parent_path();
	}

	return false;
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
	this->commandQueue->Signal(this->generalFence.Get(), ++this->generalCount);

	HRESULT result = this->generalFence->SetEventOnCompletion(this->generalCount, this->generalFenceEvent);
	assert(SUCCEEDED(result));

	WaitForSingleObjectEx(this->generalFenceEvent, INFINITE, FALSE);
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