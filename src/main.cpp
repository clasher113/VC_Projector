#include <iostream>
#include <cstring>
#include <algorithm>

#include <SFML/Network.hpp>
#include <SFML/System/Sleep.hpp>

#include "Window.hpp"
#include "Enum.hpp"

bool synchonized = false;
uint32_t lastBitmask = BitMask::NONE;
uint16_t framerate = 30;
uint16_t displayResolutionX = 240;
uint16_t displayResolutionY = 120;
uint16_t displayReadSizeX = displayResolutionX;
uint16_t displayReadSizeY = displayResolutionY;

const char* REMOTE_ADDRESS = "127.0.0.1";
const unsigned short REMOTE_PORT = 6969;
constexpr uint32_t RECEIVE_MAX_SIZE = 1024 * 1024;
const uint32_t PROTOCOL_MAGIC = 0xAAFFFAA;

#define REFNSIZE(VALUE) &VALUE, sizeof(VALUE)

static sf::Socket::Status receiveMessage(sf::TcpSocket& socket, std::vector<uint8_t>& buffer, size_t& received);
static void sendMessage(sf::TcpSocket& socket, const void* data, uint32_t size);
static void unpackData(const void* src, void* dst, uint32_t size, uint32_t& offset);
static void packData(std::vector<uint8_t>& dst, const void* src, uint32_t size, uint32_t& offset);

int main() {

	vcp::Window window(sf::Vector2u(displayReadSizeX, displayReadSizeY));

	Status currentStatus = Status::WAITING;
	window.setStatus(currentStatus);

	sf::TcpSocket socket;

	float syncTimer = 0.f;
	sf::Clock clock;

	std::vector<std::vector<uint8_t>> inPackets;
	std::vector<std::vector<uint8_t>> outPackets;

	while (window.isOpen()) {
		const float sleepDuration = ((1.f / framerate) - clock.getElapsedTime().asSeconds()) / 2.f;
		if (sleepDuration > 0) sf::sleep(sf::seconds(sleepDuration));
		const float delta = clock.restart().asSeconds();

		window.onUpdate(delta);

		outPackets.clear();
		if (currentStatus != Status::WAITING) {
			static std::vector<uint8_t> inPacket;
			size_t received = 0;
			do {
				if (receiveMessage(socket, inPacket, received) == sf::Socket::Status::Disconnected) {
					window.setStatus(currentStatus = Status::WAITING);
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
				window.setStatus(currentStatus = Status::WAITING);
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

				framerate = std::clamp(framerate, static_cast<uint16_t>(30), static_cast<uint16_t>(60));
				window.setSize(sf::Vector2u(displayReadSizeX, displayReadSizeY));

				uint8_t sync = 1;
				packData(outPacket, REFNSIZE(sync), packOffset);

				window.setStatus(currentStatus = Status::READY);
				synchonized = true;
			}
			if (unpackBitmask & BitMask::CAPTURE) {
				packBitmask |= BitMask::CAPTURE;
				uint8_t capture = 0;
				uint8_t rgbMode = 0;
				unpackData(inBuffer, REFNSIZE(capture), unpackOffset);
				unpackData(inBuffer, REFNSIZE(rgbMode), unpackOffset);

				if (capture && currentStatus == Status::READY) {
					window.setStatus(currentStatus = Status::CAPTURING);
				}

				capture = currentStatus == Status::CAPTURING;
				packData(outPacket, REFNSIZE(capture), packOffset);
				if (capture) {
					sf::Color* pixels = window.capture();

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
			if (unpackBitmask & BitMask::INIT) {
				packBitmask |= BitMask::INIT;

				uint8_t init = 1;

				uint32_t size = 0;
				unpackData(inBuffer, REFNSIZE(size), unpackOffset);

				std::vector<uint8_t> texturesData(size);
				unpackData(inBuffer, texturesData.data(), size, unpackOffset);

				std::vector<uint8_t> colors;

				uint32_t texturesDataOffset = 0;
				while(texturesDataOffset < size){
					uint32_t textureSize = 0;
					unpackData(texturesData.data(), REFNSIZE(textureSize), texturesDataOffset);

					std::vector<uint8_t> texture(textureSize);
					unpackData(texturesData.data(), texture.data(), textureSize, texturesDataOffset);

					sf::Image image;
					if (image.loadFromMemory(texture.data(), textureSize) == false){
						init = 0;
						break;
					}
					uint64_t color[4] = {};
					for (size_t i = 0; i < image.getSize().x * image.getSize().y * 4; i += 4) {
						for (size_t j = 0; j < 4; j++) {
							color[j] += image.getPixelsPtr()[i + j];
						}
					}
					for (size_t i = 0; i < 4; i++) {
						colors.emplace_back(static_cast<uint8_t>(color[i] / (image.getSize().x * image.getSize().y)));
					}
				}

				packData(outPacket, REFNSIZE(init), packOffset);
				if (init){
					uint32_t arrSize = colors.size();
					packData(outPacket, REFNSIZE(arrSize), packOffset);
					packData(outPacket, colors.data(), colors.size(), packOffset);
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
					window.setStatus(currentStatus = Status::READY);
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
					window.setStatus(currentStatus = Status::CONNECTED);
					socket.setBlocking(false);
				}
			}
		}
		
		window.pollEvents();
		window.draw();
	}
	if (currentStatus != Status::WAITING) {
		socket.disconnect();
	}

	return 0;
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
// if bitmask has init bit:
// 4 bytes textures data size
// n bytes textures data

// out packet scheme
// 4 bytes bitmask
// 1 byte ping
// if bitmask has sync bit:
// 1 byte of sync status
// if bitmask has capture bit: 
// 4 bytes of pixels size
// n bytes of pixels data
// if bitmask has init bit:
// 1 byte init status
// 4 bytes textures colors data size
// n bytes textures colors data