#include <iostream>
#include <cstring>

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#include <dwmapi.h>
#elif __linux__
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#undef Status
#undef None
#endif // _WIN32

#include "font_binary.hpp"

enum class Status {
	WAITING = 1,
	CONNECTED,
	SYNCING,
	READY,
	CAPTURING
};

enum BitMask : uint32_t {
	NONE = 0x0,
	PING_PONG = 0x1,
	SYNC = 0x2,
	CAPTURE = 0x4
};

bool synchonized = false;
uint32_t lastBitmask = BitMask::NONE;
uint16_t framerate = 30;
uint16_t displayResolutionX = 240;
uint16_t displayResolutionY = 120;
uint16_t displayReadSizeX = displayResolutionX;
uint16_t displayReadSizeY = displayResolutionY;

const sf::Vector2u statusContainerSize(200, 30);
const char* REMOTE_ADDRESS = "127.0.0.1";
const unsigned short REMOTE_PORT = 6969;
constexpr uint32_t RECEIVE_MAX_SIZE = 1024 * 1024;
const uint32_t PROTOCOL_MAGIC = 0xAAFFFAA;
const float SYNC_TIMEOUT = 1.f;
const float BORDER_THICKNESS = 3.f;

static void setWindowSize(sf::RenderWindow& window, const sf::Vector2u& size);
static void updateBorder(sf::RectangleShape& border);
static void updateStatusSprite(sf::Sprite& sprite);
static void setStatus(sf::Text& statusText, Status& currentStatus, Status newStatus);
static sf::Socket::Status receiveMessage(sf::TcpSocket& socket, std::vector<uint8_t>& buffer, size_t& received);
static void sendMessage(sf::TcpSocket& socket, const void* data, uint32_t size);

#define REFNSIZE(VALUE) &VALUE, sizeof(VALUE)

static void unpackData(const void* src, void* dst, uint32_t size, uint32_t& offset);
static void packData(std::vector<uint8_t>& dst, const void* src, uint32_t size, uint32_t& offset);

