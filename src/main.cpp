#include <iostream>

#include <SFML/Graphics.hpp>

#ifdef _WIN32
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Dwmapi.lib")
#include <dwmapi.h>
#endif // _WIN32

unsigned int framerate = 30;
float updateTimer = 0.f;
float updateInterval = 1.f / framerate;
const uint16_t screenWidth = 180;
const uint16_t screenHeigth = 120;
//const uint8_t chunksNumY = (screenHeigth / CHUNK_D) + 1;
sf::Vector2u displayReadPos(240, 240);
sf::Vector2u displayReadSize(screenWidth, screenHeigth);

constexpr float borderThickness = 3.f;

int main(){

	sf::RenderWindow window(sf::VideoMode(320, 240), "Projector server", sf::Style::None);
	window.setFramerateLimit(framerate);

	MARGINS margins{};
	margins.cxLeftWidth = -1;

	SetWindowLong(window.getSystemHandle(), GWL_STYLE, WS_POPUP | WS_VISIBLE);
	DwmExtendFrameIntoClientArea(window.getSystemHandle(), &margins);

	HWND desktop = GetDesktopWindow();
	HDC desktopHdc = GetDC(desktop);
	HDC hCaptureDC = CreateCompatibleDC(desktopHdc);
	HBITMAP hCaptureBitmap = CreateCompatibleBitmap(desktopHdc, displayReadSize.x, displayReadSize.y);
	auto const oldBmp = SelectObject(hCaptureDC, hCaptureBitmap);
	SelectObject(hCaptureDC, hCaptureBitmap);

	BITMAPINFO bmi{};
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biHeight = displayReadSize.y;
	bmi.bmiHeader.biWidth = displayReadSize.x;
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);

	sf::Color* pixels = new sf::Color[displayReadSize.x * displayReadSize.y];

	sf::RectangleShape border;
	border.setFillColor(sf::Color::Transparent);
	border.setOutlineColor(sf::Color::Yellow);
	border.setOutlineThickness(borderThickness);
	border.setPosition(sf::Vector2f(borderThickness, borderThickness));
	border.setSize(sf::Vector2f(window.getSize().x - borderThickness * 2, window.getSize().y - borderThickness * 2));

	sf::Vector2i grabbedOffset;
	bool grabbedWindow = false;

	sf::Clock clock;

	while (window.isOpen()) {
		const float delta = clock.restart().asSeconds();


		sf::Event e;
		while(window.pollEvent(e)){
			if (e.type == sf::Event::Closed) window.close();
			else if (e.type == sf::Event::MouseButtonPressed) {
				if (e.mouseButton.button == sf::Mouse::Left) {
					grabbedOffset = window.getPosition() - sf::Mouse::getPosition();
					grabbedWindow = true;
				}
			}
			else if (e.type == sf::Event::MouseButtonReleased) {
				if (e.mouseButton.button == sf::Mouse::Left)
					grabbedWindow = false;
			}
			else if (e.type == sf::Event::MouseMoved) {
				if (grabbedWindow)
					window.setPosition(sf::Mouse::getPosition() + grabbedOffset);
			}
			else if (e.type == sf::Event::KeyPressed) {
				if (e.key.code == sf::Keyboard::X) {
					//BitBlt(hCaptureDC, 0, 0, displayReadSize.x, displayReadSize.y, desktopHdc, window.getPosition().x, window.getPosition().y, SRCCOPY);
					//GetDIBits(hCaptureDC, hCaptureBitmap, 0, displayReadSize.y, &pixels[0], &bmi, DIB_RGB_COLORS);

					//sf::Image image;
					//image.create(displayReadSize.x, displayReadSize.y, reinterpret_cast<sf::Uint8*>(pixels));
					//image.saveToFile("test.png");
				}
			}
		}

		window.clear(sf::Color::Transparent);
		window.draw(border);
		window.display();
	}

	delete[] pixels;

	return 0;
}