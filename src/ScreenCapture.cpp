#include "ScreenCapture.hpp"

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "DXGI.lib")
extern "C" {
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 0;
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000000;
}
using Microsoft::WRL::ComPtr;
#elif __linux__
#include <sys/shm.h>
#include <X11/Xatom.h>
#endif // _WIN32

#include <cstring>
#include <algorithm>

ScreenCapture::ScreenCapture() :
	m_captureSize(0, 0)
{
}

ScreenCapture::~ScreenCapture() {
#ifdef _WIN32
	if (m_mode == ScreenCapture::Mode::DXGI) {
		m_p_deskDupl->Release();
		m_p_context->Release();
		m_p_device->Release();
	}
	else if (m_mode == ScreenCapture::Mode::GDI) {
		SelectObject(m_hCaptureDC, m_hOldBitmap);
		DeleteObject(m_hCaptureBitmap);
		DeleteDC(m_hCaptureDC);
		ReleaseDC(NULL, m_desktopHdc);
	}
#elif __linux__
	if (m_mode == ScreenCapture::Mode::X11) {
		if(m_p_ShmInfo) {
            shmdt(m_p_ShmInfo->shmaddr);
            shmctl(m_p_ShmInfo->shmid, IPC_RMID, 0);
            XShmDetach(m_p_display, m_p_ShmInfo);
			delete m_p_ShmInfo;
        }
        if(m_p_xImage) {
            XDestroyImage(m_p_xImage);
        }
        if(m_p_display) {
            XCloseDisplay(m_p_display);
        }
	}
#endif // _WIN32
}

bool ScreenCapture::initialize(const sf::Vector2u& captureSize) {
#ifdef _WIN32
	if (m_mode == ScreenCapture::Mode::NONE) {
		ComPtr<IDXGIAdapter1> adapter = nullptr;
		ComPtr<IDXGIOutput> dxgiOutput = nullptr;

		ComPtr<IDXGIFactory1> factory;
		if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
			for (UINT i = 0; ; i++) {
				ComPtr<IDXGIAdapter1> adapterTemp;
				if (FAILED(factory->EnumAdapters1(i, &adapterTemp))) {
					break;
				}

				DXGI_ADAPTER_DESC1 desc;
				if (FAILED(adapterTemp->GetDesc1(&desc)) || desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
					continue;
				}

				ComPtr<IDXGIOutput> dxgiOutputTemp;
				if (SUCCEEDED(adapterTemp->EnumOutputs(0, &dxgiOutputTemp))) {
					adapter = adapterTemp;
					dxgiOutput = dxgiOutputTemp;
					break;
				}
				else continue;
			}
		}

		static constexpr D3D_FEATURE_LEVEL featureLevels[] = {
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_9_1,
		};

		D3D_FEATURE_LEVEL featureLevel;

		if (SUCCEEDED(D3D11CreateDevice(
			adapter.Get(),
			adapter.Get() == nullptr ? D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_UNKNOWN,
			nullptr, 0,
			featureLevels, static_cast<UINT>(std::size(featureLevels)),
			D3D11_SDK_VERSION,
			&m_p_device,
			&featureLevel,
			&m_p_context
			))) {
			if (adapter == nullptr) {
				ComPtr<IDXGIDevice> dxgiDevice;
				if (SUCCEEDED(m_p_device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)))) {
					if (SUCCEEDED(dxgiDevice->GetParent(IID_PPV_ARGS(&adapter)))) {
						if (SUCCEEDED(adapter->EnumOutputs(0, &dxgiOutput))) {

						}
					}
				}
			}

			ComPtr<IDXGIOutput1> dxgiOutput1;
			if (SUCCEEDED(dxgiOutput->QueryInterface(IID_PPV_ARGS(&dxgiOutput1)))) {
				if (SUCCEEDED(dxgiOutput1->DuplicateOutput(m_p_device, &m_p_deskDupl))) {
					m_mode = ScreenCapture::Mode::DXGI;
				}
			}
		}
	}
	if (m_mode == ScreenCapture::Mode::NONE) {
		HWND desktop = GetDesktopWindow();
		m_desktopHdc = GetDC(desktop);
		m_hCaptureDC = CreateCompatibleDC(m_desktopHdc);
		m_hCaptureBitmap = CreateCompatibleBitmap(m_desktopHdc, captureSize.x, captureSize.y);
		m_hOldBitmap = SelectObject(m_hCaptureDC, m_hCaptureBitmap);

		m_bmi.bmiHeader.biBitCount = 32;
		m_bmi.bmiHeader.biCompression = BI_RGB;
		m_bmi.bmiHeader.biPlanes = 1;
		m_bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
		m_bmi.bmiHeader.biHeight = captureSize.y;
		m_bmi.bmiHeader.biWidth = captureSize.x;

		m_mode = ScreenCapture::Mode::GDI;
	}
#elif __linux__
	m_p_display = XOpenDisplay(NULL);
	m_p_ShmInfo = new XShmSegmentInfo;
	int scr = XDefaultScreen(m_p_display);

	m_p_xImage = XShmCreateImage(m_p_display, DefaultVisual(m_p_display, scr), DefaultDepth(m_p_display, scr), ZPixmap,
		NULL, m_p_ShmInfo, captureSize.x, captureSize.y);

	m_p_ShmInfo->shmid = shmget(IPC_PRIVATE, m_p_xImage->bytes_per_line * m_p_xImage->height, IPC_CREAT | 0777);
	m_p_ShmInfo->readOnly = False;
	m_p_ShmInfo->shmaddr = m_p_xImage->data = (char*)shmat(m_p_ShmInfo->shmid, 0, 0);

	XShmAttach(m_p_display, m_p_ShmInfo);

	m_mode = ScreenCapture::Mode::X11;
