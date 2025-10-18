#pragma once

#include <filesystem>
#include <vector>
#include <map>
#include <gif_lib.h>

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

    const sf::Image* const nextFrame();
    void update(float delta);
    void setFrameNum(size_t frame);
    void setController(gui::PlayerController* controller);
    
    size_t getFramesCount() const;

private:
    struct FrameData {
        float m_duration;
        GraphicsControlBlock m_gcb;
    };
    float m_animationTimer = 0.f;
    size_t m_currentFrame = 0, m_lastFrame = -1;
    std::vector<FrameData> m_framesData;
    std::map<size_t, sf::Image> m_keyFrames;
    GifFileType* m_p_gifFile = nullptr;
    
    gui::PlayerController* m_p_controller = nullptr;

    sf::Image* m_p_pixels;

    float getCurrentFrameDuration() const;
};