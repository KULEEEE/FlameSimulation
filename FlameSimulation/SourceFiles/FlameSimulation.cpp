#include "../Common/d3dApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"
#include "../Common/GeometryGenerator.h"
#include "../Common/Camera.h"
#include "FrameResource.h"
#include "Particle.h"

#include "ParticleMap.h"
#include "TurbulenceMap.h"


using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

const int gNumFrameResources = 3;

struct RenderItem
{
	RenderItem() = default;

	XMFLOAT4X4 World = MathHelper::Identity4x4();

	int NumFramesDirty = gNumFrameResources;

	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	MeshGeometry* Geo = nullptr;

	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};


struct FrameContext
{
	ID3D12CommandAllocator* CommandAllocator;
	UINT64                  FenceValue;
};

class Flame : public D3DApp
{
public:
	Flame(HINSTANCE hInstance);
	Flame(const Flame& rhs) = delete;
	Flame& operator=(const Flame& rhs) = delete;
	~Flame();

	virtual bool Initialize()override;

private:
	virtual void CreateRtvAndDsvDescriptorHeaps()override;
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void UpdateImGui(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateParticles(const GameTimer& gt);
	void SunLight(const GameTimer& gt);
	void LoadTextures();

	void BuildDescriptorHeaps();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildDepthStencil();
	void BuildParticlesGeometry();
	
	void BuildBox();
	void BuildBoundary();
	void BuildMap();
	void BuildTmap();
	void BuildLandGeometry();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildImGui();
	void BuildMaterials();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
	void DrawSceneToParticleMap();
	void DrawSceneToTmap();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

	float GetHillsHeight(float x, float z)const;
	XMFLOAT3 GetHillsNormal(float x, float z)const;

private:

	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource;
	int mCurrFrameResourceIndex = 0;

	UINT mCbvSrvDescriptorSize = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
	
	ComPtr<ID3D12Resource> mDepthStencilBuffer;

	std::unique_ptr<ParticleMap> mParticleMap = nullptr;
	std::unique_ptr<TurbulenceMap> mTurbluenceMap = nullptr;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mDSV;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	ComPtr<ID3D12PipelineState> mOpaquePSO = nullptr;
	ComPtr<ID3D12PipelineState> mBoundaryPSO = nullptr;
	ComPtr<ID3D12PipelineState> mMapPSO = nullptr;
	ComPtr<ID3D12PipelineState> mTmapPSO = nullptr;
	ComPtr<ID3D12PipelineState> mTransPSO = nullptr;
	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	RenderItem* mParticleItem;

	RenderItem* mGridItem;
	// Render items divided by PSO.
	std::vector<RenderItem*> mMapRitems;
	std::vector<RenderItem*> mTmapRitems;
	std::vector<RenderItem*> mOpaqueRitems;
	std::vector<RenderItem*> mBoundaryRitems;
	std::vector<RenderItem*> mBoxRitems;
	std::vector<RenderItem*> mTransRitems;
	
	

	std::unique_ptr<Particles>mParticles;
	

	PassConstants mMainPassCB;

	Camera mCamera;



	XMFLOAT3 initialPoint = XMFLOAT3(0.0f,0.0f,0.0f);

	float mTheta = 1.5f * XM_PI;
	float mPhi = 0.5f * XM_PI;
	float mRadius = 15.0f;
	float mParticleSize = 0.11f;
	float mBlur = 0.015f;

	float mTemp = 0.15f;
	float mWidth = 1.0f;


	int mTexture = 2;
	static const int mGridSize = 10;

	bool isgui = false;
	bool isBound = false;
	float mSunTheta = 1.25f * XM_PI;
	float mSunPhi = XM_PIDIV4;

	int mDensityField[mGridSize * mGridSize * mGridSize];

	POINT mLastMousePos;
};


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd){
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		Flame theApp(hInstance);
		if (!theApp.Initialize())
			return 0;


		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}

}

Flame::Flame(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

Flame::~Flame()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

bool Flame::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Get the increment size of a descriptor in this heap type.  This is hardware specific, 
	// so we have to query this information.
	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	mParticleMap = std::make_unique<ParticleMap>(md3dDevice.Get(),
		2048, 2048, DXGI_FORMAT_R16G16B16A16_FLOAT);

	mTurbluenceMap = std::make_unique<TurbulenceMap>(md3dDevice.Get(),
		2048, 2048, DXGI_FORMAT_R16G16B16A16_FLOAT);

	mParticles = std::make_unique<Particles>(8000, 0.03f, mParticleSize,initialPoint);
	
	mCamera.Walk(-10.0f);
	mCamera.UpdateViewMatrix();


	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildDepthStencil();
	BuildShadersAndInputLayout();
	BuildParticlesGeometry();
	
	BuildBox();
	BuildBoundary();
	BuildMap();
	BuildTmap();
	BuildLandGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();
	BuildImGui();

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void Flame::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount + 2;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	// Add +1 DSV for shadow map.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 3;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));

	mDSV = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mDsvHeap->GetCPUDescriptorHandleForHeapStart(),
		2,
		mDsvDescriptorSize);
}

