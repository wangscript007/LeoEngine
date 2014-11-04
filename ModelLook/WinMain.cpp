#define _CRTDBG_MAP_ALLOC
#include	<stdlib.h>
#include	<crtdbg.h>

#include <IndePlatform\platform.h>
#include <IndePlatform\Singleton.hpp>
#include <IndePlatform\ThreadSync.hpp>
#include <IndePlatform\clock.hpp>

#include <Core\Mesh.hpp>
#include <Core\Effect.h>
#include <Core\Camera.hpp>
#include <Core\Geometry.hpp>
#include <Core\RenderSync.hpp>
#include <Core\EffectLine.hpp>
#include <Core\Terrain.hpp>
#include <Core\Sky.hpp>
#include <Core\SkeletonModel.hpp>
#include <Core\MeshLoad.hpp>
#include <Core\EffectSkeleton.hpp>
#include <COM.hpp>

#include <TextureMgr.h>
#include <ShaderMgr.h>
#include <RenderStates.hpp>
#include <exception.hpp>


#include "Axis.hpp"

#include "window.hpp"
#include "COM.hpp"
#include "DeviceMgr.h"

#include <Commdlg.h>

#include <thread>
#include <atomic>
#include <mutex>
#include "resource.h"


leo::Event event;
std::unique_ptr<leo::Mesh> pMesh = nullptr;
std::unique_ptr<leo::Camera> pCamera = nullptr;
std::unique_ptr<leo::Terrain<3,64,6>> pTerrain = nullptr;
std::unique_ptr<leo::Sky> pSky = nullptr;
std::unique_ptr<leo::SkeletonModel> pSke = nullptr;
std::atomic<bool> renderAble = false;
std::atomic<bool> renderThreadRun = true;

std::mutex mSizeMutex;




void DeviceEvent()
{
	while (!leo::DeviceMgr().GetDevice())
	{
		Sleep(0);
	}
	event.Set();
}


void Render();

void BuildRes();


std::wstring GetOpenL3dFile()
{
	wchar_t szPathName[MAX_PATH] = {};
	OPENFILENAME ofn;
	std::memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = GetForegroundWindow();

	ofn.lpstrTitle = L"选择模型文件";
	ofn.lpstrFilter = L"L3D Files\0*.l3d\0\0";

	ofn.lpstrFile = szPathName;
	ofn.nMaxFile = sizeof(szPathName);

	//以当前目录为初始选择目录
	wchar_t szCurDir[MAX_PATH];
	GetCurrentDirectory(sizeof(szCurDir), szCurDir);
	ofn.lpstrInitialDir = szCurDir;

	ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	if (GetOpenFileName(&ofn))
	{
		return std::wstring(szPathName);
	}
	return std::wstring();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	leo::DeviceMgr DeviceMgr;
	leo::OutputWindow win;

	auto clientSize = std::make_pair<leo::uint16,leo::uint16>(800, 600);
	if (!win.Create(GetModuleHandle(nullptr), clientSize, L"Model LooK", 
		WS_BORDER | WS_SIZEBOX,0,
		MAKEINTRESOURCE(IDI_ICON1)))
	{
		return 0;
	}

	HACCEL hAccel = LoadAccelerators(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDR_ACCELERATOR1));


	

	DeviceMgr.CreateDevice(false, clientSize);

	auto mNeedDuration = leo::clock::to_duration<>(1.f);
	auto mHasDuration = leo::clock::to_duration<>(0.f);
	auto mTimePoint = leo::clock::now();
	auto sizeproc = [&](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		leo::RenderSync::Block block;
		if (DeviceMgr.GetDevice())
		{
			if (mHasDuration > mNeedDuration)
			{
				auto size = std::make_pair<leo::uint16, leo::uint16>(LOWORD(lParam), HIWORD(lParam));
				DeviceMgr.ReSize(size);
				mHasDuration = leo::clock::to_duration<>(0.f);
			}
			else
			{
				mHasDuration += leo::clock::now()-mTimePoint;
				mTimePoint = leo::clock::now();
			}
		}
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	};

	win.BindMsgFunc(WM_SIZE, sizeproc);

	DeviceEvent();
	BuildRes();

	auto cmdmsgproc = [&](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)-> LRESULT
	{
		switch (LOWORD(wParam))
		{
		case ID_QUIT:
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case ID_OPEN:
		{
			leo::RenderSync::Block block;
			renderAble = false;
			pMesh.reset(new leo::Mesh());
			if (pMesh->Load(GetOpenL3dFile(), leo::DeviceMgr().GetDevice()))
				renderAble = (true);
		}
			break;
		case ID_ROLL_LEFT:
			pCamera->Roll(-1.f);
			break;
		case ID_ROLL_RIGHT:
			pCamera->Roll(+1.f);
			break;
		case ID_YAW_FRONT:
			pCamera->Yaw(-1.f);
			break;
		case ID_YAW_BACK:
			pCamera->Yaw(+1.f);
			break;
		case ID_PITCH_UP:
			pCamera->Pitch(-1.f);
			break;
		case ID_PITCH_DOWN:
			pCamera->Pitch(+1.f);
		case ID_NORMALLINE:
			if (!leo::EffectConfig::GetInstance()->NormalLine())
			{
				bool t = renderAble;
				renderAble = false;
				leo::EffectConfig::GetInstance()->NormalLine(true);
				renderAble = t;
			}
			else
			{
				leo::EffectConfig::GetInstance()->NormalLine(false);
			}
			break;
		default:
			break;
		}
		return 0;
	};
	win.BindMsgFunc(WM_COMMAND, cmdmsgproc);
	
