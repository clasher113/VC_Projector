#include "Menu.hpp"

#include "Button.hpp"
#include "Container.hpp"
#include "../GifPlayer.hpp"
#include "../Window.hpp"
#include "PlayerController.hpp"
#include "portable-file-dialogs.h"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Image.hpp>

#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;

struct Image {
	float m_frameDuration;
	sf::Texture* m_p_texture;
};

gui::Menu::Menu(vcp::Window& window, const sf::Font& font, const sf::Vector2u& canvasSize, float width) :
	m_p_texture(new sf::Texture),
	m_p_sprite(new sf::Sprite(*m_p_texture)),
	m_p_canvas(new sf::RenderTexture()),
	m_p_canvasSprite(new sf::Sprite(m_p_canvas->getTexture())),
	m_p_gifPlayer(new GifPlayer),
	m_p_playerController(new PlayerController(font, m_p_gifPlayer, width))
{
	onSizeChange(canvasSize);

	m_p_imageContainer = new gui::Container(true);
	m_p_imageContainer->setPosition(sf::Vector2f(0.f, 60.f));
	m_p_imageContainer->setInteval(3.f);

	gui::Button* button = new gui::Button(font);
	button->setSize(sf::Vector2f(width, 30.f));
	button->setText("Choose Image");
	button->setCallback([this, &window]() {
		pfd::open_file path("Choose Image", "", { "Images", "*.png *.jpg *.bmp *.tga *.gif" });
		window.setIconified(true);
		std::vector<std::string> result = path.result();
		window.setIconified(false);
		if (path.result().empty()) return;
		m_p_gifPlayer->close();
		fs::path filePath(path.result().back());
		if (filePath.extension() == ".gif") {
			m_p_imageContainer->removeElement(m_p_playerController);
			m_p_gifPlayer->openFile(filePath);
			if (m_p_gifPlayer->getFramesCount() > 1) m_p_imageContainer->addElement(m_p_playerController, 0);
			if (m_p_gifPlayer->getFramesCount() < 2 || m_p_playerController->isPaused()){
				const sf::Image* const frame = m_p_gifPlayer->nextFrame();
				if (frame && m_p_texture->loadFromImage(*frame)) {
					m_p_sprite->setTexture(*m_p_texture, true);
					updateContent();
				}
			}
			m_justOpened = true;
		} else {
			if (!m_p_texture->loadFromFile(filePath.string())) {
				std::cout << "Loading error" << std::endl;
				return;
			}
			m_p_sprite->setTexture(*m_p_texture, true);
			updateContent();
		}
		if (m_onModeChangeCallback) m_onModeChangeCallback();
	});
	m_p_imageContainer->addElement(button);

	button = new gui::Button(font);
	button->setSize(sf::Vector2f(width, 30.f));
	auto alphaButtonText = [this]() {
		return std::string("Allow alpha: ") + (m_allowAlpha ? "True" : "False");
	};
	button->setText(alphaButtonText());
	button->setCallback([this, button, alphaButtonText]() {
		m_allowAlpha = !m_allowAlpha;
		updateContent();
		button->setText(alphaButtonText());
	});
	m_p_imageContainer->addElement(button);

	m_p_mainContainer = new gui::Container;

	button = new gui::Button(font);
	button->setSize(sf::Vector2f(width, 30.f));
	auto aspectRatioText = [this]() {
		return std::string("Stretch to screen: ") + (m_stretchToScreen ? "True" : "False");
	};
	button->setText(aspectRatioText());
	button->setCallback([this, button, aspectRatioText]() {
		m_stretchToScreen = !m_stretchToScreen;
		updateContent();
		button->setText(aspectRatioText());
	});
	m_p_imageContainer->addElement(button);

	button = new gui::Button(font);
	auto setMode = [this, &window, button]() {
		clearMainContainer();
		std::string modeStr = "Mode: ";
		switch (m_mode) {
			case Mode::SCREEN:
				modeStr.append("Screen");
				window.setAlwaysOnTop(true);
				break;
			case Mode::IMAGE:
				modeStr.append("Image");
				window.setAlwaysOnTop(false);
				m_p_mainContainer->addElement(m_p_imageContainer);
				break;
		}
		if (m_onModeChangeCallback) m_onModeChangeCallback();
		button->setText(modeStr);
	};
	setMode();
	button->setCallback([this, setMode]() {
		incrementEnumClass(m_mode, 1, Mode::IMAGE, Mode::SCREEN);
		setMode();
	});

	button->setPosition(sf::Vector2f(3.f, 25.f));
	button->setSize(sf::Vector2f(width, 30.f));
	m_p_mainContainer->addElement(button);
}

