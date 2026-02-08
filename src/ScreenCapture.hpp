#pragma once

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <dwmapi.h>
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGIOutputDuplication;
#elif __linux__
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/shape.h>
#undef Status
#undef None
#endif // _WIN32

#include <cstdint>
#include <SFML/System/Vector2.hpp>

class ScreenCapture {
public:
#ifdef _WIN32
    enum class Mode : uint8_t {
        NONE = 0,
        DXGI = 1,
        GDI = 2
    };
#elif __linux__
    enum class Mode : uint8_t {
        NONE = 0,
        X11 = 1
    };
#endif // _WIN32
    ScreenCapture();
    ~ScreenCapture();

    bool initialize(const sf::Vector2u& captureSize);
    bool setSize(const sf::Vector2u& captureSize);

    bool capture(const sf::Vector2i& capturePos, void* const dst);
    ScreenCapture::Mode getMode() const;

private:
#ifdef _WIN32
    //DXGI
    ID3D11Device* m_p_device = nullptr;
    ID3D11DeviceContext* m_p_context = nullptr;
    IDXGIOutputDuplication* m_p_deskDupl = nullptr;
    // GDI
    BITMAPINFO m_bmi{};
    HGDIOBJ m_hOldBitmap;
    HBITMAP m_hCaptureBitmap;
    HDC m_desktopHdc;
    HDC m_hCaptureDC;
#elif __linux__
    // X11
    Display* m_p_display = nullptr;
    XImage* m_p_xImage = nullptr;
    XShmSegmentInfo* m_p_ShmInfo = nullptr;
#endif // _WIN32
    Mode m_mode = Mode::NONE;
    sf::Vector2u m_captureSize;
};