void Flame::OnResize()
{
	D3DApp::OnResize();

	mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}


void Flame::Update(const GameTimer& gt)
{
	UpdateImGui(gt);
	UpdateCamera(gt);

	// Cycle through the circular frame resource array.
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
	

	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
	UpdateParticles(gt);
	SunLight(gt);
}



void Flame::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mOpaquePSO.Get()));

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	DrawSceneToTmap();
	DrawSceneToParticleMap();
	

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	FLOAT White[4] = { 1.0f, 1.0f, 1.0f, 0.0f };
	FLOAT Black[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Black, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());


	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());
	

	mCommandList->SetPipelineState(mOpaquePSO.Get());
	DrawRenderItems(mCommandList.Get(), mOpaqueRitems);
	
	
	if(isBound)
	{
		mCommandList->SetPipelineState(mBoundaryPSO.Get());
		DrawRenderItems(mCommandList.Get(), mBoundaryRitems);
	}
	mCommandList->SetPipelineState(mMapPSO.Get());
	DrawRenderItems(mCommandList.Get(), mMapRitems);
	
	
	ImGui::Render();
	if(isgui)
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
	
}

void Flame::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void Flame::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void Flame::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0 && !isgui)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		mCamera.Pitch(dy);
		mCamera.RotateY(dx);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void Flame::UpdateCamera(const GameTimer& gt)
{
	const float dt = gt.DeltaTime();

	if (GetAsyncKeyState('W') & 0x8000)
	{
		float temp = 5.0f;
		if (GetAsyncKeyState(0xA0))
			temp *= 2;
		mCamera.Walk(temp * dt);
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		float temp = 5.0f;
		if (GetAsyncKeyState(0xA0))
			temp *= 2;
		mCamera.Walk(-temp * dt);
	}

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera.Strafe(-5.0f * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera.Strafe(5.0f * dt);

	if (GetAsyncKeyState('Q') & 0x8000)
		mCamera.Up(-5.0f * dt);

	if (GetAsyncKeyState('E') & 0x8000)
		mCamera.Up(5.0f * dt);


	mCamera.UpdateViewMatrix();
	isgui = GetKeyState('P');
	
}

void Flame::UpdateImGui(const GameTimer& gt)
{
	
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("Source Controller");
	ImGui::Text("Control flame's feature");
	ImGui::SliderFloat("Particle Size", &mParticleSize, 0.0f, 0.5f);
	ImGui::SliderFloat("Flame width", &mTemp, 0.0f, 1.0f);
	ImGui::SliderFloat("Flame Direction", &mWidth, -10.0f, 10.0f);
	ImGui::SliderFloat("Turbulence Strength", &mBlur, 0.001f, 0.999f);
	ImGui::Checkbox("Boundary box", &isBound);
	if (ImGui::Button("Texture"))
	{
		if (mTexture == 3)
			mTexture = 0;
		else
			mTexture++;
	}
	ImGui::SameLine();
	ImGui::Text("Texture num = %d", mTexture+1);
	if (ImGui::Button("Initialize"))
	{
		mParticleSize = 0.11f;
		mBlur = 0.015f;

		mTemp = 0.15f;
		mWidth = 1.0f;
		mTexture = 2;
	}
	ImGui::End();
}



void Flame::SunLight(const GameTimer& gt)
{
	const float dt = gt.DeltaTime();

	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
		mSunTheta -= 1.0f * dt;

	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
		mSunTheta += 1.0f * dt;

	if (GetAsyncKeyState(VK_UP) & 0x8000)
		mSunPhi -= 1.0f * dt;

	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
		mSunPhi += 1.0f * dt;

	mSunPhi = MathHelper::Clamp(mSunPhi, 0.1f, XM_PIDIV2);
}


void Flame::UpdateObjectCBs(const GameTimer& gt)
{

	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (e->NumFramesDirty > 0)
		{

			XMMATRIX world = XMLoadFloat4x4(&e->World);

			ObjectConstants objConstants;
			
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}	
}

void Flame::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void Flame::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = mCamera.GetView();
	XMMATRIX proj = mCamera.GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mCamera.GetPosition3f();
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };

	XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);

	XMStoreFloat3(&mMainPassCB.Lights[0].Direction, lightDir);
	mMainPassCB.Lights[0].Strength = { 1.0f, 1.0f, 0.9f };
	mMainPassCB.Lights[0].SpotPower = 1.0f;
	//mMainPassCB.Lights[1].Position = XMFLOAT3(0.0f, 0.5f, 0.0f);
	//mMainPassCB.Lights[1].Strength = { 1.0f, 1.0f, 0.9f };
	//mMainPassCB.Lights[2].Position = XMFLOAT3(0.0f, 1.5f, 0.0f);
	//mMainPassCB.Lights[2].Strength = { 1.0f, 1.0f, 0.9f };
	mMainPassCB.initialPoint = initialPoint;
	mMainPassCB.blur = mBlur;
	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
	
}

