#pragma once

#include <d3d12.h>
#include <DirectX/d3dx12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include "Utils.h"

#include "UploadBuffer.h"


// Stores the resources needed for the CPU to build the command lists for a frame.
// The contents here will vary from app to app based on the needed resources.
struct FrameResource
{
public:
	FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount = 1, UINT waveVertCount = 1);
	FrameResource(const FrameResource& other) = delete;
	FrameResource& operator=(const FrameResource& other) = delete;
	~FrameResource();

	// We cannot reset the allocator until the GPU is done processing the commands.
	// So each frame needs their own allocator.
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

	// We cannot update a constant  buffer until the GPU is done processing the commands that reference it.
	// So each frame needs their own constant buffers
	std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
	std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

	// Since lightning
	std::unique_ptr<UploadBuffer<MaterialConstants>> MaterialCB = nullptr;

	// Temp value for DrawingD3DAppIII
	// We cannot update a dynamic vertex buffer untill the GPU is done processing
	// the commands that reference it. So each frame needs their own
	//std::unique_ptr<UploadBuffer<Vertex>> WavesVB = nullptr;

	// Temp value for LightningWaves
	std::unique_ptr<UploadBuffer<LightningVertex>> WavesVB = nullptr;

	// Frence value to mark commands up to this fence point.
	// This lets us check if these frame resources are still in use by the GPU.
	UINT64 Fence = 0;
};