#endif // _WIN32
	m_captureSize = captureSize;
	return true;
}

bool ScreenCapture::setSize(const sf::Vector2u& captureSize) {
	if (captureSize == m_captureSize) return true;
#ifdef _WIN32
	if (m_mode == ScreenCapture::Mode::DXGI) {

	}
	else if (m_mode == ScreenCapture::Mode::GDI) {
		m_bmi.bmiHeader.biWidth = captureSize.x;
		m_bmi.bmiHeader.biHeight = captureSize.y;

		m_hCaptureBitmap = CreateCompatibleBitmap(m_desktopHdc, captureSize.x, captureSize.y);
		SelectObject(m_hCaptureDC, m_hCaptureBitmap);
	}
#elif __linux__
	if (m_mode == ScreenCapture::Mode::X11) {
		if (m_p_xImage) XDestroyImage(m_p_xImage);

		int scr = XDefaultScreen(m_p_display);

		m_p_xImage = XShmCreateImage(m_p_display, DefaultVisual(m_p_display, scr), DefaultDepth(m_p_display, scr), ZPixmap,
			NULL, m_p_ShmInfo, captureSize.x, captureSize.y);

		m_p_ShmInfo->shmid = shmget(IPC_PRIVATE, m_p_xImage->bytes_per_line * m_p_xImage->height, IPC_CREAT | 0777);
		m_p_ShmInfo->readOnly = False;
		m_p_ShmInfo->shmaddr = m_p_xImage->data = (char*)shmat(m_p_ShmInfo->shmid, 0, 0);

		XShmAttach(m_p_display, m_p_ShmInfo);
	}
#endif // _WIN32
	m_captureSize = captureSize;
	return true;
}

bool ScreenCapture::capture(const sf::Vector2i& capturePos, void* const dst) {
	uint8_t* const dest = static_cast<uint8_t*>(dst);
#ifdef _WIN32
	if (m_mode == ScreenCapture::Mode::DXGI) {
		ComPtr<IDXGIResource> desktopResource;
		DXGI_OUTDUPL_FRAME_INFO frameInfo;

		if (FAILED(m_p_deskDupl->AcquireNextFrame(100, &frameInfo, desktopResource.GetAddressOf()))) {
			return false;
		}
		ComPtr<ID3D11Texture2D> gpuTex = nullptr;
		if (FAILED(desktopResource->QueryInterface(IID_PPV_ARGS(gpuTex.GetAddressOf())))) {
			return false;
		}

		D3D11_TEXTURE2D_DESC textureDesc;
		gpuTex->GetDesc(&textureDesc);
		textureDesc.Width = m_captureSize.x;
		textureDesc.Height = m_captureSize.y;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		textureDesc.Usage = D3D11_USAGE_STAGING;
		textureDesc.BindFlags = 0;
		textureDesc.MiscFlags = 0;

		ComPtr<ID3D11Texture2D> cpuTex = nullptr;
		if (FAILED(m_p_device->CreateTexture2D(&textureDesc, nullptr, cpuTex.GetAddressOf()))) {
			return false;
		}

		D3D11_BOX box = {
			std::max(0, capturePos.x),
			std::max(0, capturePos.y),
			0,
			std::max(1u, m_captureSize.x + capturePos.x),
			std::max(1u, m_captureSize.y + capturePos.y),
			1
		};
		m_p_context->CopySubresourceRegion(cpuTex.Get(), 0, 0, 0, 0, gpuTex.Get(), 0, &box);

		D3D11_MAPPED_SUBRESOURCE resourceDesc;
		if (FAILED(m_p_context->Map(cpuTex.Get(), 0, D3D11_MAP_READ, 0, &resourceDesc))) {
			return false;
		}

		const unsigned char* source = static_cast<const unsigned char*>(resourceDesc.pData);
		unsigned int location = (textureDesc.Height - 1) * resourceDesc.RowPitch;
		for (int i = 0; i < textureDesc.Height; ++i) {
			memcpy(&dest[i * (textureDesc.Width * 4)], &source[location], textureDesc.Width * 4);
			location -= resourceDesc.RowPitch;
		}

		m_p_context->Unmap(cpuTex.Get(), 0);

		m_p_deskDupl->ReleaseFrame();
	}
	else if (m_mode == ScreenCapture::Mode::GDI) {
		if (!BitBlt(m_hCaptureDC, 0, 0, m_captureSize.x, m_captureSize.y, m_desktopHdc, capturePos.x, capturePos.y, SRCCOPY)){
			return false;
		}
		GetDIBits(m_hCaptureDC, m_hCaptureBitmap, 0, m_captureSize.y, dst, &m_bmi, DIB_RGB_COLORS);
	}
#elif __linux__
	if (m_mode == ScreenCapture::Mode::X11) {
		// todo fix on wayland 
		// https://github.com/KDE/xwaylandvideobridge
		XShmGetImage(m_p_display, DefaultRootWindow(m_p_display), m_p_xImage, capturePos.x, capturePos.y, AllPlanes);
		unsigned int location = (m_captureSize.y - 1) * (m_captureSize.x * 4);
		for (int i = 0; i < m_captureSize.y; ++i) {
			memcpy(&dest[i * (m_captureSize.x * 4)], &m_p_xImage->data[location], m_captureSize.x * 4);
			location -= m_captureSize.x * 4;
		}

		uint32_t* p = static_cast<uint32_t*>(dst);
		for (int y = 0; y < m_p_xImage->height; ++y) {
			for (int x = 0; x < m_p_xImage->width; ++x) {
				*p++ |= 0xff000000;
			}
		}
	}
#endif // _WIN32

	return true;
}

ScreenCapture::Mode ScreenCapture::getMode() const {
	return m_mode;
}