#pragma once

#include "Widget.hpp"

#include <functional>

namespace gui {
    class Slider;
}

namespace sf {
    class RectangleShape;
}

class gui::Slider : public gui::Widget {
public:
    Slider();
    virtual ~Slider() override;

    void onEvent(const sf::Event& event, bool& refreshFlag, const sf::Vector2f& offset) override;

    void setSize(const sf::Vector2f& size);
    void setRange(int min, int max);
    void setValue(int value);
    void setOnValueChangeCallback(const std::function<void(int)>& callback);

    sf::Vector2f getSize() const override;

private:
    bool m_grabbed = false, m_hover = false;
    int m_min = 0, m_max = 0, m_currentValue = m_min;
    sf::RectangleShape* m_p_background, * m_p_slider;

    std::function<void(int)> m_callback;

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    void updateSliderSize();
    int getValueFromPos(int x);
    float getPosFromValue();
};