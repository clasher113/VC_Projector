#include "Button.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Window/Event.hpp>

using namespace gui;

const sf::Color IDLE_COLOR(100, 100, 100);
const sf::Color HOVER_COLOR(147, 147, 147);
const sf::Color CLICKED_COLOR(60, 140, 200);

Button::Button(const sf::Font& font) :
    m_lastState(State::IDLE),
    m_currentState(State::IDLE),
	m_p_shape(new sf::RectangleShape(sf::Vector2f(100.f, 30.f))),
	m_p_text(new sf::Text("The Button", font, 20U)),
	m_p_iconSprite(new sf::Sprite)
{
	m_p_shape->setFillColor(IDLE_COLOR);
	m_p_shape->setOutlineColor(sf::Color(35, 35, 35));
	m_p_shape->setOutlineThickness(2.f);

	centerText();
}

Button::~Button() {
	delete m_p_text;
	delete m_p_iconSprite;
	delete m_p_shape;
}

void Button::onEvent(const sf::Event& event, bool& refreshFlag, const sf::Vector2f& offset) {
	switch (event.type) {
		case sf::Event::MouseMoved :
			if (getTransform().transformRect(m_p_shape->getGlobalBounds()).contains(sf::Vector2f(event.mouseMove.x, event.mouseMove.y) - offset)) {
				m_currentState = State::HOVER;
			} else {
				m_currentState = State::IDLE;
			}
			break;
		case sf::Event::MouseButtonPressed :
			if (event.mouseButton.button == sf::Mouse::Button::Left && m_currentState == State::HOVER) {
				m_currentState = State::PRESSED;
			}
			break;
		case sf::Event::MouseButtonReleased :
			if (event.mouseButton.button == sf::Mouse::Button::Left && m_currentState == State::PRESSED) {
				m_currentState = State::HOVER;
			}
			break;
		case sf::Event::MouseLeft :
			m_currentState = State::IDLE;
			break;
	}
	if (m_currentState != m_lastState) {
		refreshFlag = true;
		m_lastState = m_currentState;
		switch (m_currentState) {
			case Button::State::IDLE:
				m_p_shape->setFillColor(IDLE_COLOR);
				break;
			case Button::State::HOVER:
				m_p_shape->setFillColor(HOVER_COLOR);
				break;
			case Button::State::PRESSED:
				m_p_shape->setFillColor(CLICKED_COLOR);
				if (m_callback) m_callback();
				break;
		}
	}
}

void Button::setText(const std::string& string) {
	m_style = Style::STRING;
	m_p_text->setString(string);
	centerText();
}

void gui::Button::setIcon(const sf::Texture& texture) {
	m_style = Style::ICON;
	m_p_iconSprite->setTexture(texture);
	centerIcon();
}

void Button::setCallback(const std::function<void()>& callback) {
	m_callback = callback;
}

void Button::setSize(const sf::Vector2f& size) {
	m_p_shape->setSize(size);
	switch (m_style) {
		case gui::Button::Style::STRING:
			centerText();
			break;
		case gui::Button::Style::ICON:
			centerIcon();
			break;
	}
}

sf::Vector2f gui::Button::getSize() const {
	return m_p_shape->getSize();
}

void Button::draw(sf::RenderTarget& target, sf::RenderStates states) const {
	states.transform *= getTransform();
	target.draw(*m_p_shape, states);
	switch (m_style) {
		case gui::Button::Style::STRING:
			target.draw(*m_p_text, states);
			break;
		case gui::Button::Style::ICON:
			target.draw(*m_p_iconSprite, states);
			break;
	}
}

void Button::centerText() {
	sf::FloatRect textRect(m_p_text->getLocalBounds());
	textRect.height = m_p_text->getFont()->getLineSpacing(m_p_text->getCharacterSize());
	m_p_text->setOrigin(sf::Vector2f(textRect.left + textRect.width / 2.0f, textRect.height / 2.0f) - m_p_shape->getSize() / 2.f);
}

void gui::Button::centerIcon() {
	const sf::FloatRect bounds = m_p_iconSprite->getLocalBounds();
	const sf::Vector2f size = getSize();

	sf::Vector2f scale(size.x / bounds.width, size.y / bounds.height);
	scale = sf::Vector2f(std::min(scale.x, scale.y), std::min(scale.x, scale.y));
	m_p_iconSprite->setScale(scale);
	m_p_iconSprite->setPosition((size.x - bounds.width * scale.x) / 2.f, (size.y - bounds.height * scale.y) / 2.f);
}