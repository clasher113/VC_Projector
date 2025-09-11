#pragma once

#include <filesystem>
#include <vector>
#include <gif_lib.h>

namespace sf {
    class Image;
}

class GifPlayer {
public:
    GifPlayer();
    ~GifPlayer();

    bool openFile(const std::filesystem::path& filePath);
    void close();

    const sf::Image* const nextFrame();

    float getCurrentFrameDuration() const;
    int getFramesCount() const;

private:
    struct FrameData {
        float m_duration;
        GraphicsControlBlock m_gcb;
    };
    int m_currentFrame = 0;
    std::vector<FrameData> m_framesData;
    GifFileType* m_p_gifFile = nullptr;
    
    sf::Image* m_p_pixels;
};