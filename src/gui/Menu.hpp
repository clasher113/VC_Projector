#pragma once

#include "Widget.hpp"

#include "../Enum.hpp"

#include <functional>

class GifPlayer;
namespace vcp {
    class Window;
}
namespace gui {
    class Menu;
    class Button;
    class Container;
    class PlayerController;
}

namespace sf {
    class Image;
    class Font;
    class Sprite;
    class Texture;
    class RenderTexture;
    class Color;
}

class gui::Menu : public gui::Widget {
public:
    Menu(vcp::Window& window, const sf::Font& font, const sf::Vector2u& canvasSize, float width);
    ~Menu();

    void onUpdate(const float deltaTime, bool& refreshFlag);
    void onEvent(const sf::Event& event, bool& refreshFlag, const sf::Vector2f& offset) override;
    void onSizeChange(const sf::Vector2u& newSize);

    void setOnModeChangeCallback(const std::function<void()>& callback);

    const sf::Drawable& getContent() const;
    void getContentPixels(sf::Color* const dest) const;
    Mode getMode() const;
    sf::Vector2f getSize() const override;

    bool isCursorOverElement(const sf::Vector2f& cursorPos);

private:
    bool m_stretchToScreen = true, m_allowAlpha = false, m_justOpened = false;
    Mode m_mode = Mode::SCREEN;

    GifPlayer* m_p_gifPlayer;
    PlayerController* m_p_playerController;

    gui::Container* m_p_mainContainer = nullptr;
    gui::Container* m_p_imageContainer = nullptr;

    sf::Texture* m_p_texture;
    sf::Sprite* m_p_sprite;

    sf::RenderTexture* m_p_canvas;
    sf::Sprite* m_p_canvasSprite;

    std::function<void()> m_onModeChangeCallback;

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    void clearMainContainer();
    void updateContent();
};