void Flame::UpdateParticles(const GameTimer& gt)
{
	mParticles->Update(gt.DeltaTime(), mCamera.GetPosition(), mCamera.GetUp(),mParticleSize,mTemp,mWidth);
	//mParticles2->Update(gt.DeltaTime(), mCamera.GetPosition(), mCamera.GetUp(), mParticleSize, mTemp, mWidth);
	auto currParticleVB = mCurrFrameResource->ParticleVB.get();
	auto currGridVB = mCurrFrameResource->GridVB.get();
	
	int vertexCountperParticle = mParticles->VertexCount() / mParticles->Num();
	
	

	for (int j = 0; j < mParticles->VertexCount(); ++j)
	{
		Vertex v;
		v.Pos = XMFLOAT3(XMVectorGetX(mParticles->Position(j / vertexCountperParticle, j % vertexCountperParticle)), XMVectorGetY(mParticles->Position(j / vertexCountperParticle, j % vertexCountperParticle)), XMVectorGetZ(mParticles->Position(j / vertexCountperParticle, j % vertexCountperParticle)));
		v.TexCoord = mParticles->Texcoord(j / vertexCountperParticle, j % vertexCountperParticle);
		v.Lifespan = mParticles->Lifespan(j / vertexCountperParticle);
		v.Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		currParticleVB->CopyData(j, v);
	}

	

	//make densityfield
	
	//mGridItem->Geo->VertexBufferGPU = currGridVB->Resource();
	mParticleItem->Geo->VertexBufferGPU = currParticleVB->Resource();
	mParticleItem->Mat->DiffuseSrvHeapIndex = mTexture;
	
}




void Flame::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0,0);
	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Create root CBV.
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter, (UINT)staticSamplers.size(), staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void Flame::LoadTextures()
{
	auto particle1Tex = std::make_unique<Texture>();
	particle1Tex->Name = "particle1Tex";
	particle1Tex->Filename = L"../Texture\\particle1.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), particle1Tex->Filename.c_str(),
		particle1Tex->Resource, particle1Tex->UploadHeap));

	mTextures[particle1Tex->Name] = std::move(particle1Tex);

	auto particle2Tex = std::make_unique<Texture>();
	particle2Tex->Name = "particle2Tex";
	particle2Tex->Filename = L"../Texture\\particle2.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), particle2Tex->Filename.c_str(),
		particle2Tex->Resource, particle2Tex->UploadHeap));

	mTextures[particle2Tex->Name] = std::move(particle2Tex);

	auto particle3Tex = std::make_unique<Texture>();
	particle3Tex->Name = "particle3Tex";
	particle3Tex->Filename = L"../Texture\\particle3.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), particle3Tex->Filename.c_str(),
		particle3Tex->Resource, particle3Tex->UploadHeap));

	mTextures[particle3Tex->Name] = std::move(particle3Tex);

	auto particle4Tex = std::make_unique<Texture>();
	particle4Tex->Name = "particle4Tex";
	particle4Tex->Filename = L"../Texture\\particle4.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), particle4Tex->Filename.c_str(),
		particle4Tex->Resource, particle4Tex->UploadHeap));

	mTextures[particle4Tex->Name] = std::move(particle4Tex);


	auto boxTex = std::make_unique<Texture>();
	boxTex->Name = "boxTex";
	boxTex->Filename = L"../Texture\\box.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), boxTex->Filename.c_str(),
		boxTex->Resource, boxTex->UploadHeap));

	mTextures[boxTex->Name] = std::move(boxTex);

	auto grassTex = std::make_unique<Texture>();
	grassTex->Name = "grassTex";
	grassTex->Filename = L"../Texture\\grass.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), grassTex->Filename.c_str(),
		grassTex->Resource, grassTex->UploadHeap));

	mTextures[grassTex->Name] = std::move(grassTex);
}

