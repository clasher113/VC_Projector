#include "Container.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <algorithm>

gui::Container::Container(bool autoRefresh) :
m_autoRefresh(autoRefresh)
{
}

gui::Container::~Container() {
    for (Widget* widget : m_widgets) {
        delete widget;
    }
}

void gui::Container::onEvent(const sf::Event& event, bool& refreshFlag, const sf::Vector2f& offset) {
    std::vector<Widget*> widgetsCopy(m_widgets);
    for (Widget* widget : widgetsCopy) {
        widget->onEvent(event, refreshFlag, offset + getPosition());
    }
}

void gui::Container::addElement(Widget* widget, size_t position) {
    m_widgets.emplace(m_widgets.begin() + std::min(m_widgets.size(), position), widget);
    refresh();
}

void gui::Container::removeElement(Widget* widget) {
    m_widgets.erase(std::remove(m_widgets.begin(), m_widgets.end(), widget), m_widgets.end());
    refresh();
}

void gui::Container::setInteval(float interval) {
    m_interval = interval;
    refresh();
}

sf::Vector2f gui::Container::getSize() const {
    sf::Vector2f size;
    for (const Widget* widget : m_widgets) {
        size.x = std::max(size.x, widget->getPosition().x + widget->getSize().x + (m_autoRefresh ? m_interval : 0.f));
        size.y = std::max(size.y, widget->getPosition().y + widget->getSize().y + (m_autoRefresh ? m_interval : 0.f));
    }
    return size;
}

void gui::Container::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    for (const Widget* widget : m_widgets) {
        target.draw(*widget, states);
    }
}

void gui::Container::refresh() {
    if (!m_autoRefresh) return;
    float posY = m_interval;
    for (Widget* widget : m_widgets) {
        widget->setPosition(sf::Vector2f(m_interval, posY));
        posY += widget->getSize().y + m_interval;
    }
}