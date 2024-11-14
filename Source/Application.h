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
#include "SolitaireGame.h"

using Microsoft::WRL::ComPtr;

#define WINDOW_CLASS_NAME		"SolitaireWindow"

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
	void RenderCard(const SolitaireGame::Card* card);

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

	HWND windowHandle;
	CD3DX12_VIEWPORT viewport;
	CD3DX12_RECT scissorRect;
	ComPtr<ID3D12Device> device;
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<IDXGISwapChain3> swapChain;
	SwapFrame swapFrame[2];
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
};