void Flame::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 8;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	//
	// Fill out the heap with actual descriptors.
	//
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	auto particle1Tex = mTextures["particle1Tex"]->Resource;
	auto particle2Tex = mTextures["particle2Tex"]->Resource;
	auto particle3Tex = mTextures["particle3Tex"]->Resource;
	auto particle4Tex = mTextures["particle4Tex"]->Resource;
	auto boxTex = mTextures["boxTex"]->Resource;
	auto grassTex = mTextures["grassTex"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = particle1Tex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	md3dDevice->CreateShaderResourceView(particle1Tex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	md3dDevice->CreateShaderResourceView(particle2Tex.Get(), &srvDesc, hDescriptor);
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	md3dDevice->CreateShaderResourceView(particle3Tex.Get(), &srvDesc, hDescriptor);
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	md3dDevice->CreateShaderResourceView(particle4Tex.Get(), &srvDesc, hDescriptor);
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = boxTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(boxTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = grassTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(grassTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	auto srvCpuStart = mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto srvGpuStart = mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	auto rtvCpuStart = mRtvHeap->GetCPUDescriptorHandleForHeapStart();

	mParticleMap->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, 6, mCbvSrvUavDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, 6, mCbvSrvUavDescriptorSize),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvCpuStart, SwapChainBufferCount, mRtvDescriptorSize));

	mTurbluenceMap->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, 7, mCbvSrvUavDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, 7, mCbvSrvUavDescriptorSize),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvCpuStart, SwapChainBufferCount+1, mRtvDescriptorSize));

}

void Flame::BuildDepthStencil()
{
	UINT64 MapSize = 2048;
	// Create the depth/stencil buffer and view.
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = MapSize;
	depthStencilDesc.Height = MapSize;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = mDepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, mDSV);

	// Transition the resource from its initial state to be used as a depth buffer.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
}

void Flame::BuildShadersAndInputLayout()
{
	

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders/Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["particleVS"] = d3dUtil::CompileShader(L"Shaders/Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["standardPS"] = d3dUtil::CompileShader(L"Shaders/Default.hlsl", nullptr, "PS", "ps_5_1");
	mShaders["mapVS"] = d3dUtil::CompileShader(L"Shaders/Default.hlsl", nullptr, "VS_map", "vs_5_1");
	mShaders["mapPS"] = d3dUtil::CompileShader(L"Shaders/Default.hlsl", nullptr, "PS_map", "ps_5_1");
	mShaders["tmapVS"] = d3dUtil::CompileShader(L"Shaders/Default.hlsl", nullptr, "VS_tmap", "vs_5_1");
	mShaders["tmapPS"] = d3dUtil::CompileShader(L"Shaders/Default.hlsl", nullptr, "PS_tmap", "ps_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders/Default.hlsl", nullptr, "PS2", "ps_5_1");
	mShaders["boundaryVS"] = d3dUtil::CompileShader(L"Shaders/Default.hlsl", nullptr, "VS_boundary", "vs_5_1");
	mShaders["boundaryPS"] = d3dUtil::CompileShader(L"Shaders/Default.hlsl", nullptr, "PS_boundary", "ps_5_1");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"LIFESPAN",0,DXGI_FORMAT_R32_FLOAT,0,20,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};
}

void Flame::BuildParticlesGeometry()
{
	
	std::vector<std::uint16_t> indices;

	for (int i = 0; i < mParticles->Num(); i++)
	{

		indices.push_back(0 + i * 4);
		indices.push_back(3 + i * 4);
		indices.push_back(1 + i * 4);
		indices.push_back(0 + i * 4);
		indices.push_back(2 + i * 4);
		indices.push_back(3 + i * 4);
	}

	UINT vbByteSize = mParticles->VertexCount() * sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "particleGeo";

	geo->VertexBufferCPU = nullptr;
	geo->VertexBufferGPU = nullptr;

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["particle"] = submesh;

	mGeometries["particles"] = std::move(geo);
}



void Flame::BuildBox()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(0.10f, 0.10f, 0.7f, 3);


	std::vector<Vertex> vertices(box.Vertices.size());

	for (size_t i = 0; i < box.Vertices.size(); ++i)
	{
		auto& p = box.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].TexCoord = box.Vertices[i].TexC;
		vertices[i].Lifespan = 1.0f;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = box.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["box"] = submesh;

	mGeometries["boxGeo"] = std::move(geo);
}

