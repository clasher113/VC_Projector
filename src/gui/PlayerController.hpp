#pragma once

#include "Container.hpp"

class GifPlayer;

namespace gui {
    class PlayerController;
    class Slider;
}

namespace sf {
    class Texture;
    class Font;
    class Text;
}

class gui::PlayerController : public gui::Container {
public:
    PlayerController(const sf::Font& font, GifPlayer* player, float width);
    ~PlayerController() override;

    void onFileOpen(size_t framesCount);
    void onNewFrame(size_t frameNum);

    bool isPaused() const;

private:
    bool m_isPaused = false, m_wasPaused = false;

    gui::Slider* m_p_slider;

    GifPlayer* m_p_player;
    sf::Texture* m_p_playTexture, * m_p_pauseTexture, * m_p_stopTexture;
    sf::Text* m_p_text;

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    void generateTextures();
    void updateText(size_t currentFrame);
};