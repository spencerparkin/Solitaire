#include "Application.h"
#include "SpiderSolitaireGame.h"
#include <string>
#include <format>
#include <locale>
#include <codecvt>
#include <assert.h>
#include <windowsx.h>

using namespace DirectX;

Application::Application()
{
	this->maxCardDrawCallsPerSwapFrame = 128;
	this->windowHandle = NULL;
	this->generalFenceEvent = NULL;
	this->generalCount = 0L;
	this->cardConstantsBufferPtr = nullptr;
	::ZeroMemory(&this->cardVertexBufferView, sizeof(this->cardVertexBufferView));
	::ZeroMemory(&this->viewport, sizeof(this->viewport));
	::ZeroMemory(&this->scissorRect, sizeof(this->scissorRect));

	this->worldExtents.min = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	this->worldExtents.max = XMVectorSet(150.0f, 100.0f, 0.0f, 1.0f);

	const double cardAspectRatio = 0.68870523415977961432506887052342;
	const double cardWidth = 12.0f;

	this->cardSize.min = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	this->cardSize.max = XMVectorSet(cardWidth, cardWidth / cardAspectRatio, 0.0f, 1.0f);
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

	this->viewport.TopLeftX = 0;
	this->viewport.TopLeftY = 0;
	this->viewport.Width = width;
	this->viewport.Height = height;

	this->scissorRect.left = 0;
	this->scissorRect.right = width;
	this->scissorRect.top = 0;
	this->scissorRect.bottom = height;

	// TODO: This matrix would need to be recalculated every time the window was resized.
	double aspectRatio = double(width) / double(height);
	this->adjustedWorldExtents = this->worldExtents;
	this->adjustedWorldExtents.ExpandToMatchAspectRatio(aspectRatio);
	this->worldToProj = XMMatrixOrthographicOffCenterLH(
		XMVectorGetX(this->adjustedWorldExtents.min),
		XMVectorGetX(this->adjustedWorldExtents.max),
		XMVectorGetY(this->adjustedWorldExtents.min),
		XMVectorGetY(this->adjustedWorldExtents.max),
		0.0f,
		1.0f
	);

	// TODO: Handle window resizing by recreating the swap-chain as necessary?  Make sure to share code between that process and our setup here.
	ComPtr<IDXGISwapChain1> swapChain1;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = 2;		// Note that this number could be increased without breaking the program (in theory), but latency would increase.
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

	UINT rtvDescriptorSize = this->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(this->rtvHeap->GetCPUDescriptorHandleForHeapStart());
	this->swapFrameArray.resize(rtvHeapDesc.NumDescriptors);
	for (int i = 0; i < rtvHeapDesc.NumDescriptors; i++)
	{
		SwapFrame& frame = this->swapFrameArray[i];

		result = this->swapChain->GetBuffer(i, IID_PPV_ARGS(&frame.renderTarget));
		if (FAILED(result))
		{
			std::string error = std::format("Failed to get render target for frame {}.  Error code: {:x}", i, result);
			MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
			return false;
		}

		wchar_t renderTargetName[512];
		wsprintfW(renderTargetName, L"Render Target %d", i);
		frame.renderTarget->SetName(renderTargetName);

		this->device->CreateRenderTargetView(frame.renderTarget.Get(), nullptr, rtvHandle);

		rtvHandle.Offset(1, rtvDescriptorSize);

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

	CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

	CD3DX12_ROOT_PARAMETER1 rootParameters[2];
	rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_VERTEX);

	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &samplerDesc, rootSignatureFlags);

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

	this->rootSignature->SetName(L"Root Signature");

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
	psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
	psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	
	result = this->device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&this->pipelineState));
	if (FAILED(result))
	{
		std::string error = std::format("Failed to create pipeline state object.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	this->pipelineState->SetName(L"Pipeline State");

	// Reserve some space in slow GPU memory for constants buffers.  Whenever the GPU
	// wants to read the memory, it has to be marsheled over (whatever the hell that means).
	// How else am I supposed to do this?  Is there a better way?
	UINT constantsBufferSize = sizeof(CardConstantsBuffer) * this->maxCardDrawCallsPerSwapFrame * this->swapFrameArray.size();
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	auto constantsBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(constantsBufferSize);
	result = this->device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&constantsBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&this->cardConstantsBuffer));
	if (FAILED(result))
	{
		std::string error = std::format("Failed to create constants buffers.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	// Map the constants buffer into CPU memory.  This is so that we can update it each draw-call as necessary.
	// Nothing wrong with just leaving it mapped for the life of the resource.
	CD3DX12_RANGE readRange(0, 0);		// This means we never need to read from the buffer.  We only write to it.
	result = this->cardConstantsBuffer->Map(0, &readRange, reinterpret_cast<void**>(&this->cardConstantsBufferPtr));
	if (FAILED(result))
	{
		std::string error = std::format("Failed to map constants buffer.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}
	::ZeroMemory(this->cardConstantsBufferPtr, constantsBufferSize);

	// Create a descriptor heap for constants buffers.
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
	cbvHeapDesc.NumDescriptors = this->maxCardDrawCallsPerSwapFrame * this->swapFrameArray.size();
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = this->device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&this->cbvHeap));
	if (FAILED(result))
	{
		std::string error = std::format("Failed to create descriptor heap for constants buffers.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	// Now create a view into each constants buffer at each heap location.
	UINT cbvDescriptorSize = this->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(this->cbvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < cbvHeapDesc.NumDescriptors; i++)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
		cbvDesc.BufferLocation = this->cardConstantsBuffer->GetGPUVirtualAddress() + i * sizeof(CardConstantsBuffer);
		cbvDesc.SizeInBytes = sizeof(CardConstantsBuffer);
		this->device->CreateConstantBufferView(&cbvDesc, cbvHandle);
		cbvHandle.Offset(1, cbvDescriptorSize);
	}

	result = this->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, this->generalCommandAllocator.Get(), this->pipelineState.Get(), IID_PPV_ARGS(&this->commandList));
	if (FAILED(result))
	{
		std::string error = std::format("Failed to create command list.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	this->commandList->SetName(L"Graphics Command List");

	// Command lists are created in the recording state.  Since no subsequent code assumes
	// that the command list is already recording, just close it now.
	this->commandList->Close();

	if (!this->LoadCardTextures())
		return false;
	
	if (!this->LoadCardVertexBuffer())
		return false;

#if defined _DEBUG
	std::srand(0);	// In debug, always seed with zero for consistent bug reproduction.
#else
	std::srand(std::time(nullptr));
#endif

	this->cardGame = std::make_shared<SpiderSolitaireGame>(this->worldExtents, this->cardSize);
	this->cardGame->NewGame();

	ShowWindow(this->windowHandle, cmdShow);

	return true;
}

bool Application::LoadCardVertexBuffer()
{
	// Here is the vertex buffer data, plain and simple.
	CardVertex cardVertexArray[] =
	{
		// First triangle...
		{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
		{ { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
		{ { 1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },

		// Second triangle...
		{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
		{ { 1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
		{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
	};

	// Get ready to generate a new command list.  This will open the command list for recording.
	HRESULT result = this->generalCommandAllocator->Reset();
	if (FAILED(result))
	{
		std::string error = std::format("Failed to reset the general command allocator.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}
	result = this->commandList->Reset(this->generalCommandAllocator.Get(), nullptr);
	if (FAILED(result))
	{
		std::string error = std::format("Failed to reset the command list with the general allocator.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	// Reserve space in the upload/staging area of the GPU for the vertex buffer.
	// This area can be mapped on the CPU, but is slow to access from a vertex shader.
	// I suppose that if we were going to skin on the CPU, thought, that we'd have to
	// use this area while rendering.  I don't know.
	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	CD3DX12_HEAP_PROPERTIES intermediateHeapProps(D3D12_HEAP_TYPE_UPLOAD);
	UINT cardVertexBufferSize = sizeof(cardVertexArray);
	auto vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cardVertexBufferSize);
	result = this->device->CreateCommittedResource(
		&intermediateHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&vertexBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&intermediateVertexBuffer));
	if (FAILED(result))
	{
		std::string error = std::format("Failed to create intermediate vertex buffer resource.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	// Reserve space in the GPU where a vertex shader has quick access to the data.
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	result = this->device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&vertexBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&this->cardVertexBuffer));
	if (FAILED(result))
	{
		std::string error = std::format("Failed to create vertex buffer resource.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	// Write the vertex buffer data from CPU memory into the staging area on the GPU.
	UINT8* vertexBufferPtr = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	result = intermediateVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&vertexBufferPtr));
	assert(SUCCEEDED(result));
	memcpy(vertexBufferPtr, cardVertexArray, cardVertexBufferSize);
	intermediateVertexBuffer->Unmap(0, nullptr);

	// Now record a command that will transfer the vertex buffer data from staging to fast GPU memory.
	this->commandList->CopyBufferRegion(this->cardVertexBuffer.Get(), 0, intermediateVertexBuffer.Get(), 0, cardVertexBufferSize);

	// Record a command that changes the resource state from a copy destination to a vertex shader data source.
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(this->cardVertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	this->commandList->ResourceBarrier(1, &barrier);

	// We won't need the staging area anymore, so let it go.
	D3D12_DISCARD_REGION region{};
	region.NumSubresources = 1;
	this->commandList->DiscardResource(intermediateVertexBuffer.Get(), &region);

	// Okay, have the GPU perform the transfer.  Block until finished.
	this->commandList->Close();
	this->ExecuteCommandList();
	this->WaitForGPUIdle();

	// We'll need this view for later rendering, so cache it now.
	this->cardVertexBufferView.BufferLocation = this->cardVertexBuffer->GetGPUVirtualAddress();
	this->cardVertexBufferView.StrideInBytes = sizeof(CardVertex);
	this->cardVertexBufferView.SizeInBytes = cardVertexBufferSize;

	return true;
}

bool Application::LoadCardTextures()
{
	// Locate our texture asset directory.
	std::filesystem::path folderPath;
	if (!this->FindAssetDirectory("Textures", folderPath))
	{
		MessageBox(NULL, "Failed to locate textures directory.", "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	// Soak up all the texture file names we find in the asset folder.
	std::vector<std::string> textureFileArray;
	for (const auto& entry : std::filesystem::directory_iterator(folderPath))
	{
		std::string ext = entry.path().extension().string();
		if (ext == ".dds")
			textureFileArray.push_back(entry.path().string());
	}

	if (textureFileArray.size() == 0)
	{
		MessageBox(NULL, "No textures found!", "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	// We'll need a heap of SRVs for all the textures.
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
	srvHeapDesc.NumDescriptors = (UINT)textureFileArray.size();
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HRESULT result = this->device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&this->srvHeap));
	if (FAILED(result))
	{
		std::string error = std::format("Failed to create SRV heap.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	// Get ready to walk the heap.
	UINT srvDescriptorSize = this->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(this->srvHeap->GetCPUDescriptorHandleForHeapStart());
	UINT srvOffset = 0;

	// Get ready to generate a new command list.  This will open the command list for recording.
	result = this->generalCommandAllocator->Reset();
	if (FAILED(result))
	{
		std::string error = std::format("Failed to reset the general command allocator.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}
	result = this->commandList->Reset(this->generalCommandAllocator.Get(), nullptr);
	if (FAILED(result))
	{
		std::string error = std::format("Failed to reset the command list with the general allocator.  Error code: {:x}", result);
		MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	// We'll need these to stay in scope until the GPU is done with them.
	std::vector<ComPtr<ID3D12Resource>> intermediateTextureArray;

	// Now go load up all the textures we found.
	for(const std::string& textureFile : textureFileArray)
	{
		// This will load the texture data into CPU memory and then reserve GPU memory that has fast-access from a pixel shader, but that's all it does.
		std::wstring ddsFilePath = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(textureFile);
		std::unique_ptr<uint8_t[]> ddsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		ComPtr<ID3D12Resource> texture;
		result = LoadDDSTextureFromFile(this->device.Get(), ddsFilePath.c_str(), &texture, ddsData, subresources);
		if (FAILED(result))
		{
			std::string error = std::format("Failed to load texture \"{}\".  Error code: {:x}", textureFile.c_str(), result);
			MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
			return false;
		}

		// This will reserve GPU memory that has slow-access from a pixel shader (I think!), but can also be mapped into CPU memory.
		UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, (UINT)subresources.size());
		auto intermediateTextureDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
		ComPtr<ID3D12Resource> intermediateTexture;
		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		result = this->device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&intermediateTextureDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&intermediateTexture));
		if (FAILED(result))
		{
			std::string error = std::format("Failed to create resource for texture with high GPU availability.  Error code: {:x}", result);
			MessageBox(NULL, error.c_str(), "Error!", MB_ICONERROR | MB_OK);
			return false;
		}

		// Keep the intermediate texture in scope until the GPU is done with it.
		intermediateTextureArray.push_back(intermediateTexture);

		// This will map the intermediate texture into CPU memory, copy the loaded texture data into it (into GPU memory),
		// then record a command that will transfer the texture data from slow GPU memory to fast GPU memory.
		UpdateSubresources(this->commandList.Get(), texture.Get(), intermediateTexture.Get(), 0, 0, (UINT)subresources.size(), subresources.data());

		// Now record a command that will change the resource state of the fast GPU memory texture from that of a copy destination to a pixel shader data source.
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		this->commandList->ResourceBarrier(1, &barrier);

		// We'll need a shader resource view into the texture for later rendering.
		D3D12_RESOURCE_DESC textureDesc = texture->GetDesc();
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		this->device->CreateShaderResourceView(texture.Get(), &srvDesc, srvHandle);

		// Move the shader resource view handle along to the next one in the heap.
		srvHandle.Offset(1, srvDescriptorSize);

		// Lastly, save the CPU-side reference we have to the texture in our map so that we can use it later for rendering.
		CardTexture cardTexture{ texture, srvOffset++ };
		std::string cardName = std::filesystem::path(textureFile).stem().string();
		this->cardTextureMap.insert(std::pair(cardName, cardTexture));

		// I don't think we need to keep the intermediate texture around, so might as well discard it.
		D3D12_DISCARD_REGION region{};
		region.NumSubresources = 1;
		this->commandList->DiscardResource(intermediateTexture.Get(), &region);
	}

	// Now go tell the GPU to transfer the textures from slow GPU memory to fast GPU memory.  Stall until the operation is complete.
	this->commandList->Close();
	this->ExecuteCommandList();
	this->WaitForGPUIdle();

	return true;
}

void Application::Shutdown(HINSTANCE instance)
{
	// We don't really need to explicitly release resources here,
	// but I guess I'm just doing it anyway.  We do, however, need
	// to wait for the GPU to finish before any resources are released
	// either here or when the destructors are called.
	this->WaitForGPUIdle();
	
	this->cardGame.reset();

	for (SwapFrame& frame : this->swapFrameArray)
	{
		frame.renderTarget = nullptr;
		frame.commandAllocator = nullptr;
		frame.fence = nullptr;

		if (frame.fenceEvent != NULL)
		{
			CloseHandle(frame.fenceEvent);
			frame.fenceEvent = NULL;
		}
	}

	this->swapFrameArray.clear();

	if (this->generalFenceEvent != NULL)
	{
		CloseHandle(this->generalFenceEvent);
		this->generalFenceEvent = NULL;
	}

	// This releases the resources as far as they're tracked on the CPU, I think,
	// but this has nothing to do with releasing or discarding resources that have
	// been reserved on the GPU.  I suppose we could go issue commands to discard
	// all resources we reserved/committed, but maybe that's just completely unecessary.
	// Maybe the D3D12 driver takes care of having the GPU free all its memory that we used?
	this->cbvHeap = nullptr;
	this->cardConstantsBuffer = nullptr;
	this->cardVertexBuffer = nullptr;
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
	SwapFrame& frame = this->swapFrameArray[i];

	// Wait if necessary for frame "i" to finish being rendered with the commands we issued last time around.
	this->StallUntilFrameCompleteIfNecessary(frame);

	// Note that we would not want to do this if the GPU was still using the commands.
	result = frame.commandAllocator->Reset();
	assert(SUCCEEDED(result));
	
	// Have our command list take memory for commands from the frame's command allocator.
	// This also opens the command list for recording.
	result = this->commandList->Reset(frame.commandAllocator.Get(), this->pipelineState.Get());
	assert(SUCCEEDED(result));

	// Indicate the back-buffer usage as a render target.  I think that the GPU can stall itself here in such cases if the resource was being used in a different way.
	// For example, maybe a shadow buffer is being used as a render target and then some other command list wants to use it as a shader resource.  I have no idea.
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame.renderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	this->commandList->ResourceBarrier(1, &barrier);

	UINT rtvDescriptorSize = this->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(this->rtvHeap->GetCPUDescriptorHandleForHeapStart(), i, rtvDescriptorSize);

	const float clearColor[] = { 0.0f, 0.5f, 0.0f, 1.0f };
	this->commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	this->commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	this->commandList->SetGraphicsRootSignature(this->rootSignature.Get());

	this->commandList->RSSetViewports(1, &this->viewport);
	this->commandList->RSSetScissorRects(1, &this->scissorRect);

	// Rendering our scene boils down to nothing more than just drawing a bunch of cards.
	if (this->cardGame.get())
	{
		UINT drawCallCount = 0;
		std::vector<const SolitaireGame::Card*> cardRenderList;
		this->cardGame->GenerateRenderList(cardRenderList);
		for (auto card : cardRenderList)
		{
			if (drawCallCount >= this->maxCardDrawCallsPerSwapFrame)
				break;

			this->RenderCard(card, drawCallCount++);
		}
	}

	// Indicate that the back-buffer can now be used to present.
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame.renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	this->commandList->ResourceBarrier(1, &barrier);

	// We're done issuing commands, so close the list.
	result = this->commandList->Close();
	assert(SUCCEEDED(result));

	// Tell the GPU to start executing the command list.
	this->ExecuteCommandList();

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

void Application::ExecuteCommandList()
{
	ID3D12CommandList* comandListArray[] = { this->commandList.Get() };
	this->commandQueue->ExecuteCommandLists(_countof(comandListArray), comandListArray);
}

void Application::StallUntilFrameCompleteIfNecessary(SwapFrame& frame)
{
	// Do we need to stall here waiting for the GPU?
	UINT64 currentFenceValue = frame.fence->GetCompletedValue();
	if (currentFenceValue < frame.count)
	{
		// Yes.  Configure our fence to set the desired value when it's triggered as complete.
		HRESULT result = frame.fence->SetEventOnCompletion(frame.count, frame.fenceEvent);
		assert(SUCCEEDED(result));

		// Let our thread go dormant until we're woken up by completion of the event.
		WaitForSingleObjectEx(frame.fenceEvent, INFINITE, FALSE);

		// Do a quick sanity check here for my sake.
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

void Application::RenderCard(const SolitaireGame::Card* card, UINT drawCallCount)
{
	// Note that here we're assuming that the command list is in the record state,
	// ready for us to record rendering commands.

	std::string renderKey = card->GetRenderKey();
	auto pair = this->cardTextureMap.find(renderKey);
	if (pair == this->cardTextureMap.end())
		return;

	XMMATRIX scaleMatrix = XMMatrixScaling(this->cardSize.GetWidth(), this->cardSize.GetHeight(), 1.0f);
	XMMATRIX translationMatrix = XMMatrixTranslation(XMVectorGetX(card->position), XMVectorGetY(card->position), 0.0f);
	XMMATRIX objToWorld = scaleMatrix * translationMatrix;

	const CardTexture& cardTexture = pair->second;

	// Fill out our constants buffer for this draw call.
	UINT i = this->swapChain->GetCurrentBackBufferIndex();
	UINT j = i * this->maxCardDrawCallsPerSwapFrame + drawCallCount;
	auto cardConstantsBufferData = &reinterpret_cast<CardConstantsBuffer*>(this->cardConstantsBufferPtr)[j];
	cardConstantsBufferData->objToProj = objToWorld * this->worldToProj;

	// Use the desired texture.
	ID3D12DescriptorHeap* descriptorHeapArray[] = { this->srvHeap.Get() };		// Note that all of these must be the same type of heap.
	this->commandList->SetDescriptorHeaps(_countof(descriptorHeapArray), descriptorHeapArray);
	UINT srvDescriptorSize = this->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(this->srvHeap->GetGPUDescriptorHandleForHeapStart(), cardTexture.srvOffset, srvDescriptorSize);
	this->commandList->SetGraphicsRootDescriptorTable(0, srvHandle);

	// Make sure our constants buffer is bound.
	descriptorHeapArray[0] = { this->cbvHeap.Get() };
	this->commandList->SetDescriptorHeaps(_countof(descriptorHeapArray), descriptorHeapArray);
	UINT cbvDescriptorSize = this->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(this->cbvHeap->GetGPUDescriptorHandleForHeapStart(), j, cbvDescriptorSize);
	this->commandList->SetGraphicsRootDescriptorTable(1, cbvHandle);

	// TODO: We might not need to call these for each card.  Maybe just once before drawing all cards.
	this->commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	this->commandList->IASetVertexBuffers(0, 1, &this->cardVertexBufferView);

	// Finally, make the draw call!
	this->commandList->DrawInstanced(6, 1, 0, 0);
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
		case WM_LBUTTONDOWN:
		{
			app->OnLeftMouseButtonDown(wParam, lParam);
			break;
		}
		case WM_LBUTTONUP:
		{
			app->OnLeftMouseButtonUp(wParam, lParam);
			break;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
	}

	return DefWindowProc(windowHandle, message, wParam, lParam);
}

void Application::OnLeftMouseButtonDown(WPARAM wParam, LPARAM lParam)
{
	XMVECTOR worldMousePoint = this->MouseLocationToWorldLocation(lParam);

	if (this->cardGame.get())
		this->cardGame->OnGrabAt(worldMousePoint);
}

void Application::OnLeftMouseButtonUp(WPARAM wParam, LPARAM lParam)
{
	XMVECTOR worldMousePoint = this->MouseLocationToWorldLocation(lParam);

	if (this->cardGame.get())
		this->cardGame->OnReleaseAt(worldMousePoint);
}

XMVECTOR Application::MouseLocationToWorldLocation(LPARAM lParam)
{
	int mouseX = GET_X_LPARAM(lParam);
	int mouseY = this->viewport.Height - GET_Y_LPARAM(lParam);
	XMVECTOR mousePoint = XMVectorSet(
		float(mouseX),
		float(mouseY),
		0.0f,
		1.0f
	);

	Box viewportBox;
	viewportBox.min = XMVectorSet(
		0.0f,
		0.0f,
		0.0f,
		1.0f);
	viewportBox.max = XMVectorSet(
		float(this->viewport.Width),
		float(this->viewport.Height),
		0.0f,
		1.0f);

	XMVECTOR uvs = viewportBox.PointToUVs(mousePoint);
	XMVECTOR worldMousePoint = this->adjustedWorldExtents.PointFromUVs(uvs);
	return worldMousePoint;
}