void Flame::BuildBoundary()
{
	std::vector<Vertex> vertices(8);
	float w, d, h;
	w = 1.0f;
	d = 1.0f;
	h = 2.5f;
	vertices[0].Pos=XMFLOAT3(-w/2,0.0f,-d/2);
	vertices[1].Pos = XMFLOAT3(w/2, 0.0f, -d/2);
	vertices[2].Pos = XMFLOAT3(-w/2, 0.0f, d/2);
	vertices[3].Pos = XMFLOAT3(w/2, 0.0f, d/2);
	vertices[4].Pos = XMFLOAT3(-w/2, h, -d/2);
	vertices[5].Pos = XMFLOAT3(w/2, h, -d/2);
	vertices[6].Pos = XMFLOAT3(-w/2, h, d/2);
	vertices[7].Pos = XMFLOAT3(w/2, h, d/2);

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices;
	for (int i = 0; i < 8; i++)
	{
		indices.push_back(0);
		indices.push_back(1);

		indices.push_back(0);
		indices.push_back(2);

		indices.push_back(1);
		indices.push_back(3);

		indices.push_back(2);
		indices.push_back(3);

		indices.push_back(4);
		indices.push_back(5);

		indices.push_back(4);
		indices.push_back(6);

		indices.push_back(5);
		indices.push_back(7);

		indices.push_back(6);
		indices.push_back(7);

		indices.push_back(0);
		indices.push_back(4);

		indices.push_back(1);
		indices.push_back(5);

		indices.push_back(2);
		indices.push_back(6);

		indices.push_back(3);
		indices.push_back(7);
	}
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "boundaryGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["boundary"] = submesh;

	mGeometries["boundaryGeo"] = std::move(geo);
}

void Flame::BuildMap()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData map = geoGen.CreateQuad(-1.0f, 1.0f, 2.0f, 2.0f,0.0f);

	std::vector<Vertex> vertices(map.Vertices.size());

	for (size_t i = 0; i < map.Vertices.size(); ++i)
	{
		auto& p = map.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].TexCoord = map.Vertices[i].TexC;
		vertices[i].Lifespan = 1.0f;
		vertices[i].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = map.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "mapGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["map"] = submesh;

	mGeometries["mapGeo"] = std::move(geo);
}

void Flame::BuildTmap()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData map = geoGen.CreateQuad(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f);

	std::vector<Vertex> vertices(map.Vertices.size());

	for (size_t i = 0; i < map.Vertices.size(); ++i)
	{
		auto& p = map.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].TexCoord = map.Vertices[i].TexC;
		vertices[i].Lifespan = 1.0f;
		vertices[i].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = map.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "fieldGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["field"] = submesh;

	mGeometries["fieldGeo"] = std::move(geo);
}

void Flame::BuildLandGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(300.0f, 300.0f, 50, 50);

	//
	// Extract the vertex elements we are interested and apply the height function to
	// each vertex.  In addition, color the vertices based on their height so we have
	// sandy looking beaches, grassy low hills, and snow mountain peaks.
	//

	std::vector<Vertex> vertices(grid.Vertices.size());
	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		auto& p = grid.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Pos.y = GetHillsHeight(p.x, p.z);
		vertices[i].TexCoord = grid.Vertices[i].TexC;
		vertices[i].Lifespan = 1.0f;
		vertices[i].Normal = GetHillsNormal(p.x, p.z);
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = grid.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "landGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	mGeometries["landGeo"] = std::move(geo);
}

