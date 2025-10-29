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

    const uint8_t* const nextFrame();
    void update(float delta);
    void setFrameNum(size_t frame);
    void setController(gui::PlayerController* controller);
    
    size_t getFramesCount() const;
    sf::Vector2u getSize() const;
    bool hasNewFrame() const;

private:
    struct FrameData {
        float m_duration;
        GraphicsControlBlock m_gcb;
        sf::IntRect m_rect;
    };
    float m_animationTimer = 0.f;
    int m_disposal = DISPOSAL_UNSPECIFIED;
    size_t m_currentFrame = 0, m_lastFrame = -1;
    std::vector<FrameData> m_framesData;
    std::map<size_t, sf::Image> m_keyFrames;
    GifFileType* m_p_gifFile = nullptr;
    
    gui::PlayerController* m_p_controller = nullptr;

    uint8_t* m_p_pixels = nullptr;

    float getCurrentFrameDuration() const;
};