#include "GifPlayer.hpp"

#include "gui/PlayerController.hpp"

#include <SFML/Graphics/Image.hpp>
#include <iostream>
#include <cstring>
#ifdef _WIN32
#define NOMINMAX
#define LEAN_AND_MEAN
#include <Windows.h>
#include <fcntl.h>
#endif // _WIN32

constexpr size_t KEY_FRAMES_INTERVAL = 40; // frames

GifPlayer::GifPlayer() {}

GifPlayer::~GifPlayer() {
    close();
}

bool GifPlayer::openFile(const std::filesystem::path& filePath) {
    close();
    int error = D_GIF_SUCCEEDED;
#ifdef _WIN32
	const int len = MultiByteToWideChar(CP_UTF8, 0, filePath.string().c_str(), (int)filePath.string().size(), nullptr, 0);
	std::wstring wFilePath(len, '\0');
	MultiByteToWideChar(CP_UTF8, 0, filePath.string().c_str(), (int)filePath.string().size(), (LPWSTR)wFilePath.data(), (int)wFilePath.size());
	int fileHandle;
	_wsopen_s(&fileHandle, wFilePath.c_str(), _O_RDONLY, _SH_DENYWR, _S_IREAD);
	if (fileHandle != -1) {
		m_p_gifFile = DGifOpenFileHandle(fileHandle, &error);
	}
#else
	m_p_gifFile = DGifOpenFileName(filePath.string().c_str(), &error);
#endif // _WIN32
    if (!m_p_gifFile) {
        std::cout << "DGifOpenFileName() failed - " << error << std::endl;
        return false;
    }
    if (DGifSlurp(m_p_gifFile) == GIF_ERROR) {
        std::cout << "DGifSlurp() failed - " << m_p_gifFile->Error << std::endl;
		close();
        return false;
    }
	if (m_p_controller) {
		m_p_controller->onFileOpen(m_p_gifFile->ImageCount - 1);
		m_p_controller->onNewFrame(m_currentFrame);
	}
	m_p_pixels = new uint8_t[m_p_gifFile->SWidth * m_p_gifFile->SHeight * 4];

	m_framesData.resize(m_p_gifFile->ImageCount);
	for (; m_currentFrame < m_p_gifFile->ImageCount; m_currentFrame++) {
		const SavedImage& saved = m_p_gifFile->SavedImages[m_currentFrame];
		FrameData& data = m_framesData[m_currentFrame];

		data.m_rect = sf::IntRect(
			sf::Vector2i(std::max(saved.ImageDesc.Left, 0), std::max(saved.ImageDesc.Top, 0)),
			sf::Vector2i(std::min(saved.ImageDesc.Left + saved.ImageDesc.Width, m_p_gifFile->SWidth), std::min(saved.ImageDesc.Top + saved.ImageDesc.Height, m_p_gifFile->SHeight))
		);

		data.m_gcb.TransparentColor = NO_TRANSPARENT_COLOR;
		for (const ExtensionBlock* extensionBlock = saved.ExtensionBlocks + saved.ExtensionBlockCount; extensionBlock-- != saved.ExtensionBlocks;) {
			if (extensionBlock->Function == GRAPHICS_EXT_FUNC_CODE && DGifExtensionToGCB(extensionBlock->ByteCount, extensionBlock->Bytes, &data.m_gcb) == GIF_OK) {
				data.m_duration = static_cast<float>(data.m_gcb.DelayTime) / 100.f;
				break;
			}
		}
		if (m_p_gifFile->ImageCount > KEY_FRAMES_INTERVAL) {
			nextFrame();
			if (m_currentFrame % KEY_FRAMES_INTERVAL == 0) {
				m_keyFrames.emplace(m_currentFrame, sf::Image(getSize(), m_p_pixels));
			}
		}
	}
	m_currentFrame = 0;
	nextFrame();

    return true;
}