void Flame::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mOpaquePSO)));


	D3D12_GRAPHICS_PIPELINE_STATE_DESC boundaryPsoDesc;

	ZeroMemory(&boundaryPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	boundaryPsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	boundaryPsoDesc.pRootSignature = mRootSignature.Get();
	boundaryPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["boundaryVS"]->GetBufferPointer()),
		mShaders["boundaryVS"]->GetBufferSize()
	};
	boundaryPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["boundaryPS"]->GetBufferPointer()),
		mShaders["boundaryPS"]->GetBufferSize()
	};
	boundaryPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	boundaryPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	boundaryPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	boundaryPsoDesc.SampleMask = UINT_MAX;
	boundaryPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	boundaryPsoDesc.NumRenderTargets = 1;
	boundaryPsoDesc.RTVFormats[0] = mBackBufferFormat;
	boundaryPsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	boundaryPsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	boundaryPsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&boundaryPsoDesc, IID_PPV_ARGS(&mBoundaryPSO)));

	
	
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc;
	ZeroMemory(&transparentPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	transparentPsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	transparentPsoDesc.pRootSignature = mRootSignature.Get();
	transparentPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["particleVS"]->GetBufferPointer()),
		mShaders["particleVS"]->GetBufferSize()
	};
	transparentPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardPS"]->GetBufferPointer()),
		mShaders["standardPS"]->GetBufferSize()
	};
	transparentPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	transparentPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	transparentPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(TRUE, D3D12_DEPTH_WRITE_MASK_ZERO, D3D12_COMPARISON_FUNC_LESS, FALSE, D3D12_DEFAULT_STENCIL_READ_MASK, D3D12_DEFAULT_STENCIL_WRITE_MASK,
	D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS);
	transparentPsoDesc.SampleMask = UINT_MAX;
	transparentPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	transparentPsoDesc.NumRenderTargets = 1;
	transparentPsoDesc.RTVFormats[0] = mBackBufferFormat;
	transparentPsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	transparentPsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	transparentPsoDesc.DSVFormat = mDepthStencilFormat;

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_ONE;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mTransPSO)));


	D3D12_GRAPHICS_PIPELINE_STATE_DESC mapPsoDesc;
	ZeroMemory(&mapPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	mapPsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	mapPsoDesc.pRootSignature = mRootSignature.Get();
	mapPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["mapVS"]->GetBufferPointer()),
		mShaders["mapVS"]->GetBufferSize()
	};
	mapPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["mapPS"]->GetBufferPointer()),
		mShaders["mapPS"]->GetBufferSize()
	};
	mapPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	mapPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	mapPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	mapPsoDesc.SampleMask = UINT_MAX;
	mapPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	mapPsoDesc.NumRenderTargets = 1;
	mapPsoDesc.RTVFormats[0] = mBackBufferFormat;
	mapPsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	mapPsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	mapPsoDesc.DSVFormat = mDepthStencilFormat;

	D3D12_RENDER_TARGET_BLEND_DESC mapBlendDesc;
	mapBlendDesc.BlendEnable = true;
	mapBlendDesc.LogicOpEnable = false;
	mapBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	mapBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	mapBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	mapBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	mapBlendDesc.DestBlendAlpha = D3D12_BLEND_ONE;
	mapBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_MAX;
	mapBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	mapBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	
	mapPsoDesc.BlendState.RenderTarget[0] = mapBlendDesc;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&mapPsoDesc, IID_PPV_ARGS(&mMapPSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC tmapPsoDesc;
	ZeroMemory(&tmapPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	tmapPsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	tmapPsoDesc.pRootSignature = mRootSignature.Get();
	tmapPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["tmapVS"]->GetBufferPointer()),
		mShaders["tmapVS"]->GetBufferSize()
	};
	tmapPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["tmapPS"]->GetBufferPointer()),
		mShaders["tmapPS"]->GetBufferSize()
	};
	tmapPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	tmapPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	tmapPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(TRUE, D3D12_DEPTH_WRITE_MASK_ZERO, D3D12_COMPARISON_FUNC_LESS, FALSE, D3D12_DEFAULT_STENCIL_READ_MASK, D3D12_DEFAULT_STENCIL_WRITE_MASK,
		D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS);
	tmapPsoDesc.SampleMask = UINT_MAX;
	tmapPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	tmapPsoDesc.NumRenderTargets = 1;
	tmapPsoDesc.RTVFormats[0] = mBackBufferFormat;
	tmapPsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	tmapPsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	tmapPsoDesc.DSVFormat = mDepthStencilFormat;

	D3D12_RENDER_TARGET_BLEND_DESC tmapBlendDesc;
	tmapBlendDesc.BlendEnable = true;
	tmapBlendDesc.LogicOpEnable = false;
	tmapBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	tmapBlendDesc.DestBlend = D3D12_BLEND_ONE;
	tmapBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	tmapBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	tmapBlendDesc.DestBlendAlpha = D3D12_BLEND_ONE;
	tmapBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	tmapBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	tmapBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	tmapPsoDesc.BlendState.RenderTarget[0] = tmapBlendDesc;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&tmapPsoDesc, IID_PPV_ARGS(&mTmapPSO)));
}

void Flame::BuildFrameResources()
{
	
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(), /*PSO Count*/ 2,(UINT)mAllRitems.size(), (UINT)mMaterials.size(), (UINT)mParticles->VertexCount(),mGridSize*mGridSize*mGridSize));
	}
	
}

