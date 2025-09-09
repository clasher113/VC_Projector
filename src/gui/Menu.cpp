#include "Menu.hpp"

#include "Button.hpp"
#include "Container.hpp"
#include "portable-file-dialogs.h"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include <cstring>

gui::Menu::Menu(const sf::Font& font, const sf::Vector2u& canvasSize, float width) :
	m_p_texture(new sf::Texture),
	m_p_sprite(new sf::Sprite),
	m_p_canvas(new sf::RenderTexture()),
	m_p_canvasSprite(new sf::Sprite)
{
	onSizeChange(canvasSize);

	m_p_imageContainer = new gui::Container;
	m_p_imageContainer->setPosition(sf::Vector2f(5.f, 65.f));

	gui::Button* button = new gui::Button(font);
	button->setSize(sf::Vector2f(width, 30.f));
	button->setText("Choose Image");
	button->setCallback([this]() {
		pfd::open_file path("Fuck", "", { "Images", "*.png *.jpg *.bmp *.tga *.gif" });
		if (path.result().empty()) return;
		if (!m_p_texture->loadFromFile(path.result().back())) {
			std::cout << "Loading error" << std::endl;
		}
		m_p_sprite->setTexture(*m_p_texture, true);
		updateContent();
	});
	m_p_imageContainer->addElement(button);

	button = new gui::Button(font);
	button->setSize(sf::Vector2f(width, 30.f));
	button->setPosition(sf::Vector2f(0.f, 35.f));
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
	button->setPosition(sf::Vector2f(0.f, 70.f));
	auto aspectRatioText = [this]() {
		return std::string("Keep aspect ratio: ") + (m_keepAspectratio ? "True" : "False");
	};
	button->setText(aspectRatioText());
	button->setCallback([this, button, aspectRatioText]() {
		m_keepAspectratio = !m_keepAspectratio;
		updateContent();
		button->setText(aspectRatioText());
	});
	m_p_imageContainer->addElement(button);

	button = new gui::Button(font);
	auto setMode = [this, button]() {
		clearMainContainer();
		std::string modeStr = "Mode: ";
		int height = 0;
		switch (m_mode) {
			case Mode::SCREEN:
				height = 65;
				modeStr.append("Screen");
				break;
			case Mode::IMAGE:
				height = 170;
				modeStr.append("Image");
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

	button->setPosition(sf::Vector2f(5.f, 30.f));
	button->setSize(sf::Vector2f(width, 30.f));
	m_p_mainContainer->addElement(button);

}

gui::Menu::~Menu() {
	if (m_p_contentPixels) delete[] m_p_contentPixels;
	delete m_p_texture;
	delete m_p_sprite;
	delete m_p_canvas;
	delete m_p_canvasSprite;
	clearMainContainer();
	delete m_p_mainContainer;
	delete m_p_imageContainer;
}

void gui::Menu::onEvent(const sf::Event& event, bool& refreshFlag, const sf::Vector2f& offset) {
	m_p_mainContainer->onEvent(event, refreshFlag, offset);
}

void gui::Menu::onSizeChange(const sf::Vector2u& newSize) {
	m_p_canvas->create(newSize.x, newSize.y);
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
	return m_p_mainContainer->getSize();
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
	m_p_sprite->setScale((m_keepAspectratio ? std::min(size.x, size.y) : size.x) / bounds.width,
		(m_keepAspectratio ? std::min(size.x, size.y) : size.y) / bounds.height);

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