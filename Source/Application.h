#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12.h>
#include <d3d12sdklayers.h>
#include <d3dcompiler.h>
#include <wrl.h>
#include <filesystem>
#include <unordered_map>
#include <DDSTextureLoader.h>
#include <DirectXMath.h>
#include "Clock.h"
#include "SolitaireGame.h"
#include "Box.h"

using Microsoft::WRL::ComPtr;

#define WINDOW_CLASS_NAME				"SolitaireWindow"
#define TICKS_PER_FPS_PROFILE			32
#define MIN_TIME_BETWEEN_CARDS_NEEDED	0.5

enum
{
	ID_NEW_GAME = 1000,
	ID_EXIT_PROGRAM,
	ID_SPIDER,
	ID_KLONDIKE,
	ID_ABOUT
};

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
	bool FindAssetDirectory(const std::string& folderName, std::filesystem::path& folderPath);
	std::string GetErrorMessageFromBlob(ID3DBlob* errorBlob);
	bool LoadCardTextures();
	bool LoadCardVertexBuffer();
	void ExecuteCommandList();
	void RenderCard(const SolitaireGame::Card* card, UINT drawCallCount);
	DirectX::XMVECTOR MouseLocationToWorldLocation(LPARAM lParam);
	void OnLeftMouseButtonDown(WPARAM wParam, LPARAM lParam);
	void OnLeftMouseButtonUp(WPARAM wParam, LPARAM lParam);
	void OnMouseMove(WPARAM wParam, LPARAM lParam);
	void OnKeyUp(WPARAM wParam, LPARAM lParam);
	void OnRightMouseButtonUp(WPARAM wParam, LPARAM lParam);
	void OnMouseCaptureChanged(WPARAM wParam, LPARAM lParam);

	struct SwapFrame
	{
		ComPtr<ID3D12Resource> renderTarget;
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		HANDLE fenceEvent;
		ComPtr<ID3D12Fence> fence;
		UINT64 count;
	};

	struct CardTexture
	{
		ComPtr<ID3D12Resource> texture;
		UINT srvOffset;
	};

	struct CardVertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT2 texCoords;
	};

	// In practics, the values that go into a constants buffer change each frame,
	// but they're called constants, because they're constant for the life of a
	// vertex or pixel shader in one particular draw call.
	struct CardConstantsBuffer
	{
		DirectX::XMMATRIX objToProj;		// This is an object-space to projection-space matrix.
		UINT8 pad[192];			// Pad the buffer so that its size is a multiple of 256 bytes.  This is a requirement.
	};

	HWND windowHandle;
	CD3DX12_VIEWPORT viewport;
	CD3DX12_RECT scissorRect;
	ComPtr<ID3D12Device> device;
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<IDXGISwapChain3> swapChain;
	std::vector<SwapFrame> swapFrameArray;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<ID3D12DescriptorHeap> srvHeap;
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12CommandAllocator> generalCommandAllocator;
	HANDLE generalFenceEvent;
	ComPtr<ID3D12Fence> generalFence;
	UINT64 generalCount;
	std::unordered_map<std::string, CardTexture> cardTextureMap;
	ComPtr<ID3D12Resource> cardVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW cardVertexBufferView;
	std::shared_ptr<SolitaireGame> cardGame;
	ComPtr<ID3D12Resource> cardConstantsBuffer;
	ComPtr<ID3D12DescriptorHeap> cbvHeap;
	UINT maxCardDrawCallsPerSwapFrame;
	UINT8* cardConstantsBufferPtr;
	DirectX::XMMATRIX worldToProj;
	Box worldExtents;
	Box adjustedWorldExtents;
	Box cardSize;
	Clock clock;
	std::list<double> tickTimeList;
	UINT64 tickCount;
	bool mouseCaptured;
	Clock cardsNeededClock;
};