void Flame::BuildImGui()
{

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui_ImplDX12_Init(md3dDevice.Get(), 3,
		DXGI_FORMAT_R8G8B8A8_UNORM, mSrvDescriptorHeap.Get(),
		mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

void Flame::BuildMaterials()
{

	auto particle = std::make_unique<Material>();
	particle->Name = "particle";
	particle->MatCBIndex = 0;
	particle->DiffuseSrvHeapIndex = 0;
	particle->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	particle->FresnelR0 = XMFLOAT3(0.0f, 0.0f, 0.0f);
	particle->Roughness = 0.0f;

	mMaterials["particle"] = std::move(particle);

	auto box = std::make_unique<Material>();
	box->Name = "box";
	box->MatCBIndex = 1;
	box->DiffuseSrvHeapIndex = 5;
	box->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	box->FresnelR0 = XMFLOAT3(0.0f, 0.0f, 0.0f);
	box->Roughness = 0.0f;

	mMaterials["box"] = std::move(box);

	auto grass = std::make_unique<Material>();
	grass->Name = "grass";
	grass->MatCBIndex = 2;
	grass->DiffuseSrvHeapIndex = 5;
	grass->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass->Roughness = 0.125f;

	mMaterials["grass"] = std::move(grass);


	auto map = std::make_unique<Material>();
	map->Name = "map";
	map->MatCBIndex = 3;
	map->DiffuseSrvHeapIndex = 6;
	map->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	map->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	map->Roughness = 0.125f;

	mMaterials["map"] = std::move(map);

	auto tmap = std::make_unique<Material>();
	tmap->Name = "tmap";
	tmap->MatCBIndex = 4;
	tmap->DiffuseSrvHeapIndex = 7;
	tmap->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	tmap->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	tmap->Roughness = 0.125f;

	mMaterials["tmap"] = std::move(tmap);
}

void Flame::BuildRenderItems()
{
	auto particlesRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&particlesRitem->World,XMMatrixScaling(1.0f,1.0f,1.0f)*XMMatrixTranslation(0.0f,0.2f,0.0f));
	particlesRitem->ObjCBIndex = 0;
	particlesRitem->Mat = mMaterials["particle"].get();
	particlesRitem->Geo = mGeometries["particles"].get();
	particlesRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	particlesRitem->IndexCount = particlesRitem->Geo->DrawArgs["particle"].IndexCount;
	particlesRitem->StartIndexLocation = particlesRitem->Geo->DrawArgs["particle"].StartIndexLocation;
	particlesRitem->BaseVertexLocation = particlesRitem->Geo->DrawArgs["particle"].BaseVertexLocation;
	mParticleItem = particlesRitem.get();
	
	mTransRitems.push_back(particlesRitem.get());
	mAllRitems.push_back(std::move(particlesRitem));

	


	auto boxRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation(0.0f, 0.2f, 0.0f));
	boxRitem->ObjCBIndex = 1;
	boxRitem->Mat = mMaterials["box"].get();
	boxRitem->Geo = mGeometries["boxGeo"].get();
	boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;

	mBoxRitems.push_back(boxRitem.get());
	mAllRitems.push_back(std::move(boxRitem));

	auto boxRitem2 = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&boxRitem2->World, XMMatrixTranslation(0.0f, 0.3f, 0.0f) *XMMatrixRotationY(90.0f));
	boxRitem2->ObjCBIndex = 2;
	boxRitem2->Mat = mMaterials["box"].get();
	boxRitem2->Geo = mGeometries["boxGeo"].get();
	boxRitem2->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem2->IndexCount = boxRitem2->Geo->DrawArgs["box"].IndexCount;
	boxRitem2->StartIndexLocation = boxRitem2->Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem2->BaseVertexLocation = boxRitem2->Geo->DrawArgs["box"].BaseVertexLocation;

	mBoxRitems.push_back(boxRitem2.get());
	mAllRitems.push_back(std::move(boxRitem2));

	auto boxRitem3 = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&boxRitem3->World, XMMatrixTranslation(0.0f, 0.4f, 0.0f) * XMMatrixRotationY(120.0f));
	boxRitem3->ObjCBIndex = 3;
	boxRitem3->Mat = mMaterials["box"].get();
	boxRitem3->Geo = mGeometries["boxGeo"].get();
	boxRitem3->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem3->IndexCount = boxRitem3->Geo->DrawArgs["box"].IndexCount;
	boxRitem3->StartIndexLocation = boxRitem3->Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem3->BaseVertexLocation = boxRitem3->Geo->DrawArgs["box"].BaseVertexLocation;

	mBoxRitems.push_back(boxRitem3.get());
	mAllRitems.push_back(std::move(boxRitem3));

	auto boundaryRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&boundaryRitem->World, XMMatrixTranslation(0.0f, 0.0f, 0.0f));
	boundaryRitem->ObjCBIndex = 4;
	boundaryRitem->Mat = mMaterials["box"].get();
	boundaryRitem->Geo = mGeometries["boundaryGeo"].get();
	boundaryRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	boundaryRitem->IndexCount = boundaryRitem->Geo->DrawArgs["boundary"].IndexCount;
	boundaryRitem->StartIndexLocation = boundaryRitem->Geo->DrawArgs["boundary"].StartIndexLocation;
	boundaryRitem->BaseVertexLocation = boundaryRitem->Geo->DrawArgs["boundary"].BaseVertexLocation;

	mBoundaryRitems.push_back(boundaryRitem.get());
	mAllRitems.push_back(std::move(boundaryRitem));

	auto mapRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&mapRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	mapRitem->ObjCBIndex = 5;
	mapRitem->Mat = mMaterials["map"].get();
	mapRitem->Geo = mGeometries["mapGeo"].get();
	mapRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mapRitem->IndexCount = mapRitem->Geo->DrawArgs["map"].IndexCount;
	mapRitem->StartIndexLocation = mapRitem->Geo->DrawArgs["map"].StartIndexLocation;
	mapRitem->BaseVertexLocation = mapRitem->Geo->DrawArgs["map"].BaseVertexLocation;

	mMapRitems.push_back(mapRitem.get());
	mAllRitems.push_back(std::move(mapRitem));



	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->World = MathHelper::Identity4x4();
	gridRitem->ObjCBIndex = 6;
	gridRitem->Mat = mMaterials["grass"].get();
	gridRitem->Geo = mGeometries["landGeo"].get();
	gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
	
	mOpaqueRitems.push_back(gridRitem.get());
	mAllRitems.push_back(std::move(gridRitem));

	auto fieldRitem = std::make_unique<RenderItem>();
	fieldRitem->World = MathHelper::Identity4x4();
	fieldRitem->ObjCBIndex = 7;
	fieldRitem->Mat = mMaterials["map"].get();
	fieldRitem->Geo = mGeometries["fieldGeo"].get();
	fieldRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	fieldRitem->IndexCount = fieldRitem->Geo->DrawArgs["field"].IndexCount;
	fieldRitem->StartIndexLocation = fieldRitem->Geo->DrawArgs["field"].StartIndexLocation;
	fieldRitem->BaseVertexLocation = fieldRitem->Geo->DrawArgs["field"].BaseVertexLocation;
	mGridItem = fieldRitem.get();

	mTmapRitems.push_back(fieldRitem.get());
	mAllRitems.push_back(std::move(fieldRitem));
}



