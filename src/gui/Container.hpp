#pragma once

#include "Widget.hpp"

#include <vector>

namespace gui {
    class Container;
    class Widget;
}

class gui::Container : public gui::Widget {
public:
    Container();
    virtual ~Container() override;

    void onEvent(const sf::Event& event, bool& refreshFlag, const sf::Vector2f& offset) override;

    void addElement(Widget* widget);
    void removeElement(Widget* widget);

    sf::Vector2f getSize() const override;

private:
    std::vector<Widget*> m_widgets;

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
};