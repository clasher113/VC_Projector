#include <iostream>

#include <SFML/Graphics.hpp>
#include <filesystem>

#ifdef _WIN32
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Dwmapi.lib")
#include <dwmapi.h>
#include <tlhelp32.h>
#include <psapi.h>
#endif // _WIN32

namespace fs = std::filesystem;

enum class Status {
	WAITING = 1,
	CONNECTED,
	READY,
	CAPTURING
};

unsigned int framerate = 30;
float updateTimer = 0.f;
float updateInterval = 1.f / framerate;
const uint16_t screenWidth = 240;
const uint16_t screenHeigth = 120;
//const uint8_t chunksNumY = (screenHeigth / CHUNK_D) + 1;
sf::Vector2u displayReadPos(240, 240);
sf::Vector2u displayReadSize(screenWidth, screenHeigth);
const sf::Vector2u statusContainerSize(300, 30);

constexpr float borderThickness = 3.f;
HANDLE hProcess = nullptr;
fs::path processDirectory;

static void setWindowSize(sf::RenderWindow& window, const sf::Vector2u& size);
static void updateBorder(sf::RectangleShape& border);
static void updateStatusSprite(sf::Sprite& sprite);
static void setStatus(sf::Text& statusText, Status& currentStatus, Status newStatus);

int main(){

	sf::RenderWindow window(sf::VideoMode(320, 240), "Projector server", sf::Style::None);
	window.setFramerateLimit(framerate);
	setWindowSize(window, displayReadSize);

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

	sf::Color* rgbPixels = new sf::Color[displayReadSize.x * displayReadSize.y];
	uint8_t* monochromePixels = new uint8_t[displayReadSize.x * displayReadSize.y];
	Status currentStatus = Status::WAITING;

	sf::RectangleShape border;
	border.setFillColor(sf::Color::Transparent);
	border.setOutlineColor(sf::Color::Yellow);
	border.setOutlineThickness(borderThickness);
	border.setPosition(sf::Vector2f(borderThickness, borderThickness));
	updateBorder(border);

	sf::Font font;
	font.loadFromFile("C://Windows//Fonts//arial.ttf");

	sf::Text statusText("", font, 20U);
	setStatus(statusText, currentStatus, currentStatus);

	sf::RenderTexture statusContainer;
	statusContainer.create(statusContainerSize.x, statusContainerSize.y);

	sf::Sprite statusContainerSprite(statusContainer.getTexture());
	updateStatusSprite(statusContainerSprite);

	sf::Vector2i grabbedOffset;
	bool grabbedWindow = false;

	sf::Clock clock;

	while (window.isOpen()) {
		const float delta = clock.restart().asSeconds();

		if (currentStatus == Status::WAITING){
			static float searchProcessTimer = 0.f;
			searchProcessTimer += delta;
			if (searchProcessTimer > 1) {
				searchProcessTimer = 0;

				PROCESSENTRY32 entry;
				entry.dwSize = sizeof(PROCESSENTRY32);

				HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

				if (Process32First(snapshot, &entry) == TRUE) {
					while (Process32Next(snapshot, &entry) == TRUE) {
						if (stricmp(entry.szExeFile, "VoxelEngine-Cpp.exe") == 0) {
							hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
							WCHAR path[MAX_PATH]{};
							GetModuleFileNameExW(hProcess, NULL, path, MAX_PATH);
							processDirectory = fs::path(path).parent_path();
							setStatus(statusText, currentStatus, Status::CONNECTED);
						}
					}
				}

				CloseHandle(snapshot);
			}
		}
		else if (currentStatus == Status::CONNECTED) {
		}
		else if (currentStatus == Status::READY){
		}
		else if (currentStatus == Status::CAPTURING){
			BitBlt(hCaptureDC, 0, 0, displayReadSize.x, displayReadSize.y, desktopHdc, window.getPosition().x, window.getPosition().y, SRCCOPY);
			GetDIBits(hCaptureDC, hCaptureBitmap, 0, displayReadSize.y, &rgbPixels[0], &bmi, DIB_RGB_COLORS);

			for (size_t i = 0; i < displayReadSize.x * displayReadSize.y; i++) {
				const sf::Color& rgbPixel = rgbPixels[i];
				monochromePixels[i] = 0.2126 * (rgbPixel.r / 255.f) + 0.7152 * (rgbPixel.g / 255.f) + 0.0722 * (rgbPixel.b / 255.f);
			}
		}

		if (currentStatus != Status::WAITING){
			DWORD ret = 0;
			if (hProcess) GetExitCodeProcess(hProcess, &ret);
			if (ret != STILL_ACTIVE) {
				CloseHandle(hProcess);
				setStatus(statusText, currentStatus, Status::WAITING);
			}
		}

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
		}

		statusContainer.clear(sf::Color::Black);
		statusContainer.draw(statusText);
		statusContainer.display();

		window.clear(sf::Color::Transparent);
		window.draw(border);
		window.draw(statusContainerSprite);
		window.display();
	}

	delete[] rgbPixels;
	delete[] monochromePixels;

	return 0;
}

void setWindowSize(sf::RenderWindow& window, const sf::Vector2u& size) {
	const sf::Vector2u newSize(std::max(size.x + static_cast<unsigned int>(borderThickness * 2), statusContainerSize.x),
							   size.y + static_cast<unsigned int>(borderThickness * 2) + statusContainerSize.y);
	window.setSize(newSize);
	window.setView(sf::View(sf::Vector2f(newSize.x / 2, newSize.y / 2), sf::Vector2f(newSize)));
}

void updateBorder(sf::RectangleShape& border) {
	border.setSize(sf::Vector2f(displayReadSize.x - borderThickness * 2, displayReadSize.y - borderThickness * 2 ));
}

void updateStatusSprite(sf::Sprite& sprite) {
	sprite.setPosition(sf::Vector2f(0.f, displayReadSize.y));
}

void setStatus(sf::Text& statusText, Status& currentStatus, Status newStatus) {
	currentStatus = newStatus;
	std::string statusStr;
	switch (currentStatus) {
		case Status::WAITING: statusStr = "Waiting for VC"; break;
		case Status::CONNECTED: statusStr = "Connected"; break;
		case Status::READY: statusStr = "Ready"; break;
		case Status::CAPTURING: statusStr = "Capturing"; break;
	}
	statusText.setString("Status: " + statusStr);
}