void Flame::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();

	// For each render item...
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);

		UINT temp = objectCB->GetGPUVirtualAddress();

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize;

		cmdList->SetGraphicsRootDescriptorTable(0, tex);
		cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
		cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

void Flame::DrawSceneToParticleMap()
{
	mCommandList->RSSetViewports(1, &mParticleMap->Viewport());
	mCommandList->RSSetScissorRects(1, &mParticleMap->ScissorRect());

	// Change to RENDER_TARGET.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mParticleMap->Resource(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	// Clear the back buffer and depth buffer.
	FLOAT Black[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	FLOAT White[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	mCommandList->ClearRenderTargetView(mParticleMap->Rtv(), Black, 0, nullptr);
	mCommandList->ClearDepthStencilView(mDSV, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &mParticleMap->Rtv(), true, &mDSV);

	// Bind the pass constant buffer for this cube map face so we use 
	// the right view/proj matrix for this cube face.
	auto passCB = mCurrFrameResource->PassCB->Resource();
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCBAddress);

	
	mCommandList->SetPipelineState(mOpaquePSO.Get());
	DrawRenderItems(mCommandList.Get(), mBoxRitems);
	mCommandList->SetPipelineState(mTransPSO.Get());
	DrawRenderItems(mCommandList.Get(), mTransRitems);
	

	//DrawRenderItems(mCommandList.Get(), mTransRitems);

	

	// Change back to GENERIC_READ so we can read the texture in a shader.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mParticleMap->Resource(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void Flame::DrawSceneToTmap()
{
	mCommandList->RSSetViewports(1, &mTurbluenceMap->Viewport());
	mCommandList->RSSetScissorRects(1, &mTurbluenceMap->ScissorRect());

	// Change to RENDER_TARGET.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mTurbluenceMap->Resource(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	// Clear the back buffer and depth buffer.
	FLOAT Black[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	FLOAT White[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	mCommandList->ClearRenderTargetView(mTurbluenceMap->Rtv(), Black, 0, nullptr);
	mCommandList->ClearDepthStencilView(mDSV, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &mTurbluenceMap->Rtv(), true, &mDSV	);

	// Bind the pass constant buffer for this cube map face so we use 
	// the right view/proj matrix for this cube face.
	auto passCB = mCurrFrameResource->PassCB->Resource();
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCBAddress);

	

	mCommandList->SetPipelineState(mTmapPSO.Get());
	DrawRenderItems(mCommandList.Get(), mTmapRitems);
	


	//DrawRenderItems(mCommandList.Get(), mTransRitems);



	// Change back to GENERIC_READ so we can read the texture in a shader.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mTurbluenceMap->Resource(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> Flame::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}

float Flame::GetHillsHeight(float x, float z)const
{
	return 0.1f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 Flame::GetHillsNormal(float x, float z)const
{
	// n = (-df/dx, 1, -df/dz)
	XMFLOAT3 n(
		-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
		1.0f,
		-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}