#if 0
	leo::ViewPort vp;
	UINT numVp = 1;
	D3D11_VIEWPORT dvp;
	DeviceMgr.GetDeviceContext()->RSGetViewports(&numVp, &dvp);
	vp.mHeight = dvp.Height;
	vp.mMaxDepth = dvp.MaxDepth;
	vp.mMinDepth = dvp.MinDepth;
	vp.mTLX = dvp.TopLeftX;
	vp.mTLY = dvp.TopLeftY;
	vp.mWindth = dvp.Width;
	leo::float4x4 proj;
	leo::XMStoreFloat4x4A((leo::XMFLOAT4X4A*)&proj, pCamera->Proj());
	

	auto mouseproc = [&](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		auto x = (float)GET_X_LPARAM(lParam);
		auto y = (float)GET_Y_LPARAM(lParam);
		leo::float4x4 inv;
		leo::XMVECTOR temp;
		auto ray = leo::Ray::Pick(vp, proj, leo::float2(x, y));
		for (auto i = 0; i != 10;++i){
			leo::XMStoreFloat4x4A((leo::XMFLOAT4X4A*)&inv, leo::XMMatrixInverse(&temp, pCamera->View())*leo::XMMatrixInverse(&temp, mBoxSqts[i].operator DirectX::XMMATRIX()));

			if (ray.Transform(inv).Normalize().Intersect(mBox.GetBoundingBox()).first)
				mBoxPicked[i] = true;
			else
				mBoxPicked[i] = false;
		}
		return 0;
	};
	win.BindMsgFunc(WM_LBUTTONDOWN, mouseproc);
	mBox.Color(leo::float4(1.f, 0.f, 0.f, 1.f));

	
#endif

	std::thread renderThread(Render);

	while (true)
	{
		::MSG msg{ nullptr, 0, 0, 0, 0, { 0, 0 } };
		if (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) != 0)
		{
			if (msg.message == WM_QUIT)
				break;
			if (!TranslateAccelerator(win.GetHwnd(), hAccel, &msg))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
		}
		else
			::WaitMessage();
	}
	renderThreadRun = false;
	renderThread.join();

	pMesh.reset(nullptr);
	pTerrain.reset(nullptr);
	pSky.reset(nullptr);
	pSke.reset(nullptr);
	leo::global::Destroy();
#ifdef DEBUG
	leo::SingletonManger::GetInstance()->PrintAllSingletonInfo();
#endif
	leo::SingletonManger::GetInstance()->UnInstallAllSingleton();
	return 0;
}

