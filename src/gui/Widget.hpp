#pragma once

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Transformable.hpp>

namespace gui {
    class Widget;
}

namespace sf {
    class Event;
}

class gui::Widget : public sf::Drawable, public sf::Transformable {
public:
    Widget() {};
    virtual ~Widget() override {};

    virtual void onEvent(const sf::Event& event, bool& refreshFlag, const sf::Vector2f& offset) = 0;

    virtual sf::Vector2f getSize() const = 0;
};