void GifPlayer::nextFrame() {
	const auto it = m_keyFrames.find(m_currentFrame);
	if (it != m_keyFrames.end()) {
		memcpy(m_p_pixels, it->second.getPixelsPtr(), m_p_gifFile->SWidth * m_p_gifFile->SHeight * 4);
	}
	else {
		const SavedImage& saved = m_p_gifFile->SavedImages[m_currentFrame];
		const GifImageDesc& desc = saved.ImageDesc;
		const ColorMapObject* colorMap = desc.ColorMap ? desc.ColorMap : m_p_gifFile->SColorMap;
		const FrameData& frameData = m_framesData[m_currentFrame];
		const sf::IntRect& rect = frameData.m_rect;

		if (frameData.m_gcb.TransparentColor >= 0 || rect.position.x > 0 || rect.size.x < m_p_gifFile->SWidth || rect.position.y > 0 || 
			rect.size.y < m_p_gifFile->SHeight || frameData.m_gcb.DisposalMode == DISPOSE_PREVIOUS) {
			if (!m_currentFrame) memset(m_p_pixels, 0, m_p_gifFile->SWidth * m_p_gifFile->SHeight * 4);
			else if (m_disposal == DISPOSE_BACKGROUND || m_disposal == DISPOSE_PREVIOUS) {
				const sf::IntRect& prevRect = m_framesData[m_currentFrame - 1].m_rect;
				uint8_t* dst = m_p_pixels + (m_p_gifFile->SWidth * prevRect.position.y + prevRect.position.x) * 4;
				if (m_disposal == DISPOSE_PREVIOUS) {
					uint8_t* src = m_p_pixels + m_p_gifFile->SWidth * m_p_gifFile->SHeight * 4;
					for (int y = 0; y < prevRect.size.y - prevRect.position.y; ++y) {
						memcpy(dst, src, (prevRect.size.x - prevRect.position.x) * 4);
						dst += m_p_gifFile->SWidth * 4;
						src += (prevRect.size.x - prevRect.position.x) * 4;
					}
				}
				else {
					for (int y = 0; y < prevRect.size.y - prevRect.position.y; ++y) {
						memset(dst, 0, (prevRect.size.x - prevRect.position.x) * 4);
						dst += m_p_gifFile->SWidth * 4;
					}
				}
			}
		}

		m_disposal = frameData.m_gcb.DisposalMode;
		if (m_disposal == DISPOSE_PREVIOUS) {
			if (m_currentFrame && m_currentFrame < m_p_gifFile->ImageCount - 1) {
				uint8_t* dst = m_p_pixels + m_p_gifFile->SWidth * m_p_gifFile->SHeight * 4;
				uint8_t* src = m_p_pixels + (m_p_gifFile->SWidth * rect.position.y + rect.position.x) * 4;
				for (int y = 0; y < rect.size.y - rect.position.y; ++y) {
					memcpy(dst, src, (rect.size.x - rect.position.x) * 4);
					dst += (rect.size.x - rect.position.x) * 4;
					src += m_p_gifFile->SWidth * 4;
				}
			}
			else m_disposal = DISPOSE_PREVIOUS;
		}

		for (int y = rect.position.y; y < rect.size.y; ++y) {
			const GifByteType* src = saved.RasterBits + (desc.Width * (y - desc.Top) + (rect.position.x - desc.Left));
			for (int x = rect.position.x; x < rect.size.x; ++x) {
				int i = int(*src++);
				i *= (unsigned)i < (unsigned)colorMap->ColorCount;
				GifColorType color = colorMap->Colors[i];
				if (colorMap && i != frameData.m_gcb.TransparentColor) {
					uint8_t* dst = &m_p_pixels[(y * m_p_gifFile->SWidth + x) * 4];
					dst[0] = color.Red;
					dst[1] = color.Green;
					dst[2] = color.Blue;
					dst[3] = 255;
				}
			}
		}
	}
	
	if (m_p_controller) m_p_controller->onNewFrame(m_currentFrame);
	m_hasNewFrame = true;
}

void GifPlayer::update(float delta) {
	if (getFramesCount() < 2) return;
	m_animationTimer += delta;
	while (m_animationTimer > getCurrentFrameDuration()) {
		m_animationTimer -= getCurrentFrameDuration();
		m_currentFrame++;
		if (m_currentFrame >= m_p_gifFile->ImageCount) m_currentFrame = 0;
		nextFrame();
	}
}

void GifPlayer::setFrameNum(size_t frame) {
	if (!m_p_gifFile) return;
	if (m_currentFrame == frame) return;
	if (frame >= m_p_gifFile->ImageCount) frame = 0;

	size_t closestKeyFrame = 0;
	for (const auto& [frameNum, _] : m_keyFrames) {
		if (frameNum > frame) break;
		closestKeyFrame = frameNum;
	}
	if (frame < m_currentFrame || frame - m_currentFrame > frame - closestKeyFrame) m_currentFrame = closestKeyFrame;

	while (true) {
		nextFrame();
		if (m_currentFrame >= frame) {
			break;
		}
		m_currentFrame++;
	}
}

void GifPlayer::setController(gui::PlayerController* controller) {
	m_p_controller = controller;
}

const uint8_t* const GifPlayer::getPixels() {
	return m_p_pixels;
}

size_t GifPlayer::getFramesCount() const {
	return m_p_gifFile ? m_p_gifFile->ImageCount : 0;
}

sf::Vector2u GifPlayer::getSize() const {
	return m_p_gifFile ? sf::Vector2u(m_p_gifFile->SWidth, m_p_gifFile->SHeight) : sf::Vector2u();
}

bool GifPlayer::hasNewFrame() {
	if (m_p_gifFile != nullptr && m_hasNewFrame) {
		m_hasNewFrame = false;
		return true;
	}
	return false;
}

float GifPlayer::getCurrentFrameDuration() const {
	return m_framesData.empty() ? -1 : m_framesData[m_currentFrame].m_duration;
}

void GifPlayer::close() {
	if (m_p_pixels) {
		delete[] m_p_pixels;
		m_p_pixels = nullptr;
	}
    if (m_p_gifFile) {
        int error = D_GIF_SUCCEEDED;
        DGifCloseFile(m_p_gifFile, &error);
		m_p_gifFile = nullptr;
    }
    m_currentFrame = 0;
	m_animationTimer = 0.f;
	m_framesData.clear();
	m_keyFrames.clear();
}
