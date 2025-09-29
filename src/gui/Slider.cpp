#include "Slider.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>

gui::Slider::Slider() :
m_p_background(new sf::RectangleShape(sf::Vector2f(100.f, 30.f))),
m_p_slider(new sf::RectangleShape(sf::Vector2f(20.f, 30.f)))
{
    m_p_background->setFillColor(sf::Color(100, 100, 100));
    m_p_background->setOutlineColor(sf::Color(35, 35, 35));
    m_p_background->setOutlineThickness(2.f);
    m_p_slider->setFillColor(sf::Color(150, 150, 150));
}

gui::Slider::~Slider() {
    delete m_p_background;
    delete m_p_slider;
}

void gui::Slider::onEvent(const sf::Event& event, bool& refreshFlag, const sf::Vector2f& offset) {
    const int lastValue = m_currentValue;
    switch (event.type) {
        case sf::Event::MouseButtonPressed :
            if (event.mouseButton.button == sf::Mouse::Button::Left && m_hover) {
                m_grabbed = true;
            }
            break;
        case sf::Event::MouseButtonReleased :
            m_grabbed = false;
            break;
        case sf::Event::MouseMoved :
            m_hover = getTransform().transformRect(m_p_slider->getGlobalBounds()).contains(sf::Vector2f(event.mouseMove.x, event.mouseMove.y) - offset);
            if (m_grabbed) {
                m_currentValue = getValueFromPos(std::clamp(event.mouseMove.x - m_p_slider->getSize().x / 2.f,
                    0.f, m_p_background->getSize().x - m_p_slider->getSize().x / 2.f));              
            }
            break;
    }
    if (m_currentValue != lastValue) {
        m_p_slider->setPosition(getPosFromValue(), 0.f);
        if (m_callback) m_callback(m_currentValue);
        refreshFlag = true;
    }
}

void gui::Slider::setSize(const sf::Vector2f& size) {
    m_p_background->setSize(size);
    updateSliderSize();
}

void gui::Slider::setRange(int min, int max) {
    if (m_min > m_max) return;
    m_min = min;
    m_max = max;
    setValue(m_currentValue);
    updateSliderSize();
}

void gui::Slider::setValue(int value) {
    m_currentValue = value;
    m_currentValue = std::clamp(m_currentValue, m_min, m_max);
    m_p_slider->setPosition(getPosFromValue(), 0.f);
    if (m_callback) m_callback(m_currentValue);
}

void gui::Slider::setOnValueChangeCallback(const std::function<void(int)>& callback) {
    m_callback = callback;
}

sf::Vector2f gui::Slider::getSize() const {
    return m_p_background->getSize();
}

void gui::Slider::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    target.draw(*m_p_background, states);
    target.draw(*m_p_slider, states);
}

void gui::Slider::updateSliderSize() {
    const float width = std::max(10.f, m_p_background->getSize().x / static_cast<float>(std::max(1, m_max - m_min + 1)));
    m_p_slider->setSize(sf::Vector2f(width, m_p_background->getSize().y));
}

int gui::Slider::getValueFromPos(int x) {
    const float maxPos = m_p_background->getSize().x - m_p_slider->getSize().x;
    const float percentage = static_cast<float>(x) * 100.f / maxPos;
    return (m_max - m_min) * percentage / 100 + m_min;
}

float gui::Slider::getPosFromValue() {
    const float maxPos = m_p_background->getSize().x - m_p_slider->getSize().x;
    return maxPos * (m_currentValue - m_min) / (m_max - m_min);
}
