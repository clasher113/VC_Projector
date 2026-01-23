#pragma once

#include <filesystem>
#include <vector>
#include <map>
#include <gif_lib.h>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>

namespace sf {
    class Image;
}

namespace gui {
    class PlayerController;
};

class GifPlayer {
public:
    GifPlayer();
    ~GifPlayer();

    bool openFile(const std::filesystem::path& filePath);
    void close();

    void update(float delta);
    void setFrameNum(size_t frame);
    void setController(gui::PlayerController* controller);
    
    const uint8_t* const getPixels();
    size_t getFramesCount() const;
    sf::Vector2u getSize() const;
    bool hasNewFrame();

private:
    struct FrameData {
        float m_duration;
        GraphicsControlBlock m_gcb;
        sf::IntRect m_rect;
    };
    bool m_eof = true;
    bool m_hasNewFrame = false;
    float m_animationTimer = 0.f, m_currentFrameDuration = 0.f;
    int m_disposal = DISPOSAL_UNSPECIFIED;
    size_t m_currentFrame = 0, m_framesTotal = 0;
    std::vector<FrameData> m_framesData;
    std::map<size_t, sf::Image> m_keyFrames;
    GifFileType* m_p_gifFile = nullptr;
    std::filesystem::path m_currentFilePath;
    
    gui::PlayerController* m_p_controller = nullptr;

    uint8_t* m_p_pixels = nullptr;
    GifPixelType* m_p_gifPixels = nullptr;

    float getCurrentFrameDuration() const;
    bool nextFrame(bool savePixels);
    bool nextRecordType(GifRecordType* gifRecordType);
    bool readExtention(GraphicsControlBlock& gcb);
    bool readFrame(const GraphicsControlBlock& gcb, bool savePixels);
    bool openFile();
    bool fetchInfo();
};