gui::Menu::~Menu() {
	if (m_p_contentPixels) delete[] m_p_contentPixels;
	delete m_p_gifPlayer;
	delete m_p_sprite;
	delete m_p_canvas;
	delete m_p_canvasSprite;
	clearMainContainer();
	delete m_p_mainContainer;
	delete m_p_imageContainer;
}

void gui::Menu::onUpdate(const float deltaTime, bool& refreshFlag) {
	if (m_mode == Mode::SCREEN) return;
	if (m_justOpened) {
		m_justOpened = false;
		return;
	}
	if (m_p_gifPlayer->getFramesCount() < 2) return;
	m_p_gifPlayer->update(m_p_playerController->isPaused() ? 0.f : deltaTime);
	const sf::Image* newFrame = m_p_gifPlayer->nextFrame();
	if (newFrame && m_p_texture->loadFromImage(*newFrame)) {
		m_p_sprite->setTexture(*m_p_texture, true);
		updateContent();
		refreshFlag = true;
	}
}

void gui::Menu::onEvent(const sf::Event& event, bool& refreshFlag, const sf::Vector2f& offset) {
	m_p_mainContainer->onEvent(event, refreshFlag, offset);
}

void gui::Menu::onSizeChange(const sf::Vector2u& newSize) {
	if (!m_p_canvas->resize(sf::Vector2u(newSize.x, newSize.y))) return;
	m_p_canvasSprite->setTexture(m_p_canvas->getTexture(), true);
	if (m_p_contentPixels) delete[] m_p_contentPixels;
	m_p_contentPixels = new sf::Color[newSize.x * newSize.y];
	updateContent();
}

void gui::Menu::setOnModeChangeCallback(const std::function<void()>& callback) {
	m_onModeChangeCallback = callback;
}

const sf::Drawable& gui::Menu::getContent() const {
	return *m_p_canvasSprite;
}

const sf::Color* gui::Menu::getContentPixels() const {
	return m_p_contentPixels;
}

Mode gui::Menu::getMode() const {
	return m_mode;
}

sf::Vector2f gui::Menu::getSize() const {
	if (m_mode == Mode::SCREEN) return m_p_mainContainer->getSize() + sf::Vector2f(3.f, 3.f);
	else return m_p_mainContainer->getSize();
}

bool gui::Menu::isCursorOverElement(const sf::Vector2f& cursorPos) {
	return sf::FloatRect(m_p_mainContainer->getPosition(), m_p_mainContainer->getSize()).contains(cursorPos);
}

void gui::Menu::draw(sf::RenderTarget& target, sf::RenderStates states) const {
	target.draw(*m_p_mainContainer, states);
}

void gui::Menu::clearMainContainer() {
	m_p_mainContainer->removeElement(m_p_imageContainer);
}

void gui::Menu::updateContent() {
	const sf::FloatRect bounds = m_p_sprite->getLocalBounds();
	const sf::Vector2u size = m_p_canvas->getSize();

	sf::Vector2f scale(size.x / bounds.size.x, size.y / bounds.size.y);
	if (!m_stretchToScreen) scale = sf::Vector2f(std::min(scale.x, scale.y), std::min(scale.x, scale.y));
	m_p_sprite->setScale(scale);
	m_p_sprite->setPosition(sf::Vector2f((size.x - bounds.size.x * scale.x) / 2.f, (size.y - bounds.size.y * scale.y) / 2.f));

	m_p_canvas->clear(m_allowAlpha ? sf::Color::Transparent : sf::Color::Black);
	m_p_canvas->draw(*m_p_sprite);
	m_p_canvas->display();

	sf::Image image = m_p_canvas->getTexture().copyToImage();
	image.flipVertically();
	memcpy(m_p_contentPixels, image.getPixelsPtr(), image.getSize().x * image.getSize().y * 4);

	for (size_t i = 0; i < image.getSize().x * image.getSize().y; i++) {
		std::swap(m_p_contentPixels[i].r, m_p_contentPixels[i].b);
	}
}