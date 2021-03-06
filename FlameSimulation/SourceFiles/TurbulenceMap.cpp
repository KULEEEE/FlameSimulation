
#include "TurbulenceMap.h"

TurbulenceMap::TurbulenceMap(ID3D12Device* device,
	UINT width, UINT height,
	DXGI_FORMAT format)
{
	md3dDevice = device;

	mWidth = width;
	mHeight = height;
	mFormat = format;

	mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	mScissorRect = { 0, 0, (int)width, (int)height };

	BuildResource();
}

ID3D12Resource* TurbulenceMap::Resource()
{
	return mTurbulenceMap.Get();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE TurbulenceMap::Srv()
{
	return mhGpuSrv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE TurbulenceMap::Rtv()
{
	return mhCpuRtv;
}

D3D12_VIEWPORT TurbulenceMap::Viewport()const
{
	return mViewport;
}

D3D12_RECT TurbulenceMap::ScissorRect()const
{
	return mScissorRect;
}

void TurbulenceMap::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv)
{
	// Save references to the descriptors. 
	mhCpuSrv = hCpuSrv;
	mhGpuSrv = hGpuSrv;
	mhCpuRtv = hCpuRtv;

	//  Create the descriptors
	BuildDescriptors();
}

void TurbulenceMap::OnResize(UINT newWidth, UINT newHeight)
{
	if ((mWidth != newWidth) || (mHeight != newHeight))
	{
		mWidth = newWidth;
		mHeight = newHeight;

		BuildResource();

		// New resource, so we need new descriptors to that resource.
		BuildDescriptors();
	}
}

void TurbulenceMap::BuildDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = 1;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

	// Create SRV
	md3dDevice->CreateShaderResourceView(mTurbulenceMap.Get(), &srvDesc, mhCpuSrv);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = mFormat;
	rtvDesc.Texture2DArray.MipSlice = 0;
	rtvDesc.Texture2DArray.PlaneSlice = 0;

	// Render target to ith element.
	rtvDesc.Texture2DArray.FirstArraySlice = 0;

	// Only view one element of the array.
	rtvDesc.Texture2DArray.ArraySize = 1;

	// Create RTV to ith cubemap face.
	md3dDevice->CreateRenderTargetView(mTurbulenceMap.Get(), &rtvDesc, mhCpuRtv);
}

void TurbulenceMap::BuildResource()
{
	// Note, compressed formats cannot be used for UAV.  We get error like:
	// ERROR: ID3D11Device::CreateTexture2D: The format (0x4d, BC3_UNORM) 
	// cannot be bound as an UnorderedAccessView, or cast to a format that
	// could be bound as an UnorderedAccessView.  Therefore this format 
	// does not support D3D11_BIND_UNORDERED_ACCESS.

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mFormat;
	optClear.Color[0] = 0.f;
	optClear.Color[1] = 0.f;
	optClear.Color[2] = 0.f;
	optClear.Color[3] = 0.f;

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&mTurbulenceMap)));
}