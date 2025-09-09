#include "Container.hpp"

#include <SFML/Graphics/RenderTarget.hpp>

gui::Container::Container() {
}

gui::Container::~Container() {
    for (Widget* widget : m_widgets) {
        delete widget;
    }
}

void gui::Container::onEvent(const sf::Event& event, bool& refreshFlag, const sf::Vector2f& offset) {
    for (Widget* widget : m_widgets) {
        widget->onEvent(event, refreshFlag, offset + getPosition());
    }
}

void gui::Container::addElement(Widget* widget) {
    m_widgets.emplace_back(widget);
}

void gui::Container::removeElement(Widget* widget) {
    m_widgets.erase(std::remove(m_widgets.begin(), m_widgets.end(), widget), m_widgets.end());
}

sf::Vector2f gui::Container::getSize() const {
    sf::Vector2f size;
    for (const Widget* widget : m_widgets) {
        size.x = std::max(size.x, widget->getPosition().x + widget->getSize().x);
        size.y = std::max(size.y, widget->getPosition().y + widget->getSize().y);
    }
    return size;
}

void gui::Container::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    for (const Widget* widget : m_widgets) {
        target.draw(*widget, states);
    }
}
