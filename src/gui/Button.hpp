#pragma once

#include "Widget.hpp"

#include <functional>
#include <string>

namespace gui {
    class Container;
    class Button;
}

namespace sf {
    class RectangleShape;
    class Font;
    class Text;
    class Texture;
    class Sprite;
}

class gui::Button : public gui::Widget {
public:
    enum class State {
        IDLE = 0,
        HOVER,
        PRESSED
    };

    Button(const sf::Font& font);
    virtual ~Button() override;

    void onEvent(const sf::Event& event, bool& refreshFlag, const sf::Vector2f& offset) override;

    void setText(const std::string& string);
    void setIcon(const sf::Texture& texture);
    void setCallback(const std::function<void()>& callback);
    void setSize(const sf::Vector2f& size);

    sf::Vector2f getSize() const override;

private:
    enum class Style {
        STRING = 0,
        ICON
    };
    State m_lastState, m_currentState;
    Style m_style = Style::STRING;
    sf::RectangleShape* m_p_shape;
    sf::Text* m_p_text;
    sf::Sprite* m_p_iconSprite;

    std::function<void()> m_callback;

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    void centerText();
    void centerIcon();
};