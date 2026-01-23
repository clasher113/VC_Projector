#include "Slider.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <algorithm>

static const sf::Color IDLE_COLOR(100, 100, 100);
static const sf::Color HOVER_COLOR(147, 147, 147);
static const sf::Color CLICKED_COLOR(60, 140, 200);

gui::Slider::Slider() :
    m_lastState(Slider::State::IDLE),
    m_currentState(Slider::State::IDLE),
    m_p_background(new sf::RectangleShape(sf::Vector2f(100.f, 30.f))),
    m_p_slider(new sf::RectangleShape(sf::Vector2f(20.f, 30.f)))
{
    m_p_background->setFillColor(IDLE_COLOR);
    m_p_background->setOutlineColor(sf::Color(35, 35, 35));
    m_p_background->setOutlineThickness(2.f);
    m_p_background->setOrigin(sf::Vector2f(-2.f, -2.f));
    m_p_slider->setFillColor(sf::Color::White);
    m_p_slider->setOrigin(m_p_background->getOrigin());
}

gui::Slider::~Slider() {
    delete m_p_background;
    delete m_p_slider;
}

void gui::Slider::onEvent(const sf::Event& event, bool& refreshFlag, const sf::Vector2f& offset) {
    const auto act = [this, &refreshFlag, &offset](int mousePosX) {
        const int newValue = getValueFromPos(mousePosX - offset.x);
        if (m_currentValue != newValue) {
            m_currentValue = newValue;
            m_p_slider->setPosition(sf::Vector2f(getPosFromValue(), 0.f));
            if (m_callback) m_callback(m_currentValue);
            refreshFlag = true;
        }
    };

    const auto isHover = [this, &offset](sf::Vector2i mousePos) {
        return getTransform().transformRect(m_p_background->getGlobalBounds()).contains(
                sf::Vector2f(mousePos.x, mousePos.y) - offset);
    };

    if (const auto pressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (pressed->button == sf::Mouse::Button::Left && m_currentState == Slider::State::HOVER) {
            m_currentState = Slider::State::GRABBED;
            if (m_onGrabbedCallback) m_onGrabbedCallback();
            act(pressed->position.x);
        }
    }
    else if (const auto released = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (released->button == sf::Mouse::Button::Left) {
            m_currentState = isHover(released->position) ? Slider::State::HOVER : Slider::State::IDLE;
            if (m_lastState == Slider::State::GRABBED) {
                if (m_onReleasedCallback) m_onReleasedCallback();
            }
        }
    }
    else if (const auto moved = event.getIf<sf::Event::MouseMoved>()) {
        if (m_currentState != Slider::State::GRABBED) {
            m_currentState = isHover(moved->position) ? Slider::State::HOVER : Slider::State::IDLE;
        }
        else if (m_currentState == Slider::State::GRABBED) {
            act(moved->position.x);
        }
    }
    if (m_lastState != m_currentState) {
        refreshFlag = true;
        m_lastState = m_currentState;
        switch (m_currentState) {
            case Slider::State::IDLE:
                m_p_background->setFillColor(IDLE_COLOR);
                break;
            case Slider::State::HOVER:
                m_p_background->setFillColor(HOVER_COLOR);
                break;
            case Slider::State::GRABBED:
                m_p_background->setFillColor(CLICKED_COLOR);
                break;
        }
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
    if (m_currentValue == value) return;
    m_currentValue = value;
    m_currentValue = std::clamp(m_currentValue, m_min, m_max);
    m_p_slider->setPosition(sf::Vector2f(getPosFromValue(), 0.f));
    if (m_callback) m_callback(m_currentValue);
}

void gui::Slider::setOnValueChangeCallback(const std::function<void(int)>& callback) {
    m_callback = callback;
}

void gui::Slider::setOnGrabbedCallback(const std::function<void()>& callback) {
    m_onGrabbedCallback = callback;
}

void gui::Slider::setOnReleasedCallback(const std::function<void()>& callback) {
    m_onReleasedCallback = callback;
}

sf::Vector2f gui::Slider::getSize() const {
    const float outlineThickness = m_p_background->getOutlineThickness() * 2.f;
    return m_p_background->getSize() + sf::Vector2f(outlineThickness, outlineThickness);
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
    const sf::FloatRect bounds = m_p_background->getGlobalBounds();
    const float minPos = bounds.position.x + m_p_background->getOutlineThickness();
    const float maxPos = bounds.position.x + bounds.size.x - m_p_background->getOutlineThickness() - m_p_slider->getSize().x;
    const float percentage = (std::clamp(static_cast<float>(x), minPos, maxPos) - minPos) * 100.f / (maxPos - minPos);
    return percentage / (100.f / (m_max - m_min));
}

float gui::Slider::getPosFromValue() {
    const float maxPos = m_p_background->getSize().x - m_p_slider->getSize().x;
    return maxPos * (m_currentValue - m_min) / (m_max - m_min);
}