int main() {

#ifdef _WIN32
	sf::RenderWindow window(sf::VideoMode(320, 240), "Projector server", sf::Style::None);
#elif __linux__
	// make x11 window with transparent background
	Display* display = XOpenDisplay(NULL);

	XVisualInfo vinfo;
	XMatchVisualInfo(display, DefaultScreen(display), 32, TrueColor, &vinfo);

	XSetWindowAttributes attr;
	attr.colormap = XCreateColormap(display, DefaultRootWindow(display), vinfo.visual, AllocNone);
	attr.border_pixel = 0;
	attr.background_pixel = 0;

	Window win = XCreateWindow(display, DefaultRootWindow(display), 0, 0, 300, 200, 0, vinfo.depth, InputOutput, vinfo.visual, CWColormap | CWBorderPixel | CWBackPixel, &attr);
	XSelectInput(display, win, StructureNotifyMask);
	GC gc = XCreateGC(display, win, 0, 0);

	Atom wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", 0);

	// disable window decorations
	Atom mwmHintsProperty = XInternAtom(display, "_MOTIF_WM_HINTS", 0);

	struct MwmHints {
		unsigned long flags{};
		unsigned long functions{};
		unsigned long decorations{};
		long input_mode{};
		unsigned long status{};
	};

	struct MwmHints hints;
	hints.flags = (1L << 1);
	hints.decorations = 0;
	XChangeProperty(display, win, mwmHintsProperty, mwmHintsProperty, 32, PropModeReplace, (unsigned char*)&hints, 5);

	XSetWMProtocols(display, win, &wmDeleteWindow, 1);
	XMapWindow(display, win);

	sf::RenderWindow window(win);
#endif // _WIN32
	window.setFramerateLimit(framerate);
	setWindowSize(window, sf::Vector2u(displayReadSizeX, displayReadSizeY));
	sf::Color* pixels = new sf::Color[displayReadSizeX * displayReadSizeY];

#ifdef _WIN32
	MARGINS margins{};
	margins.cxLeftWidth = -1;

	// enable window transparency
	SetWindowLong(window.getSystemHandle(), GWL_STYLE, WS_POPUP | WS_VISIBLE);
	DwmExtendFrameIntoClientArea(window.getSystemHandle(), &margins);
	// make window always on top
	SetWindowPos(window.getSystemHandle(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	HWND desktop = GetDesktopWindow();
	HDC desktopHdc = GetDC(desktop);
	HDC hCaptureDC = CreateCompatibleDC(desktopHdc);
	HBITMAP hCaptureBitmap = CreateCompatibleBitmap(desktopHdc, displayReadSizeX, displayReadSizeY);
	SelectObject(hCaptureDC, hCaptureBitmap);

	BITMAPINFO bmi{};
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biHeight = displayReadSizeY;
	bmi.bmiHeader.biWidth = displayReadSizeX;
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
#elif __linux__
	Window root = DefaultRootWindow(display);

	XWindowAttributes attributes = { 0 };
	XGetWindowAttributes(display, root, &attributes);
#endif // _WIN32

	Status currentStatus = Status::WAITING;

	sf::RectangleShape border;
	border.setFillColor(sf::Color::Transparent);
	border.setOutlineColor(sf::Color::Yellow);
	border.setOutlineThickness(BORDER_THICKNESS);
	border.setPosition(sf::Vector2f(BORDER_THICKNESS, BORDER_THICKNESS));
	updateBorder(border);

	sf::Font font;
	font.loadFromMemory(font_binary::getData(), font_binary::getSize());

	sf::Text statusText("", font, 20U);
	setStatus(statusText, currentStatus, currentStatus);

	sf::RenderTexture statusContainer;
	statusContainer.create(statusContainerSize.x, statusContainerSize.y);

	sf::Sprite statusContainerSprite(statusContainer.getTexture());
	updateStatusSprite(statusContainerSprite);

	sf::TcpSocket socket;

	sf::Vector2i grabbedOffset;
	bool grabbedWindow = false;

	float syncTimer = 0.f;
	sf::Clock clock;

	std::vector<std::vector<uint8_t>> inPackets;
	std::vector<std::vector<uint8_t>> outPackets;

	while (window.isOpen()) {
		const float delta = clock.restart().asSeconds();

		outPackets.clear();
		if (currentStatus != Status::WAITING) {
			static std::vector<uint8_t> inPacket;
			size_t received = 0;
			do {
				if (receiveMessage(socket, inPacket, received) == sf::Socket::Status::Disconnected) {
					setStatus(statusText, currentStatus, Status::WAITING);
					socket.setBlocking(true);
					std::cout << "Server disconnected" << std::endl;
					break;
				}
				if (received) inPackets.push_back(inPacket);
			} while (received);
		}
		for (const auto& packet : inPackets) {	
			static std::vector<uint8_t> outPacket;
			const uint8_t* inBuffer = packet.data();

			uint32_t packBitmask = BitMask::PING_PONG;
			uint32_t unpackBitmask = 0;
			uint32_t packOffset = sizeof(packBitmask);
			uint32_t unpackOffset = 0;

			unpackData(inBuffer, REFNSIZE(unpackBitmask), unpackOffset);

			uint8_t ping = 0;
			unpackData(inBuffer, REFNSIZE(ping), unpackOffset);
			if (ping == 0) {
				std::cout << "Connection reset by server" << std::endl;
				setStatus(statusText, currentStatus, Status::WAITING);
				socket.setBlocking(true);
				break;
			}
			packData(outPacket, REFNSIZE(ping), packOffset);

			if (unpackBitmask & BitMask::SYNC) {
				packBitmask |= BitMask::SYNC;
				unpackData(inBuffer, REFNSIZE(framerate), unpackOffset);
				unpackData(inBuffer, REFNSIZE(displayResolutionX), unpackOffset);
				unpackData(inBuffer, REFNSIZE(displayResolutionY), unpackOffset);
				unpackData(inBuffer, REFNSIZE(displayReadSizeX), unpackOffset);
				unpackData(inBuffer, REFNSIZE(displayReadSizeY), unpackOffset);

				window.setFramerateLimit(framerate);
				setWindowSize(window, sf::Vector2u(displayReadSizeX, displayReadSizeY));
				updateBorder(border);
				updateStatusSprite(statusContainerSprite);
#ifdef _WIN32
				bmi.bmiHeader.biWidth = displayReadSizeX;
				bmi.bmiHeader.biHeight = displayReadSizeY;

				hCaptureBitmap = CreateCompatibleBitmap(desktopHdc, displayReadSizeX, displayReadSizeY);
				SelectObject(hCaptureDC, hCaptureBitmap);
#endif // _WIN32
				delete[] pixels;
				pixels = new sf::Color[displayReadSizeX * displayReadSizeY];

				uint8_t sync = 1;
				packData(outPacket, REFNSIZE(sync), packOffset);
				//justSyncked = true;
				setStatus(statusText, currentStatus, Status::READY);
				synchonized = true;
			}
			if (unpackBitmask & BitMask::CAPTURE) {
				packBitmask |= BitMask::CAPTURE;
				uint8_t capture = 0;
				uint8_t rgbMode = 0;
				unpackData(inBuffer, REFNSIZE(capture), unpackOffset);
				unpackData(inBuffer, REFNSIZE(rgbMode), unpackOffset);

				if (capture && currentStatus == Status::READY) {
					setStatus(statusText, currentStatus, Status::CAPTURING);
				}

				capture = currentStatus == Status::CAPTURING;
				packData(outPacket, REFNSIZE(capture), packOffset);
				if (capture) {
#ifdef _WIN32
					BitBlt(hCaptureDC, 0, 0, displayReadSizeX, displayReadSizeY, desktopHdc, 
						static_cast<int>(window.getPosition().x + BORDER_THICKNESS), static_cast<int>(window.getPosition().y + BORDER_THICKNESS), SRCCOPY);
					GetDIBits(hCaptureDC, hCaptureBitmap, 0, displayReadSizeY, &pixels[0], &bmi, DIB_RGB_COLORS);
#elif __linux__
					XImage* img = XGetImage(display, root, window.getPosition().x + BORDER_THICKNESS, window.getPosition().y + BORDER_THICKNESS, displayReadSizeX, displayReadSizeY, AllPlanes, ZPixmap);
					unsigned int location = (displayReadSizeY - 1) * (displayReadSizeX * 4);
					for (int i = 0; i < displayReadSizeY; ++i) {
						memcpy(&pixels[i * (displayReadSizeX)], &img->data[location], displayReadSizeX * 4);
						location -= displayReadSizeX * 4;
					}

					XDestroyImage(img);
#endif // _WIN32
					std::vector<uint8_t> convertedPixels;
					convertedPixels.reserve(displayResolutionX * displayResolutionY * (rgbMode ? 3 : 1));

					const float scale_x = static_cast<float>(displayReadSizeX) / displayResolutionX;
					const float scale_y = static_cast<float>(displayReadSizeY) / displayResolutionY;

					for (size_t x = 0; x < displayResolutionX; x++) {
						for (size_t y = 0; y < displayResolutionY; y++) {
							const size_t read_x = static_cast<size_t>(x * scale_x);
							const size_t read_y = static_cast<size_t>(y * scale_y);
							const sf::Color& pixel = pixels[read_y * displayReadSizeX + read_x];
							if (rgbMode) {
								convertedPixels.emplace_back(pixel.b / 16 | pixel.g / 16 << 4);
								convertedPixels.emplace_back(pixel.r / 16);
							}
							else convertedPixels.emplace_back(static_cast<uint8_t>((0.2126 * (pixel.b / 255.f) + 0.7152 * (pixel.g / 255.f) + 0.0722 * (pixel.r / 255.f)) * 15));
						}
					}

					const uint32_t pixelsSize = static_cast<uint32_t>(convertedPixels.size());
					packData(outPacket, REFNSIZE(pixelsSize), packOffset);
					packData(outPacket, convertedPixels.data(), pixelsSize, packOffset);
				}
			}
			{
				uint32_t offset = 0;
				packData(outPacket, REFNSIZE(packBitmask), offset);
			}

			sendMessage(socket, outPacket.data(), packOffset);

			if (lastBitmask != unpackBitmask) {
				lastBitmask = unpackBitmask;
				if (synchonized && !(lastBitmask & BitMask::CAPTURE)) {
					setStatus(statusText, currentStatus, Status::READY);
				}
			}
		}
		inPackets.clear();

		if (currentStatus == Status::WAITING) {
			static float connectTimer = 0.f;
			connectTimer += delta;
			if (connectTimer > 1) {
				connectTimer = 0;
				if (socket.connect(REMOTE_ADDRESS, REMOTE_PORT, sf::seconds(0.01f)) == sf::Socket::Status::Done) {
					std::cout << "Connected to the server" << std::endl;
					synchonized = false;
					setStatus(statusText, currentStatus, Status::CONNECTED);
					socket.setBlocking(false);
				}
			}
		}

		if (grabbedWindow) grabbedWindow = sf::Mouse::isButtonPressed(sf::Mouse::Left);
		sf::Event e;
		while (window.pollEvent(e)) {
			if (e.type == sf::Event::Closed)
				window.close();
			else if (e.type == sf::Event::MouseButtonPressed) {
				if (e.mouseButton.button == sf::Mouse::Left) {
					grabbedOffset = window.getPosition() - sf::Mouse::getPosition();
					grabbedWindow = true;
				}
			}
			else if (e.type == sf::Event::MouseButtonReleased) {
				if (e.mouseButton.button == sf::Mouse::Left)
					grabbedWindow = false;
				else if (e.mouseButton.button == sf::Mouse::Right)
					window.close();
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
	if (currentStatus != Status::WAITING) {
		socket.disconnect();
	}

	delete[] pixels;

	return 0;
}

void setWindowSize(sf::RenderWindow& window, const sf::Vector2u& size) {
	const sf::Vector2u newSize(std::max(size.x + static_cast<unsigned int>(BORDER_THICKNESS * 2), statusContainerSize.x),
							   size.y + static_cast<unsigned int>(BORDER_THICKNESS * 2) + statusContainerSize.y);
	window.setSize(newSize);
	window.setView(sf::View(sf::Vector2f(newSize.x / 2.f, newSize.y / 2.f), sf::Vector2f(newSize)));
}

void updateBorder(sf::RectangleShape& border) {
	border.setSize(sf::Vector2f(displayReadSizeX, displayReadSizeY));
}

void updateStatusSprite(sf::Sprite& sprite) {
	sprite.setPosition(sf::Vector2f(0.f, displayReadSizeY + BORDER_THICKNESS * 2));
}

void setStatus(sf::Text& statusText, Status& currentStatus, Status newStatus) {
	currentStatus = newStatus;
	std::string statusStr;
	switch (currentStatus) {
		case Status::WAITING: statusStr = "Waiting for VC"; break;
		case Status::CONNECTED: statusStr = "Connected"; break;
		case Status::SYNCING: statusStr = "Syncing"; break;
		case Status::READY: statusStr = "Ready"; break;
		case Status::CAPTURING: statusStr = "Capturing"; break;
	}
	statusText.setString("Status: " + statusStr);
}

static void cleanUpSocket(sf::TcpSocket& socket) {
	socket.setBlocking(false);
	char buffer[1024];
	size_t received = 0;
	do {
		socket.receive(buffer, sizeof(buffer), received);
	} while (received);

}

sf::Socket::Status receiveMessage(sf::TcpSocket& socket, std::vector<uint8_t>& buffer, size_t& received) {
	sf::Socket::Status status = sf::Socket::Status::Disconnected;
	uint32_t protocolMagic = 0;
	uint32_t messageSize = 0;

	socket.setBlocking(false);
	status = socket.receive(REFNSIZE(protocolMagic), received);
	if (received == 0 || status != sf::Socket::Status::Done) {
		return status;
	}
	else if (protocolMagic != PROTOCOL_MAGIC || received != sizeof(protocolMagic)) {
		std::cout << "[WARNING]: No protocol magic or invalid protocol detected" << std::endl;
		cleanUpSocket(socket);
		return status;
	}

	status = socket.receive(REFNSIZE(messageSize), received);
	if (received != sizeof(messageSize) || status != sf::Socket::Status::Done) {
		std::cout << "[WARNING]: Invalid message format" << std::endl;
		cleanUpSocket(socket);
		return status;
	}
	if (messageSize == 0 || messageSize >= RECEIVE_MAX_SIZE) {
		std::cout << "[WARNING]: Invalid message size" << std::endl;
		cleanUpSocket(socket);
		return status;
	}

	buffer.resize(messageSize);

	size_t offset = 0;
	size_t l_received = received = 0;
	while (messageSize) {
		socket.setBlocking(true);

		status = socket.receive(buffer.data() + offset, messageSize, l_received);
		received += l_received;
		offset += l_received;
		messageSize -= static_cast<uint32_t>(l_received);

		if (l_received == 0) {
			std::cout << "[WARNING]: Read buffer empty" << std::endl;
			return status;
		}

		if (status != sf::Socket::Status::Done) return status;
	}
#ifdef _DEBUG
	std::cout << "received " << received << " bytes" << std::endl;
#endif // _DEBUG
	return status;
}

void sendMessage(sf::TcpSocket& socket, const void* data, uint32_t size) {
	socket.setBlocking(true);

	uint32_t offset = 0;
	std::vector<uint8_t> additional(sizeof(PROTOCOL_MAGIC) + sizeof(size));
	packData(additional, REFNSIZE(PROTOCOL_MAGIC), offset);
	packData(additional, REFNSIZE(size), offset);

	sf::Socket::Status status = socket.send(additional.data(), offset);
	status = socket.send(data, size);

#ifdef _DEBUG
	std::cout << "sent " << size << " bytes" << std::endl;
	std::cout << "send status " << status << std::endl;
#endif // _DEBUG
}

void unpackData(const void* src, void* dst, uint32_t size, uint32_t& offset) {
	memcpy(dst, static_cast<const uint8_t*>(src) + offset, size);
	offset += size;
}

void packData(std::vector<uint8_t>& dst, const void* src, uint32_t size, uint32_t& offset) {
	size_t currentSize = dst.size();
	if (currentSize <= offset + size) {
		dst.resize(currentSize + offset + size);
	}
	memcpy(dst.data() + offset, src, size);
	offset += size;
}

// in packet scheme
// 4 bytes bitmask
// 1 byte ping
// if bitmask has sync bit:
// 2 bytes fps value
// 2 bytes resolution x value
// 2 bytes resolution y value
// 2 bytes read size x value
// 2 bytes read size y value
// if bitmask has capture bit: 
// 1 byte capture require
// 1 byte rgb mode enabled

// out packet scheme
// 4 bytes bitmask
// 1 byte ping
// if bitmask has sync bit:
// 1 byte of sync status
// if bitmask has capture bit: 
// 4 bytes of pixels size
// n bytes of pixels data