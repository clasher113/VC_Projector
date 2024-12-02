#include <iostream>

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

#ifdef _WIN32
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#include <dwmapi.h>
#endif // _WIN32

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

bool justSyncked = false;
uint16_t framerate = 30;
uint16_t displayReadSizeX = 240;
uint16_t displayReadSizeY = 120;
sf::Vector2u displayReadPos(240, 240);

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

static void reverseBytes(void* start, size_t size);
static void unpackData(const void* src, void* dst, uint32_t size, uint32_t& offset);
static void packData(std::vector<uint8_t>& dst, const void* src, uint32_t size, uint32_t& offset);

int main(){

	sf::RenderWindow window(sf::VideoMode(320, 240), "Projector server", sf::Style::None);
	window.setFramerateLimit(framerate);
	setWindowSize(window, sf::Vector2u(displayReadSizeX, displayReadSizeY));

	MARGINS margins{};
	margins.cxLeftWidth = -1;

	SetWindowLong(window.getSystemHandle(), GWL_STYLE, WS_POPUP | WS_VISIBLE);
	DwmExtendFrameIntoClientArea(window.getSystemHandle(), &margins);
	SetWindowPos(window.getSystemHandle(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	HWND desktop = GetDesktopWindow();
	HDC desktopHdc = GetDC(desktop);
	HDC hCaptureDC = CreateCompatibleDC(desktopHdc);
	HBITMAP hCaptureBitmap = CreateCompatibleBitmap(desktopHdc, displayReadSizeX, displayReadSizeY);
	auto const oldBmp = SelectObject(hCaptureDC, hCaptureBitmap);
	SelectObject(hCaptureDC, hCaptureBitmap);

	BITMAPINFO bmi{};
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biHeight = displayReadSizeY;
	bmi.bmiHeader.biWidth = displayReadSizeX;
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);

	sf::Color* pixels = new sf::Color[displayReadSizeX * displayReadSizeY];
	Status currentStatus = Status::WAITING;

	sf::RectangleShape border;
	border.setFillColor(sf::Color::Transparent);
	border.setOutlineColor(sf::Color::Yellow);
	border.setOutlineThickness(BORDER_THICKNESS);
	border.setPosition(sf::Vector2f(BORDER_THICKNESS, BORDER_THICKNESS));
	updateBorder(border);

	sf::Font font;
	font.loadFromFile("C://Windows//Fonts//arial.ttf");

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

		if (justSyncked){
			justSyncked = false;
			setStatus(statusText, currentStatus, Status::READY);
		}

		outPackets.clear();
		if (currentStatus != Status::WAITING){
			static std::vector<uint8_t> inPacket;
			size_t received = 0;
			do {
				if (receiveMessage(socket, inPacket, received) == sf::Socket::Status::Disconnected) {
					setStatus(statusText, currentStatus, Status::WAITING);
					socket.setBlocking(true);
					break;
				}
				if (received) inPackets.push_back(inPacket);
			} while (received);
		}
		for (auto& packet : inPackets) {
			static std::vector<uint8_t> outPacket;
			const uint8_t* inBuffer = packet.data();

			uint32_t packBitmask = 0;
			uint32_t unpackBitmask = 0;
			uint32_t packOffset = sizeof(packBitmask);
			uint32_t unpackOffset = 0;

			unpackData(inBuffer, REFNSIZE(unpackBitmask), unpackOffset);
			reverseBytes(&unpackBitmask, sizeof(unpackBitmask));

			bool ping = false;
			unpackData(inBuffer, REFNSIZE(ping), unpackOffset);
			if (ping == false){
				setStatus(statusText, currentStatus, Status::WAITING);
				socket.setBlocking(true);
				break;
			}
			packData(outPacket, REFNSIZE(ping), packOffset);

			if (ping) {
				if (unpackBitmask & BitMask::SYNC) {
					packBitmask |= BitMask::SYNC;
					unpackData(inBuffer, REFNSIZE(framerate), unpackOffset);
					unpackData(inBuffer, REFNSIZE(displayReadSizeX), unpackOffset);
					unpackData(inBuffer, REFNSIZE(displayReadSizeY), unpackOffset);

					window.setFramerateLimit(framerate);
					setWindowSize(window, sf::Vector2u(displayReadSizeX, displayReadSizeY));
					updateBorder(border);
					updateStatusSprite(statusContainerSprite);

					bmi.bmiHeader.biWidth = displayReadSizeX;
					bmi.bmiHeader.biHeight = displayReadSizeY;

					delete[] pixels;
					pixels = new sf::Color[displayReadSizeX * displayReadSizeY];

					bool sync = true;
					packData(outPacket, REFNSIZE(sync), packOffset);
					setStatus(statusText, currentStatus, Status::SYNCING);
				}
				if (unpackBitmask & BitMask::CAPTURE) {
					bool capture = false;
					unpackData(inBuffer, REFNSIZE(capture), unpackOffset);

					if (capture && currentStatus == Status::READY) {
						setStatus(statusText, currentStatus, Status::CAPTURING);
					}

					capture = currentStatus == Status::CAPTURING;
					packData(outPacket, REFNSIZE(capture), packOffset);
					if (capture) {
						BitBlt(hCaptureDC, 0, 0, displayReadSizeX, displayReadSizeY, desktopHdc, window.getPosition().x + BORDER_THICKNESS, window.getPosition().y + BORDER_THICKNESS, SRCCOPY);
						GetDIBits(hCaptureDC, hCaptureBitmap, 0, displayReadSizeY, &pixels[0], &bmi, DIB_RGB_COLORS);

						std::vector<uint8_t> monochromePixels;
						for (size_t i = 0; i < displayReadSizeX * displayReadSizeY; i++) {
							const sf::Color& rgbPixel = pixels[i];
							monochromePixels.emplace_back(static_cast<uint8_t>((0.2126 * (rgbPixel.b / 255.f) + 0.7152 * (rgbPixel.g / 255.f) + 0.0722 * (rgbPixel.r / 255.f)) * 15));
						}
						packData(outPacket, monochromePixels.data(), monochromePixels.size(), packOffset);
					}
				}
			}
			{
				uint32_t offset = 0;
				packData(outPacket, REFNSIZE(packBitmask), offset);
			}
			
			sendMessage(socket, outPacket.data(), packOffset);
		}
		inPackets.clear();

		if (currentStatus == Status::WAITING){
			static float connectTimer = 0.f;
			connectTimer += delta;
			if (connectTimer > 1) {
				connectTimer = 0;
				if (socket.connect(REMOTE_ADDRESS, REMOTE_PORT, sf::seconds(0.01)) == sf::Socket::Status::Done){
					setStatus(statusText, currentStatus, Status::CONNECTED);
					socket.setBlocking(false);
				}
			}
		}

		if (grabbedWindow) grabbedWindow = sf::Mouse::isButtonPressed(sf::Mouse::Left);
		sf::Event e;
		while(window.pollEvent(e)){
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
	if (currentStatus != Status::WAITING){
		socket.disconnect();
	}

	delete[] pixels;

	return 0;
}

void setWindowSize(sf::RenderWindow& window, const sf::Vector2u& size) {
	const sf::Vector2u newSize(std::max(size.x + static_cast<unsigned int>(BORDER_THICKNESS * 2), statusContainerSize.x),
							   size.y + static_cast<unsigned int>(BORDER_THICKNESS * 2) + statusContainerSize.y);
	window.setSize(newSize);
	window.setView(sf::View(sf::Vector2f(newSize.x / 2, newSize.y / 2), sf::Vector2f(newSize)));
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

static void reverseBytes(void* start, size_t size) {
	uint8_t* istart = static_cast<uint8_t*>(start);
	uint8_t* iend = istart + size;
	std::reverse(istart, iend);
}

sf::Socket::Status receiveMessage(sf::TcpSocket& socket, std::vector<uint8_t>& buffer, size_t& received) {
	sf::Socket::Status status = sf::Socket::Status::Disconnected;
	uint32_t protocolMagic = 0;
	uint32_t messageSize = 0;

	socket.setBlocking(false);
	status = socket.receive(&protocolMagic, sizeof(protocolMagic), received);
	reverseBytes(&protocolMagic, sizeof(protocolMagic));
	if (received == 0 || status != sf::Socket::Status::Done){
		return status;
	}
	else if (protocolMagic != PROTOCOL_MAGIC || received != sizeof(protocolMagic)){
		std::cout << "[WARNING]: No protocol magic or invalid protocol detected" << std::endl;
		cleanUpSocket(socket);
		return status;
	}

	status = socket.receive(&messageSize, sizeof(messageSize), received);
	reverseBytes(&messageSize, sizeof(messageSize));
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
		messageSize -= l_received;

		if (l_received == 0){
			std::cout << "[WARNING]: Read buffer empty" << std::endl;
			return status;
		}

		if (status != sf::Socket::Status::Done) return status;
	}
	std::cout << "received " << received << " bytes" << std::endl;

	return status;
}

void sendMessage(sf::TcpSocket& socket, const void* data, uint32_t size) {
	socket.setBlocking(true);
	
	uint32_t offset = 0;
	std::vector<uint8_t> additional(sizeof(PROTOCOL_MAGIC) + sizeof(size));
	packData(additional, REFNSIZE(PROTOCOL_MAGIC), offset);
	reverseBytes(additional.data(), sizeof(PROTOCOL_MAGIC));

	packData(additional, REFNSIZE(size), offset);
	reverseBytes(additional.data() + sizeof(PROTOCOL_MAGIC), sizeof(size));
	sf::Socket::Status status = socket.send(additional.data(), offset);

	status = socket.send(data, size);

	std::cout << "sended " << size<< " bytes" << std::endl;
	std::cout << "send status " << status << std::endl;
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