void BuildRes()
{
	pCamera = std::make_unique<leo::Camera>();

	using leo::float3;

	auto Eye = float3(0.f,0.f,-5.f);
	auto At = float3(0.f,0.f,0.f);
	auto Up = float3(0.f,1.f, 0.f);

	pCamera->LookAt(Eye, At, Up);

	event.Wait();

	pCamera->SetFrustum(leo::def::frustum_fov, leo::DeviceMgr().GetAspect(), leo::def::frustum_near, leo::def::frustum_far);
	pCamera->SetFrustum(leo::PROJECTION_TYPE::PERSPECTIVE);

	auto proj = pCamera->Proj();
	auto testproj = leo::XMMatrixPerspectiveFovLH(leo::def::frustum_fov, leo::DeviceMgr().GetAspect(), leo::def::frustum_near, leo::def::frustum_far);

	auto& pEffect =  leo::EffectNormalMap::GetInstance(leo::DeviceMgr().GetDevice());
	leo::EffectTerrain::GetInstance(leo::DeviceMgr().GetDevice());
	

	leo::DirectionLight dirlight;
	dirlight.ambient = leo::float4(1.f, 1.f, 1.f, 1.f);
	dirlight.diffuse = leo::float4(1.f, 1.f, 1.f, 1.f);
	dirlight.specular = leo::float4(1.f, 1.f, 1.f, 32.f);
	dirlight.dir = leo::float4(0.0f, 1.0f, 5.0f, 0.f);

	pEffect->Light(dirlight);

	leo::EffectNormalLine::GetInstance(leo::DeviceMgr().GetDevice());
	leo::EffectSky::GetInstance(leo::DeviceMgr().GetDevice());
	leo::EffectSkeleton::GetInstance(leo::DeviceMgr().GetDevice())->Light(dirlight);
	//leo::EffectLine::GetInstance(leo::DeviceMgr().GetDevice());
	//leo::Axis::GetInstance(leo::DeviceMgr().GetDevice());


	struct TerrainFileHeader
	{
		float mChunkSize;
		std::uint32_t mHorChunkNum;
		std::uint32_t mVerChunkNum;
		wchar_t mHeightMap[leo::win::file::max_path];
	}mTerrainFileHeader;

	mTerrainFileHeader.mChunkSize = 8;
	mTerrainFileHeader.mHorChunkNum = 32;
	mTerrainFileHeader.mVerChunkNum = 32;
	wcscpy(mTerrainFileHeader.mHeightMap, L"Resource\\GaussianNoise256.jpg");

	{
		auto & pFile = leo::win::File::Open(L"Resource\\Test.Terrain", leo::win::File::TO_WRITE);
		pFile->Write(0, &mTerrainFileHeader, sizeof(mTerrainFileHeader));
	}
	pTerrain = std::make_unique < leo::Terrain<3,64,6> >(leo::DeviceMgr().GetDevice(), L"Resource\\Test.Terrain");

	pSky = std::make_unique<leo::Sky>(leo::DeviceMgr().GetDevice(), L"Resource\\sunsetcube1024.dds");

	pSke = std::make_unique<leo::SkeletonModel>();
	pSke->LoadFromFile(L"Resource\\soldier.l3d", leo::DeviceMgr().GetDevice());
	pSke->Scale(0.05f);
	//leo::DeviceMgr().GetDeviceContext()->RSSetState(leo::RenderStates().GetRasterizerState(L"WireframeRS"));
}


void Render()
{
	event.Wait();
	while (renderThreadRun)
	{
		leo::DeviceMgr dm;

		if (GetAsyncKeyState('W') & 0X8000)
			pCamera->Walk(+0.05f);

		if (GetAsyncKeyState('S') & 0X8000)
			pCamera->Walk(-0.05f);

		if (GetAsyncKeyState('A') & 0X8000)
			pCamera->Strafe(-0.05f);

		if (GetAsyncKeyState('D') & 0X8000)
			pCamera->Strafe(+0.05f);

		pCamera->UpdateViewMatrix();
		leo::clock::GameClock::Update(1/60.f);
		leo::RenderSync::GetInstance()->Sync();

		auto devicecontext = dm.GetDeviceContext();
		float ClearColor[4] = { 0.0f, 0.25f, 0.25f, 0.8f };
		devicecontext->ClearRenderTargetView(dm.GetRenderTargetView(), ClearColor);
		devicecontext->ClearDepthStencilView(dm.GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);

		if (renderAble)
			pMesh->Render(devicecontext, *pCamera);
		
		//leo::Axis::GetInstance()->Render(devicecontext, *pCamera);

		

		pSky->Render(devicecontext, *pCamera);
		pSke->Update();
		pSke->Render(devicecontext,*pCamera);
		//pTerrain->Render(devicecontext, *pCamera);

		leo::DeviceMgr().GetSwapChain()->Present(0, 0);

		leo::RenderSync::GetInstance()->Present();

		std::this_thread::sleep_for(std::chrono::milliseconds(0));
	}
}
