#include "Window.hpp"
#include "Enum.hpp"
#include "network/Packet.hpp"
#include "network/Network.hpp"

#include <iostream>
#include <algorithm>
#include <SFML/System/Sleep.hpp>
#include <SFML/Graphics/Image.hpp>

const uint8_t PREVIOUS_BLOCK_ID = 254;
const uint16_t PREVIOUS_BLOCK_ID_RGB = 65534;

int main() {
	SyncPacketIn syncPacket;
	std::vector<uint8_t> previousPixels;

	vcp::Window window(sf::Vector2u(syncPacket.captureSize.x, syncPacket.captureSize.y));
	window.setFramerateLimit(syncPacket.framerate);
	Network network;

	Status currentStatus = Status::WAITING, lastStatus = Status::NONE;
	bool synchonized = false;

	sf::Clock clock;

	while (window.isOpen()) {
		const float sleepDuration = ((1.f / syncPacket.framerate) - clock.getElapsedTime().asSeconds()) / 2.f;
		if (sleepDuration > 0) sf::sleep(sf::seconds(sleepDuration));
		const float delta = clock.restart().asSeconds();

		window.onUpdate(delta);
		network.update(delta);
		if (!network.isConnected()){
			synchonized = false;
			currentStatus = Status::WAITING;
		}

		network.pull();
		while (auto nextPacket = network.getNextPacket()) {
			if (!network.isConnected()) break;
			std::visit([&](const auto& packet) {
				if constexpr (std::is_same_v<std::decay_t<decltype(packet)>, HeaderPacket>) {
					if (packet.ping == 0) {
						window.setStatus(currentStatus = Status::WAITING);
						std::cout << "Connection reset by server" << std::endl;
						network.disconnect();
						synchonized = false;
						return;
					}

					network.pushPacket(HeaderPacket{ true });
					if (synchonized) currentStatus = Status::READY;
					else currentStatus = Status::CONNECTED;
				}
				else if constexpr (std::is_same_v<std::decay_t<decltype(packet)>, SyncPacketIn>) {
					uint8_t syncSuccess = window.setSize(sf::Vector2u(packet.captureSize.x, packet.captureSize.y));

					if (syncSuccess) {
						syncPacket = packet;
						syncPacket.framerate = std::clamp<decltype(syncPacket.framerate)>(syncPacket.framerate, 30, 60);
						window.setFramerateLimit(syncPacket.framerate);
					}
					synchonized = syncSuccess;

					network.pushPacket(SyncPacketOut{ syncSuccess });
					currentStatus = Status::SYNCING;
				}
				else if constexpr (std::is_same_v<std::decay_t<decltype(packet)>, CapturePacketIn>) {
					if (!packet.capture) return;
					currentStatus = Status::CAPTURING;

					const sf::Color* const pixels = window.capture();
					const size_t pixelsSize = syncPacket.projectionSize.x * syncPacket.projectionSize.y * (packet.rgbMode ? 2 : 1);

					CapturePacketOut capturePacketOut;
					capturePacketOut.pixels.reserve(pixelsSize);

					const float scale_x = static_cast<float>(syncPacket.captureSize.x) / syncPacket.projectionSize.x;
					const float scale_y = static_cast<float>(syncPacket.captureSize.y) / syncPacket.projectionSize.y;

					for (size_t x = 0; x < syncPacket.projectionSize.x; x++) {
						for (size_t y = 0; y < syncPacket.projectionSize.y; y++) {
							const size_t read_x = static_cast<size_t>(x * scale_x);
							const size_t read_y = static_cast<size_t>(y * scale_y);
							const sf::Color& pixel = pixels[read_y * syncPacket.captureSize.x + read_x];
							const bool transparent = pixel.a == 0;
							if (packet.rgbMode) {
								if (transparent) {
									capturePacketOut.pixels.emplace_back(255);
									capturePacketOut.pixels.emplace_back(255);
								}
								else {
									capturePacketOut.pixels.emplace_back(pixel.b / 16 | pixel.g / 16 << 4);
									capturePacketOut.pixels.emplace_back(pixel.r / 16);
								}
							}
							else {
								if (transparent) capturePacketOut.pixels.emplace_back(255);
								else capturePacketOut.pixels.emplace_back(static_cast<uint8_t>((0.2126 * (pixel.b / 255.f) + 0.7152 * (pixel.g / 255.f) + 0.0722 * (pixel.r / 255.f)) * 15));
							}
						}
					}

					if (previousPixels.size() == pixelsSize) {
						for (size_t i = 0; i < pixelsSize; i += (packet.rgbMode ? 2 : 1)) {
							if (packet.rgbMode) {
								const uint16_t currentPixel = static_cast<uint16_t>(capturePacketOut.pixels[i]);
								if (static_cast<uint16_t>(previousPixels[i]) == currentPixel)
									reinterpret_cast<uint16_t&>(capturePacketOut.pixels[i]) = PREVIOUS_BLOCK_ID_RGB;
								reinterpret_cast<uint16_t&>(previousPixels[i]) = currentPixel;
							}
							else {
								const uint8_t currentPixel = capturePacketOut.pixels[i];
								if (previousPixels[i] == currentPixel) capturePacketOut.pixels[i] = PREVIOUS_BLOCK_ID;
								previousPixels[i] = currentPixel;
							}
						}
					}
					else previousPixels.resize(pixelsSize);

					network.pushPacket(capturePacketOut);
					currentStatus = Status::CAPTURING;
				}
				else if constexpr (std::is_same_v<std::decay_t<decltype(packet)>, InitPacketIn>) {
					InitPacketOut initPacketOut;

					const void* texturesData = packet.texturesData.data();

					while (texturesData < packet.texturesData.data() + packet.texturesData.size()) {

						uint32_t textureSize = unpackData<uint32_t>(texturesData);

						sf::Image image;
						if (image.loadFromMemory(texturesData, textureSize) == false) {
							initPacketOut.initStatus = 0;
							break;
						}
						texturesData = static_cast<const uint8_t*>(texturesData) + textureSize;

						uint64_t color[3] = {};
						for (size_t i = 0; i < image.getSize().x * image.getSize().y * 4; i += 4) {
							const uint8_t alpha = image.getPixelsPtr()[i + 3];
							for (size_t j = 0; j < 3; j++) {
								const uint8_t component = image.getPixelsPtr()[i + j];
								color[j] += component + ((255 - component) * (static_cast<float>(255 - alpha) / 255));
							}
						}
						for (size_t i = 0; i < 3; i++) {
							initPacketOut.texturesColors.emplace_back(static_cast<uint8_t>(color[i] / (image.getSize().x * image.getSize().y)));
						}
					}
					
					network.pushPacket(initPacketOut);
					currentStatus = Status::INITIALIZING;
				}
			}, nextPacket.value());
		}
		if (lastStatus != currentStatus){
			lastStatus = currentStatus;
			if (currentStatus != Status::CAPTURING){
				previousPixels.clear();
			}
			window.setStatus(currentStatus);
		}
		network.push();

		window.pollEvents();
		window.draw();
	}
	network.pushPacket(HeaderPacket{ false });
	network.push();

	return 0;
}