#include "BloomMap.h"

BloomMap::BloomMap(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format)
{
    md3dDevice = device;

    mWidth  = width;
    mHeight = height;
    mFormat = format;

    mViewport    = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
    mScissorRect = { 0, 0, (int)width, (int)height };

    BuildResource();
}

ID3D12Resource* BloomMap::Resource()
{
    return mBloomMap.Get();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE BloomMap::Srv()
{
    return mhGpuSrv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE BloomMap::Rtv()
{
    return mhCpuRtv;
}

D3D12_VIEWPORT BloomMap::Viewport() const
{
    return mViewport;
}

D3D12_RECT BloomMap::ScissorRect() const
{
    return mScissorRect;
}

void BloomMap::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
                                CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
                                CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv)
{
    mhCpuSrv = hCpuSrv;
    mhGpuSrv = hGpuSrv;
    mhCpuRtv = hCpuRtv;

    BuildDescriptors();
}

void BloomMap::OnResize(UINT newWidth, UINT newHeight)
{
    if ((mWidth != newWidth) || (mHeight != newHeight))
    {
        mWidth  = newWidth;
        mHeight = newHeight;

        BuildResource();
        BuildDescriptors();
    }
}

void BloomMap::BuildDescriptors()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format                  = mFormat;
    srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip     = 0;
    srvDesc.Texture2D.MipLevels           = 1;
    srvDesc.Texture2D.PlaneSlice          = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    md3dDevice->CreateShaderResourceView(mBloomMap.Get(), &srvDesc, mhCpuSrv);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension      = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Format             = mFormat;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;

    md3dDevice->CreateRenderTargetView(mBloomMap.Get(), &rtvDesc, mhCpuRtv);
}

void BloomMap::BuildResource()
{
    D3D12_CLEAR_VALUE optClear = {};
    optClear.Format = mFormat;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width              = mWidth;
    texDesc.Height             = mHeight;
    texDesc.DepthOrArraySize   = 1;
    texDesc.MipLevels          = 1;
    texDesc.Format             = mFormat;
    texDesc.SampleDesc.Count   = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &optClear,
        IID_PPV_ARGS(&mBloomMap)));
}
