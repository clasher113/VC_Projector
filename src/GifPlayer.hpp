#pragma once

#include <filesystem>
#include <vector>
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
    void setFrameNum(size_t frame);
    void setController(gui::PlayerController* controller);

    float getCurrentFrameDuration() const;
    size_t getFramesCount() const;

private:
    struct FrameData {
        float m_duration;
        GraphicsControlBlock m_gcb;
    };
    size_t m_currentFrame = 0;
    std::vector<FrameData> m_framesData;
    GifFileType* m_p_gifFile = nullptr;
    
    gui::PlayerController* m_p_controller = nullptr;

    sf::Image* m_p_pixels;
};