#pragma once

#include "../Common/d3dUtil.h"

class TurbulenceMap
{
public:
	TurbulenceMap(ID3D12Device* device,
		UINT width, UINT height,
		DXGI_FORMAT format);

	TurbulenceMap(const TurbulenceMap& rhs) = delete;
	TurbulenceMap& operator=(const TurbulenceMap& rhs) = delete;
	~TurbulenceMap() = default;

	ID3D12Resource* Resource();
	CD3DX12_GPU_DESCRIPTOR_HANDLE Srv();
	CD3DX12_CPU_DESCRIPTOR_HANDLE Rtv();

	D3D12_VIEWPORT Viewport()const;
	D3D12_RECT ScissorRect()const;

	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv);

	void OnResize(UINT newWidth, UINT newHeight);

private:
	void BuildDescriptors();
	void BuildResource();

private:

	ID3D12Device* md3dDevice = nullptr;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuRtv;

	Microsoft::WRL::ComPtr<ID3D12Resource> mTurbulenceMap = nullptr;
};

#pragma once
