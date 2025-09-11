#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Sprite.hpp>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <dwmapi.h>
#elif __linux__
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#undef Status
#undef None
#endif // _WIN32

#include "Enum.hpp"

namespace vcp {
    class Window;
}
namespace gui {
    class Menu;
}

class vcp::Window : public sf::RenderWindow {
public:
    Window(const sf::Vector2u& captureSize);
    ~Window();

    void setSize(const sf::Vector2u& size);
    void setPosition(const sf::Vector2i& position);
    void setStatus(Status status);

    void onUpdate(const float deltaTime);
    void onEvent(const sf::Event& event);

    sf::Color* capture();

    void draw();

private:
    bool m_updateRequire = true;

    sf::Vector2u m_captureSize;

    sf::RenderTexture m_statusContainer;
    sf::RectangleShape m_border;
    sf::Font m_font;
    sf::Text m_statusText;
    sf::Sprite m_statusContainerSprite;

    sf::Color* m_p_pixels = nullptr;

    gui::Menu* m_p_menu = nullptr;

    void updateWindowSize();

#ifdef _WIN32
    BITMAPINFO m_bmi{};
    HGDIOBJ m_hOldBitmap;
    HBITMAP m_hCaptureBitmap;
    HDC m_desktopHdc;
    HDC m_hCaptureDC;
#elif __linux__
    Display* m_p_display = nullptr;
    ::Window m_window, m_rootWindow;
    XImage* m_p_xImage = nullptr;
    XShmSegmentInfo* m_p_ShmInfo = nullptr;
#endif // _WIN32
};