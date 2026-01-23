#include "GifPlayer.hpp"

#include "gui/PlayerController.hpp"
#include "Util.hpp"

#include <SFML/Graphics/Image.hpp>
#include <cstring>
#ifdef _WIN32
#include <fcntl.h>
#endif // _WIN32

static std::string getErrorCode(int code);

constexpr size_t KEY_FRAMES_INTERVAL = 40; // frames

GifPlayer::GifPlayer() {}

GifPlayer::~GifPlayer() {
    close();
}

bool GifPlayer::openFile(const std::filesystem::path& filePath) {
	close();
	m_currentFilePath = filePath;

	if (!openFile()) {
		close();
		return false;
	}
	m_p_pixels = new uint8_t[m_p_gifFile->SWidth * m_p_gifFile->SHeight * 4];
	m_p_gifPixels = new GifPixelType[m_p_gifFile->SWidth * m_p_gifFile->SHeight];

	if (!fetchInfo()) {
		close();
		return false;
	}

	m_currentFrame = 0;
	if (m_p_controller) {
		m_p_controller->onFileOpen(m_framesTotal - 1);
		m_p_controller->onNewFrame(m_currentFrame);
	}

	nextFrame(true);

    return true;
}

bool GifPlayer::nextFrame(bool savePixels) {
	const static auto onEof = [this]() {
		int errorCode = D_GIF_SUCCEEDED;
		if (DGifCloseFile(m_p_gifFile, &errorCode) != GIF_OK) {
			showError("DGifCloseFile() error: " + getErrorCode(errorCode));
			return false;
		}
		if (!openFile()) {
			return false;
		}
		m_currentFrame = 0;
		return true;
	};

	if (m_eof) {
		if (!onEof()) {
			return false;
		}
	}

	bool frameDone = false;

	GraphicsControlBlock gcb;
	while (!frameDone) {
		GifRecordType gifRecordType = GifRecordType::UNDEFINED_RECORD_TYPE;
		if (!nextRecordType(&gifRecordType)) {
			return false;
		}

		switch (gifRecordType) {
			case GifRecordType::UNDEFINED_RECORD_TYPE:
			case GifRecordType::TERMINATE_RECORD_TYPE:
				m_eof = true;
				if (!onEof()) {
					return false;
				}
				break;
			case GifRecordType::EXTENSION_RECORD_TYPE:
				if (!readExtention(gcb)) {
					return false;
				}
				break;
			case GifRecordType::IMAGE_DESC_RECORD_TYPE:
				if (DGifGetImageDesc(m_p_gifFile) == GIF_OK) {
					const auto it = m_keyFrames.find(m_currentFrame);
					if (it != m_keyFrames.end()) {
						memcpy(m_p_pixels, it->second.getPixelsPtr(), m_p_gifFile->SWidth * m_p_gifFile->SHeight * 4);
					}
					if (!readFrame(gcb, savePixels && it == m_keyFrames.end())) {
						return false;
					}
					m_hasNewFrame = frameDone = true;
				}
				else {
					showError("DGifGetImageDesc() error");
					return false;
				}
				break;
		}
	}
	
	if (m_p_controller) m_p_controller->onNewFrame(m_currentFrame);
	return true;
}

bool GifPlayer::nextRecordType(GifRecordType* gifRecordType) {
	if (DGifGetRecordType(m_p_gifFile, gifRecordType) == GIF_ERROR) {
		showError("DGifGetRecordType error");
		return false;
	}
	return true;
}

bool GifPlayer::readExtention(GraphicsControlBlock& gcb) {
	int errorCode = D_GIF_SUCCEEDED;
	GifByteType* gifExtention = nullptr;

	if (DGifGetExtension(m_p_gifFile, &errorCode, &gifExtention) == GIF_OK) {
		DGifExtensionToGCB(gifExtention[0], &gifExtention[1], &gcb);
		while (true) {
			if (DGifGetExtensionNext(m_p_gifFile, &gifExtention) == GIF_ERROR) {
				showError("DGifGetExtensionNext() error");
				return false;
			}
			if (gifExtention == NULL) {
				break;
			}
			DGifExtensionToGCB(gifExtention[0], &gifExtention[1], &gcb);
		}
	}
	else {
		showError("DGifGetExtension() error: " + getErrorCode(errorCode));
		return false;
	}
	return true;
}

