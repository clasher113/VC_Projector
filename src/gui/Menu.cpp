#include "Menu.hpp"

#include "Button.hpp"
#include "Container.hpp"
#include "portable-file-dialogs.h"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include <gif_lib.h>

#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;

std::vector<Image*> loadGif(const fs::path& path);

struct Image {
	float m_frameDuration;
	sf::Texture* m_p_texture;
};

gui::Menu::Menu(const sf::Font& font, const sf::Vector2u& canvasSize, float width) :
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
		pfd::open_file path("Choose Image", "", { "Images", "*.png *.jpg *.bmp *.tga *.gif" });
		if (path.result().empty()) return;
		clearImages();
		fs::path filePath(path.result().back());
		if (filePath.extension() == ".gif") {
			m_images = loadGif(filePath);
		} else {
			sf::Texture* texture = new sf::Texture;
			if (!texture->loadFromFile(filePath.string())) {
				std::cout << "Loading error" << std::endl;
				delete texture;
			}
			else m_images.emplace_back(new Image{ -1, texture });
		}
		if (!m_images.empty()){
			m_p_sprite->setTexture(*m_images.front()->m_p_texture, true);
		}
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
	clearImages();
	delete m_p_sprite;
	delete m_p_canvas;
	delete m_p_canvasSprite;
	clearMainContainer();
	delete m_p_mainContainer;
	delete m_p_imageContainer;
}

void gui::Menu::onUpdate(const float deltaTime, bool& refreshFlag) {
	if (m_mode == Mode::SCREEN) return;
	if (m_images.size() > 1) {
		m_animationTimer += deltaTime;
		Image* image = m_images[m_currentFrame];
		const size_t lastFrame = m_currentFrame;
		while (m_animationTimer > image->m_frameDuration){
			m_animationTimer -= image->m_frameDuration;
			m_currentFrame += 1;
			if (m_currentFrame >= m_images.size()) m_currentFrame = 0;
			image = m_images[m_currentFrame];
		}
		if (lastFrame != m_currentFrame) {
			m_p_sprite->setTexture(*image->m_p_texture, true);
			updateContent();
			refreshFlag = true;
		}
	}
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

void gui::Menu::clearImages() {
	m_currentFrame = 0;
	m_animationTimer = 0.f;
	for (Image* image : m_images) {
		delete image->m_p_texture;
		delete image;
	}
	m_images.clear();
}

void gui::Menu::updateContent() {
	const sf::FloatRect bounds = m_p_sprite->getLocalBounds();
	const sf::Vector2u size = m_p_canvas->getSize();

	sf::Vector2f scale(size.x / bounds.width, size.y / bounds.height);
	if (m_keepAspectratio) scale = sf::Vector2f(std::min(scale.x, scale.y), std::min(scale.x, scale.y));
	m_p_sprite->setScale(scale);
	m_p_sprite->setPosition((size.x - bounds.width * scale.x) / 2.f, (size.y - bounds.height * scale.y) / 2.f);

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

std::vector<Image*> loadGif(const fs::path& path) {
	std::vector<Image*> result;
	int error = D_GIF_SUCCEEDED;
	GifFileType* gifFile = DGifOpenFileName(path.string().c_str(), &error);
	if (!gifFile) {
		std::cout << "DGifOpenFileName() failed - " << error << std::endl;
		return result;
	}
	if (DGifSlurp(gifFile) == GIF_ERROR) {
		std::cout << "DGifSlurp() failed - " << gifFile->Error << std::endl;
		DGifCloseFile(gifFile, &error);
		return result;
	}

	ColorMapObject* commonMap = gifFile->SColorMap;

	sf::Image sfImage;
	sfImage.create(gifFile->SWidth, gifFile->SHeight);

	for (int i = 0; i < gifFile->ImageCount; ++i) {
		Image* image = new Image;
		image->m_p_texture = new sf::Texture;
		result.emplace_back(image);

		const SavedImage& saved = gifFile->SavedImages[i];
		const GifImageDesc& desc = saved.ImageDesc;
		const ColorMapObject* colorMap = desc.ColorMap ? desc.ColorMap : commonMap;
		GraphicsControlBlock gcb{};
		gcb.TransparentColor = NO_TRANSPARENT_COLOR;

		for (const ExtensionBlock* extensionBlock = gifFile->SavedImages[i].ExtensionBlocks + gifFile->SavedImages[i].ExtensionBlockCount; extensionBlock-- != gifFile->SavedImages[i].ExtensionBlocks;) {
			if (extensionBlock->Function == GRAPHICS_EXT_FUNC_CODE && DGifExtensionToGCB(extensionBlock->ByteCount, extensionBlock->Bytes, &gcb) == GIF_OK) {
				image->m_frameDuration = (gifFile->ImageCount > 1 ? static_cast<float>(gcb.DelayTime) / 100.f : -1);
				break;
			}
		}

		sf::IntRect rect(
			std::max(desc.Left, 0),
			std::max(desc.Top, 0),
			std::min(desc.Left + desc.Width, gifFile->SWidth),
			std::min(desc.Top + desc.Height, gifFile->SHeight)
		);

		for (int y = rect.top; y < rect.height; ++y) {
			const GifByteType* src = saved.RasterBits + (desc.Width * (y - desc.Top) + (rect.left - desc.Left));
			for (int x = rect.left; x < rect.width; ++x) {
				int i = int(*src++);
				i *= (unsigned)i < (unsigned)colorMap->ColorCount;
				GifColorType color = colorMap->Colors[i];
				if (colorMap && i != gcb.TransparentColor) {
					sfImage.setPixel(x, y, sf::Color(color.Red, color.Green, color.Blue));
				}
			}
		}
		image->m_p_texture->loadFromImage(sfImage);
	}
	DGifCloseFile(gifFile, &error);
	return result;
}