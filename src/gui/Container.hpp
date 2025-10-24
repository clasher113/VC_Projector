#pragma once

#include "Widget.hpp"

#include <vector>

namespace gui {
    class Container;
    class Widget;
}

class gui::Container : public gui::Widget {
public:
    Container(bool autoRefresh = false);
    virtual ~Container() override;

    void onEvent(const sf::Event& event, bool& refreshFlag, const sf::Vector2f& offset) override;

    void addElement(Widget* widget, size_t position = -1);
    void removeElement(Widget* widget);
    void setInteval(float interval);

    sf::Vector2f getSize() const override;

protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    bool m_autoRefresh;
    float m_interval = 0.f;
    std::vector<Widget*> m_widgets;

    void refresh();
};