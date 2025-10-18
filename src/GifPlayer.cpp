#include "GifPlayer.hpp"

#include "gui/PlayerController.hpp"

#include <SFML/Graphics/Image.hpp>
#include <iostream>

GifPlayer::GifPlayer() :
m_p_pixels(new sf::Image())
{
}

GifPlayer::~GifPlayer() {
    close();
	delete m_p_pixels;
}

bool GifPlayer::openFile(const std::filesystem::path& filePath) {
	m_currentFrame = 0;
    if (m_p_gifFile) close();
    int error = D_GIF_SUCCEEDED;
	m_p_gifFile = DGifOpenFileName(filePath.string().c_str(), &error);
    if (!m_p_gifFile) {
        std::cout << "DGifOpenFileName() failed - " << error << std::endl;
        return false;
    }
    if (DGifSlurp(m_p_gifFile) == GIF_ERROR) {
        std::cout << "DGifSlurp() failed - " << m_p_gifFile->Error << std::endl;
		close();
        return false;
    }
	m_p_pixels->resize(sf::Vector2u(m_p_gifFile->SWidth, m_p_gifFile->SHeight), sf::Color::Transparent);

	m_framesData.resize(m_p_gifFile->ImageCount);
	for (int i = 0; i < m_p_gifFile->ImageCount; i++) {
		const SavedImage& saved = m_p_gifFile->SavedImages[i];
		FrameData& data = m_framesData[i];

		data.m_gcb.TransparentColor = NO_TRANSPARENT_COLOR;

		for (const ExtensionBlock* extensionBlock = saved.ExtensionBlocks + saved.ExtensionBlockCount; extensionBlock-- != saved.ExtensionBlocks;) {
			if (extensionBlock->Function == GRAPHICS_EXT_FUNC_CODE && DGifExtensionToGCB(extensionBlock->ByteCount, extensionBlock->Bytes, &data.m_gcb) == GIF_OK) {
				data.m_duration = static_cast<float>(data.m_gcb.DelayTime) / 100.f;
				break;
			}
		}
	}
	if (m_p_controller) {
		m_p_controller->onFileOpen(m_p_gifFile->ImageCount);
		m_p_controller->onNewFrame(m_currentFrame);
	}

    return true;
}

const sf::Image* const GifPlayer::nextFrame() {
    if (m_p_gifFile->ImageCount < 2) return m_p_pixels;
	const SavedImage& saved = m_p_gifFile->SavedImages[m_currentFrame];
	const GifImageDesc& desc = saved.ImageDesc;
	const ColorMapObject* colorMap = desc.ColorMap ? desc.ColorMap : m_p_gifFile->SColorMap;

	sf::IntRect rect(
		sf::Vector2i(std::max(desc.Left, 0), std::max(desc.Top, 0)),
		sf::Vector2i(std::min(desc.Left + desc.Width, m_p_gifFile->SWidth), std::min(desc.Top + desc.Height, m_p_gifFile->SHeight))
	);

	for (int y = rect.position.y; y < rect.size.y; ++y) {
		const GifByteType* src = saved.RasterBits + (desc.Width * (y - desc.Top) + (rect.position.x - desc.Left));
		for (int x = rect.position.x; x < rect.size.x; ++x) {
			int i = int(*src++);
			i *= (unsigned)i < (unsigned)colorMap->ColorCount;
			GifColorType color = colorMap->Colors[i];
			if (colorMap && i != m_framesData[m_currentFrame].m_gcb.TransparentColor) {
				m_p_pixels->setPixel(sf::Vector2u(x, y), sf::Color(color.Red, color.Green, color.Blue));
			}
		}
	}
	m_currentFrame++;
	if (m_currentFrame >= m_p_gifFile->ImageCount) m_currentFrame = 0;
	if (m_p_controller) m_p_controller->onNewFrame(m_currentFrame);

    return m_p_pixels;
}

void GifPlayer::setFrameNum(size_t frame) {
	if (!m_p_gifFile) return;
	if (frame >= m_p_gifFile->ImageCount) frame = 0;
	if (frame < m_currentFrame) m_currentFrame = 0;
	while (m_currentFrame < frame) {
		nextFrame();
	}
}

void GifPlayer::setController(gui::PlayerController* controller) {
	m_p_controller = controller;
}

size_t GifPlayer::getFramesCount() const {
	return m_p_gifFile ? m_p_gifFile->ImageCount - 1 : 0;
}

float GifPlayer::getCurrentFrameDuration() const {
	return m_framesData.empty() ? -1 : m_framesData[m_currentFrame].m_duration;
}

void GifPlayer::close() {
    if (m_p_gifFile) {
        int error = D_GIF_SUCCEEDED;
        DGifCloseFile(m_p_gifFile, &error);
		m_p_gifFile = nullptr;
    }
    m_currentFrame = 0;
	m_framesData.clear();
}
