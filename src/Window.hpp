#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "Enum.hpp"
#include "ScreenCapture.hpp"

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

    bool setSize(const sf::Vector2u& size);
    void setPosition(const sf::Vector2i& position);
    void setStatus(Status status);
    void setAlwaysOnTop(bool flag);
    void setIconified(bool flag);

    void onUpdate(const float deltaTime);
    void pollEvents();

    const sf::Color* const capture();
    Status getStatus() const;

    void draw();

private:
    bool m_updateRequire = true, m_grabbed = false;

    Status m_currentStatus = Status::NONE;
    sf::Vector2u m_captureSize;
    sf::Vector2i m_grabbedOffset;

    sf::RenderTexture m_statusContainer;
    sf::RectangleShape m_border;
    sf::Font m_font;
    sf::Text m_statusText;
    sf::Sprite m_statusContainerSprite;

    sf::Color* m_p_pixels = nullptr;

    gui::Menu* m_p_menu = nullptr;
    ScreenCapture m_screenCapture;

#ifdef __linux__
    Display* m_p_display = nullptr;
    ::Window m_window, m_rootWindow;
#endif // __linux__

    bool updateWindowSize(const sf::Vector2u& size);
};