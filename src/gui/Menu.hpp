#pragma once

#include "Widget.hpp"

#include "../Enum.hpp"

#include <functional>
#include <vector>

namespace gui {
    class Menu;
    class Button;
    class Container;
}

namespace sf {
    class Image;
    class Font;
    class Sprite;
    class Texture;
    class RenderTexture;
    class Color;
}

struct Image;

class gui::Menu : public gui::Widget {
public:
    Menu(const sf::Font& font, const sf::Vector2u& canvasSize, float width);
    ~Menu();

    void onUpdate(const float deltaTime, bool& refreshFlag);
    void onEvent(const sf::Event& event, bool& refreshFlag, const sf::Vector2f& offset) override;
    void onSizeChange(const sf::Vector2u& newSize);

    void setOnModeChangeCallback(const std::function<void()>& callback);

    const sf::Drawable& getContent() const;
    const sf::Color* getContentPixels() const;
    Mode getMode() const;
    sf::Vector2f getSize() const override;

    sf::Image* m_p_currentImage = nullptr;

private:
    bool m_keepAspectratio = false, m_allowAlpha = false;
    Mode m_mode = Mode::SCREEN;
    float m_animationTimer = 0.f;
    size_t m_currentFrame = 0;

    gui::Container* m_p_mainContainer = nullptr;
    gui::Container* m_p_imageContainer = nullptr;

    sf::Sprite* m_p_sprite;

    sf::RenderTexture* m_p_canvas;
    sf::Sprite* m_p_canvasSprite;

    sf::Color* m_p_contentPixels = nullptr;

    std::function<void()> m_onModeChangeCallback;
    std::vector<Image*> m_images;

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    void clearMainContainer();
    void clearImages();
    void updateContent();
};