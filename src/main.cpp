#include "Window.hpp"
#include "Enum.hpp"
#include "network/Packet.hpp"
#include "network/Network.hpp"

#include <SFML/System/Sleep.hpp>
#include <SFML/Graphics/Image.hpp>
#include <iostream>

int main() {
	SyncPacketIn syncPacket;

	vcp::Window window(sf::Vector2u(syncPacket.captureSize.x, syncPacket.captureSize.y));
	Network network;

	Status currentStatus = Status::WAITING;
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
					}
					synchonized = syncSuccess;

					network.pushPacket(SyncPacketOut{ syncSuccess });
					currentStatus = Status::SYNCING;
				}
				else if constexpr (std::is_same_v<std::decay_t<decltype(packet)>, CapturePacketIn>) {
					if (packet.capture) {
						currentStatus = Status::CAPTURING;

						sf::Color* pixels = window.capture();

						CapturePacketOut capturePacketOut;
						capturePacketOut.pixels.reserve(syncPacket.projectionSize.x * syncPacket.projectionSize.y * (packet.rgbMode ? 3 : 1));

						const float scale_x = static_cast<float>(syncPacket.captureSize.x) / syncPacket.projectionSize.x;
						const float scale_y = static_cast<float>(syncPacket.captureSize.y) / syncPacket.projectionSize.y;

						for (size_t x = 0; x < syncPacket.projectionSize.x; x++) {
							for (size_t y = 0; y < syncPacket.projectionSize.y; y++) {
								const size_t read_x = static_cast<size_t>(x * scale_x);
								const size_t read_y = static_cast<size_t>(y * scale_y);
								const sf::Color& pixel = pixels[read_y * syncPacket.captureSize.x + read_x];
								if (packet.rgbMode) {
									capturePacketOut.pixels.emplace_back(pixel.b / 16 | pixel.g / 16 << 4);
									capturePacketOut.pixels.emplace_back(pixel.r / 16);
								}
								else capturePacketOut.pixels.emplace_back(static_cast<uint8_t>((0.2126 * (pixel.b / 255.f) + 0.7152 * (pixel.g / 255.f) + 0.0722 * (pixel.r / 255.f)) * 15));
							}
						}

						network.pushPacket(capturePacketOut);
						currentStatus = Status::CAPTURING;
					}
				}
				else if constexpr (std::is_same_v<std::decay_t<decltype(packet)>, InitPacketIn>) {
					InitPacketOut initPacketOut;

					const void* texturesData = packet.texturesData.data();

					while (texturesData < packet.texturesData.data() + packet.texturesData.size()) {

						uint32_t textureSize = unpackData<uint32_t>(texturesData);

						std::vector<uint8_t> texture(textureSize);
						memcpy(texture.data(), texturesData, textureSize);
						texturesData = static_cast<const uint8_t*>(texturesData) + textureSize;

						sf::Image image;
						if (image.loadFromMemory(texture.data(), textureSize) == false) {
							initPacketOut.initStatus = 0;
							break;
						}
						uint64_t color[4] = {};
						for (size_t i = 0; i < image.getSize().x * image.getSize().y * 4; i += 4) {
							for (size_t j = 0; j < 4; j++) {
								color[j] += image.getPixelsPtr()[i + j];
							}
						}
						for (size_t i = 0; i < 4; i++) {
							initPacketOut.texturesColors.emplace_back(static_cast<uint8_t>(color[i] / (image.getSize().x * image.getSize().y)));
						}
					}
					
					network.pushPacket(initPacketOut);
					currentStatus = Status::INITIALIZING;
				}
			}, nextPacket.value());
		}
		window.setStatus(currentStatus);
		network.push();

		window.pollEvents();
		window.draw();
	}
	network.pushPacket(HeaderPacket{ false });
	network.push();

	return 0;
}