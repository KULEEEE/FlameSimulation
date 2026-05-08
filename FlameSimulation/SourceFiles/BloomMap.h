#pragma once

#include "../Common/d3dUtil.h"

// Off-screen HDR render target for the bloom post-process pass.
// Sampled by PS_bloom (input ParticleMap → output bright-extracted + blurred
// halo into BloomMap), then sampled again by PS_map at composite time as
// an additive contribution onto the main backbuffer.
//
// Half-resolution (1/16 the area of ParticleMap) is enough — the blur
// kernel naturally smooths over per-texel detail and the result is sampled
// bilinearly in the composite shader, so no quality is lost.
class BloomMap
{
public:
    BloomMap(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format);

    BloomMap(const BloomMap& rhs)            = delete;
    BloomMap& operator=(const BloomMap& rhs) = delete;
    ~BloomMap()                              = default;

    ID3D12Resource*                Resource();
    CD3DX12_GPU_DESCRIPTOR_HANDLE  Srv();
    CD3DX12_CPU_DESCRIPTOR_HANDLE  Rtv();

    D3D12_VIEWPORT Viewport()    const;
    D3D12_RECT     ScissorRect() const;

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

    D3D12_VIEWPORT mViewport    {};
    D3D12_RECT     mScissorRect {};

    UINT        mWidth  = 0;
    UINT        mHeight = 0;
    DXGI_FORMAT mFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

    CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuRtv;

    Microsoft::WRL::ComPtr<ID3D12Resource> mBloomMap = nullptr;
};