bool GifPlayer::readFrame(const GraphicsControlBlock& gcb, bool savePixels) {
	const GifImageDesc& imageDesc = m_p_gifFile->Image;
	const ColorMapObject* colorMap = imageDesc.ColorMap ? imageDesc.ColorMap : m_p_gifFile->SColorMap;

	if (savePixels) {
		if (imageDesc.Interlace) {
			constexpr static int InterlacedOffset[] = { 0, 4, 2, 1 };
			constexpr static int InterlacedJumps[] = { 8, 8, 4, 2 };

			for (int i = 0; i < 4; i++) {
				for (int j = InterlacedOffset[i]; j < imageDesc.Height; j += InterlacedJumps[i]) {
					if (DGifGetLine(m_p_gifFile, m_p_gifPixels + j * imageDesc.Width, imageDesc.Width) == GIF_ERROR) {
						showError("DGifGetLine() error");
						return false;
					}
				}
			}
		}
		else {
			if (DGifGetLine(m_p_gifFile, m_p_gifPixels, imageDesc.Width * imageDesc.Height) == GIF_ERROR) {
				showError("DGifGetLine() error");
				return false;
			}
		}
	} 
	else {
		GifByteType* codeBlocks;
		do {
			if (DGifGetCodeNext(m_p_gifFile, &codeBlocks) == GIF_ERROR) {
				showError("DGifGetCodeNext() error");
				return false;
			}
		}
		while (codeBlocks != NULL);
		return true;
	}

	const sf::IntRect& rect = m_framesData[m_currentFrame].m_rect;

	if (gcb.TransparentColor >= 0 || rect.position.x > 0 || rect.size.x < m_p_gifFile->SWidth || rect.position.y > 0 ||
			rect.size.y < m_p_gifFile->SHeight || gcb.DisposalMode == DISPOSE_PREVIOUS) {
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

	m_disposal = gcb.DisposalMode;
	if (m_disposal == DISPOSE_PREVIOUS) {
		if (m_currentFrame && !m_eof) {
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
		const GifByteType* src = m_p_gifPixels + (imageDesc.Width * (y - imageDesc.Top) + (rect.position.x - imageDesc.Left));
		for (int x = rect.position.x; x < rect.size.x; ++x) {
			int i = int(*src++);
			i *= (unsigned)i < (unsigned)colorMap->ColorCount;
			GifColorType color = colorMap->Colors[i];
			if (colorMap && i != gcb.TransparentColor) {
				uint8_t* dst = &m_p_pixels[(y * m_p_gifFile->SWidth + x) * 4];
				dst[0] = color.Red;
				dst[1] = color.Green;
				dst[2] = color.Blue;
				dst[3] = 255;
			}
		}
	}

	return true;
}

void GifPlayer::update(float delta) {
	if (getFramesCount() < 2) return;
	m_animationTimer += delta;
	while (m_animationTimer > getCurrentFrameDuration()) {
		m_animationTimer -= getCurrentFrameDuration();
		m_currentFrame++;
		if (m_currentFrame >= getFramesCount()) {
			m_eof = true;
		}
		if (!nextFrame(true)) {
			close();
			return;
		}
	}
}

void GifPlayer::setFrameNum(size_t frame) {
	if (!m_p_gifFile) return;
	if (m_currentFrame == frame) return;
	if (frame >= getFramesCount()) frame = 0;
	if (frame < m_currentFrame) {
		m_eof = true;
	}
	else m_currentFrame++;

	size_t closestKeyFrame = 0;
	for (const auto& [frameNum, _] : m_keyFrames) {
		if (frameNum > frame) break;
		closestKeyFrame = frameNum;
	}

	while (true) {
		if (!nextFrame(closestKeyFrame >= KEY_FRAMES_INTERVAL ? m_currentFrame > closestKeyFrame : true)) {
			close();
			return;
		}
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
	return m_framesTotal;
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
	if (m_p_gifPixels) {
		delete[] m_p_gifPixels;
		m_p_gifPixels = nullptr;
	}
    if (m_p_gifFile) {
        int error = D_GIF_SUCCEEDED;
        DGifCloseFile(m_p_gifFile, &error);
		m_p_gifFile = nullptr;
    }
	m_eof = true;
	m_hasNewFrame = false;
    m_currentFrame = m_framesTotal = 0;
	m_animationTimer = m_currentFrameDuration = 0.f;
	m_currentFilePath.clear();
	m_framesData.clear();
	m_keyFrames.clear();
}

bool GifPlayer::openFile() {
	int error = D_GIF_SUCCEEDED;
#ifdef _WIN32
	int fileHandle;
	_wsopen_s(&fileHandle, m_currentFilePath.c_str(), _O_RDONLY, _SH_DENYWR, _S_IREAD);
	if (fileHandle != -1) {
		m_p_gifFile = DGifOpenFileHandle(fileHandle, &error);
	}
#else
	m_p_gifFile = DGifOpenFileName(m_currentFilePath.string().c_str(), &error);
#endif // _WIN32
	if (!m_p_gifFile) {
		showError("DGifOpenFileName() failed - " + std::to_string(error));
		return false;
	}
	m_eof = false;

	return true;
}

bool GifPlayer::fetchInfo() {
	GraphicsControlBlock gcb;

	while (!m_eof) {
		GifRecordType gifRecordType = GifRecordType::UNDEFINED_RECORD_TYPE;
		if (!nextRecordType(&gifRecordType)) {
			return false;
		}

		switch (gifRecordType) {
			case GifRecordType::UNDEFINED_RECORD_TYPE:
			case GifRecordType::TERMINATE_RECORD_TYPE:
				m_eof = true;
				break;
			case GifRecordType::EXTENSION_RECORD_TYPE:
				if (!readExtention(gcb)) {
					return false;
				}
				break;
			case GifRecordType::IMAGE_DESC_RECORD_TYPE:
				if (DGifGetImageDesc(m_p_gifFile) == GIF_OK) {
					GifPlayer::FrameData& frameData = m_framesData.emplace_back();
					const GifImageDesc& imageDesc = m_p_gifFile->Image;

					frameData.m_rect = sf::IntRect(
						sf::Vector2i(std::max(imageDesc.Left, 0), std::max(imageDesc.Top, 0)),
						sf::Vector2i(std::min(imageDesc.Left + imageDesc.Width, m_p_gifFile->SWidth),
						std::min(imageDesc.Top + imageDesc.Height, m_p_gifFile->SHeight))
					);
					frameData.m_duration = static_cast<float>(gcb.DelayTime) / 100.f;

					if (readFrame(gcb, true)){
						if (m_framesTotal >= KEY_FRAMES_INTERVAL) {
							if (m_currentFrame % KEY_FRAMES_INTERVAL == 0) {
								m_keyFrames.emplace(m_currentFrame, sf::Image(getSize(), m_p_pixels));
							}
						}
						m_framesTotal++;
						m_currentFrame = m_framesTotal;
					}
					else return false;
				}
				else return false;
				break;
		}
	}

	return true;
}

std::string getErrorCode(int code) {
	if (const char* string = GifErrorString(code)) {
		return string;
	}
	return "Undefined error";
}
