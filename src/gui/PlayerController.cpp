#include "PlayerController.hpp"

#include "Button.hpp"
#include "Slider.hpp"
#include "../GifPlayer.hpp"

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Image.hpp>

gui::PlayerController::PlayerController(const sf::Font& font, GifPlayer* player, float width) :
m_p_player(player),
m_p_pauseTexture(new sf::Texture),
m_p_playTexture(new sf::Texture),
m_p_stopTexture(new sf::Texture),
m_p_slider(new gui::Slider()),
m_p_text(new sf::Text(font, "", 20U))
{
	player->setController(this);
	generateTextures();

	m_p_text->setPosition(sf::Vector2f(width, 25.f));
	updateText(0);

	m_p_slider->setSize(sf::Vector2f(width, 20.f));
	m_p_slider->setOnValueChangeCallback([this](int value) {
		m_p_player->setFrameNum(value);
	});
	addElement(m_p_slider);

	gui::Button* playPauseButton = new gui::Button(font);
	playPauseButton->setIcon(*m_p_pauseTexture);
	playPauseButton->setSize(sf::Vector2f(30.f, 30.f));
	playPauseButton->setPosition(sf::Vector2f(35.f, 25.f));
	playPauseButton->setCallback([this, playPauseButton]() {
		m_isPaused = !m_isPaused;
		playPauseButton->setIcon(*(m_isPaused ? m_p_playTexture : m_p_pauseTexture));
	});
	addElement(playPauseButton);

	gui::Button* stopButton = new gui::Button(font);
	stopButton->setIcon(*m_p_stopTexture);
	stopButton->setSize(sf::Vector2f(30.f, 30.f));
	stopButton->setPosition(sf::Vector2f(0.f, 25.f));
	stopButton->setCallback([this, playPauseButton]() {
		m_isPaused = true;
		m_p_player->setFrameNum(0);
		playPauseButton->setIcon(*(m_isPaused ? m_p_playTexture : m_p_pauseTexture));
	});
	addElement(stopButton);

	m_p_slider->setOnGrabbedCallback([this, playPauseButton]() {
		m_wasPaused = m_isPaused;
		m_isPaused = true;
		playPauseButton->setIcon(*(m_isPaused ? m_p_playTexture : m_p_pauseTexture));
	});
	m_p_slider->setOnReleasedCallback([this, playPauseButton]() {
		m_isPaused = m_wasPaused;
		playPauseButton->setIcon(*(m_isPaused ? m_p_playTexture : m_p_pauseTexture));
	});
}

gui::PlayerController::~PlayerController() {
	delete m_p_text;
	delete m_p_pauseTexture;
	delete m_p_playTexture;
	delete m_p_stopTexture;
}

void gui::PlayerController::onFileOpen(size_t framesCount) {
	m_p_slider->setRange(0, framesCount);
	m_p_slider->setValue(0);
	updateText(0);
}

void gui::PlayerController::onNewFrame(size_t frameNum) {
	m_p_slider->setValue(frameNum);
	updateText(frameNum);
}

bool gui::PlayerController::isPaused() const {
	return m_isPaused;
}

void gui::PlayerController::draw(sf::RenderTarget& target, sf::RenderStates states) const {
	Container::draw(target, states);
	target.draw(*m_p_text, states);
}

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>

void gui::PlayerController::generateTextures() {
	sf::RenderTexture renderTarget;
	renderTarget.resize(sf::Vector2u(30, 30));

	auto drawTo = [&renderTarget](sf::Shape& shape, sf::Texture* dstTexture) {
		renderTarget.clear(sf::Color::Transparent);
		renderTarget.draw(shape);
		renderTarget.display();
		dstTexture->loadFromImage(renderTarget.getTexture().copyToImage());
	};

	{
		sf::RectangleShape shape(sf::Vector2f(20, 20));
		shape.setPosition(sf::Vector2f(5.f, 5.f));
		drawTo(shape, m_p_stopTexture);
	}

	{
		sf::ConvexShape shape(3);
		shape.setPoint(0, sf::Vector2f(5.f, 5.f));
		shape.setPoint(2, sf::Vector2f(25.f, 15.f));
		shape.setPoint(1, sf::Vector2f(5.f, 25.f));
		drawTo(shape, m_p_playTexture);
	}

	{
		sf::RectangleShape shape(sf::Vector2f(9.f, 20.f));
		renderTarget.clear(sf::Color::Transparent);
		shape.setPosition(sf::Vector2f(4.f, 5.f));
		renderTarget.draw(shape);
		shape.setPosition(sf::Vector2f(16.f, 5.f));
		renderTarget.draw(shape);
		renderTarget.display();
		m_p_pauseTexture->loadFromImage(renderTarget.getTexture().copyToImage());
	}
}

void gui::PlayerController::updateText(size_t currentFrame) {
	m_p_text->setString("Frame: " + std::to_string(currentFrame + 1) + '/' + std::to_string(m_p_player->getFramesCount()));
	m_p_text->setOrigin(sf::Vector2f(m_p_text->getGlobalBounds().size.x, 0.f));
}