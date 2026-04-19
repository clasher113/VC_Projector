#include "Window.hpp"
#include "Enum.hpp"
#include "network/Packet.hpp"
#include "network/Network.hpp"

#include <iostream>
#include <algorithm>
#include <cmath>
#include <SFML/System/Sleep.hpp>
#include <SFML/Graphics/Image.hpp>

constexpr uint8_t PREVIOUS_BLOCK_ID = 254;
constexpr uint8_t BLOCK_UPDATE_REQUIRE_ID = 253;
constexpr uint16_t PREVIOUS_BLOCK_ID_RGB = 65534;
constexpr uint16_t BLOCK_UPDATE_REQUIRE_ID_RGB = 65533;
constexpr uint8_t TRANSPARENT_BLOCK_ID = 255;
constexpr sf::Vector2u CHUNK_SIZE{ 16, 16 };

int main() {
	SyncPacketIn syncPacket;
	std::vector<uint8_t> previousPixels;

	vcp::Window window(sf::Vector2u(syncPacket.captureSize.x, syncPacket.captureSize.y));
	window.setFramerateLimit(syncPacket.framerate);
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
					const float scale_x = static_cast<float>(syncPacket.captureSize.x) / syncPacket.projectionSize.x;
					const float scale_y = static_cast<float>(syncPacket.captureSize.y) / syncPacket.projectionSize.y;

					CapturePacketOut capturePacketOut;
					capturePacketOut.updateMethod = packet.updateMethod;

					if (packet.updateMethod == UpdateMethod::PIXELS) {
						capturePacketOut.pixels.reserve(pixelsSize);

						for (size_t x = 0; x < syncPacket.projectionSize.x; x++) {
							for (size_t y = 0; y < syncPacket.projectionSize.y; y++) {
								const size_t read_x = static_cast<size_t>(x * scale_x);
								const size_t read_y = static_cast<size_t>(y * scale_y);
								const sf::Color& pixel = pixels[read_y * syncPacket.captureSize.x + read_x];
								const bool transparent = pixel.a == 0;

								if (packet.rgbMode) {
									if (transparent) {
										capturePacketOut.pixels.emplace_back(TRANSPARENT_BLOCK_ID);
										capturePacketOut.pixels.emplace_back(TRANSPARENT_BLOCK_ID);
									}
									else {
										capturePacketOut.pixels.emplace_back(pixel.b / 16 | pixel.g / 16 << 4);
										capturePacketOut.pixels.emplace_back(pixel.r / 16);
									}
								}
								else {
									if (transparent) capturePacketOut.pixels.emplace_back(TRANSPARENT_BLOCK_ID);
									else capturePacketOut.pixels.emplace_back(static_cast<uint8_t>((0.2126 * (pixel.b / 255.f) + 0.7152 * (pixel.g / 255.f) + 0.0722 * (pixel.r / 255.f)) * 15));
								}
							}
						}

						if (previousPixels.size() == pixelsSize) {
							for (size_t i = 0; i < pixelsSize; i += (packet.rgbMode ? 2 : 1)) {
								if (packet.rgbMode) {
									const uint16_t currentPixel = reinterpret_cast<uint16_t&>(capturePacketOut.pixels[i]);
									if (reinterpret_cast<uint16_t&>(previousPixels[i]) == currentPixel)
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
					}
					else if (packet.updateMethod == UpdateMethod::CHUNKS) {
						const sf::Vector2u chunksCount(
							std::ceil(static_cast<float>(syncPacket.projectionSize.x) / CHUNK_SIZE.x),
							std::ceil(static_cast<float>(syncPacket.projectionSize.y) / CHUNK_SIZE.y)
						);
						size_t pixelsOffset = 0;

						for (size_t cx = 0; cx < chunksCount.x; cx++) {
							for (size_t cy = 0; cy < chunksCount.y; cy++) {
								sf::Vector2u chunkSize(
									std::min<unsigned int>((cx + 1) * CHUNK_SIZE.x, syncPacket.projectionSize.x) % CHUNK_SIZE.x,
									std::min<unsigned int>((cy + 1) * CHUNK_SIZE.y, syncPacket.projectionSize.y) % CHUNK_SIZE.y
								);
								chunkSize.x = chunkSize.x ? chunkSize.x : CHUNK_SIZE.x;
								chunkSize.y = chunkSize.y ? chunkSize.y : CHUNK_SIZE.y;

								CapturePacketOut::Chunk& chunk = capturePacketOut.chunks.emplace_back(chunkSize, packet.rgbMode);
								for (size_t x = 0; x < chunkSize.x; x++) {
									for (size_t y = 0; y < chunkSize.y; y++) {
										const size_t read_x = static_cast<size_t>((x + CHUNK_SIZE.x * cx) * scale_x);
										const size_t read_y = static_cast<size_t>((y + CHUNK_SIZE.y * cy) * scale_y);
										const sf::Color& pixel = pixels[read_y * syncPacket.captureSize.x + read_x];
										const bool transparent = pixel.a == 0;
										uint8_t* dst = &chunk.data[(x * chunkSize.y + y) * (packet.rgbMode ? 2 : 1)];

										if (packet.rgbMode) {
											if (transparent) {
												dst[0] = TRANSPARENT_BLOCK_ID;
												dst[1] = TRANSPARENT_BLOCK_ID;
											}
											else {
												dst[0] = pixel.b / 16 | pixel.g / 16 << 4;
												dst[1] = pixel.r / 16;
											}
										}
										else {
											if (transparent) dst[0] = TRANSPARENT_BLOCK_ID;
											else dst[0] = (static_cast<uint8_t>((0.2126 * (pixel.b / 255.f) + 0.7152 * (pixel.g / 255.f) + 0.0722 * (pixel.r / 255.f)) * 15));
										}
									}
								}
								if (previousPixels.size() == pixelsSize) {

									bool hasChanges = false;
									for (size_t i = 0; i < chunk.data.size(); i += (packet.rgbMode ? 2 : 1)) {
										if (packet.rgbMode) {
											const uint16_t currentPixel = reinterpret_cast<uint16_t&>(chunk.data[i]);
											if (reinterpret_cast<uint16_t&>(previousPixels[i + pixelsOffset]) == currentPixel)
												reinterpret_cast<uint16_t&>(chunk.data[i]) = PREVIOUS_BLOCK_ID_RGB;
											else hasChanges = true;
											reinterpret_cast<uint16_t&>(previousPixels[i + pixelsOffset]) = currentPixel;
										}
										else {
											const uint8_t currentPixel = chunk.data[i];
											if (previousPixels[i + pixelsOffset] == currentPixel) chunk.data[i] = PREVIOUS_BLOCK_ID;
											else hasChanges = true;
											previousPixels[i + pixelsOffset] = currentPixel;
										}
									}
									pixelsOffset += chunk.data.size();
									if (!hasChanges) chunk.data.clear();
								}
							}
						}
					}

					if (previousPixels.size() != pixelsSize) {
						previousPixels.resize(pixelsSize, BLOCK_UPDATE_REQUIRE_ID);
						if (packet.rgbMode) {
							for (size_t i = 0; i < pixelsSize; i += 2) {
								reinterpret_cast<uint16_t&>(previousPixels[i]) = BLOCK_UPDATE_REQUIRE_ID_RGB;
							}
						}
					}

					network.pushPacket(capturePacketOut);
					currentStatus = Status::CAPTURING;
				}
			}, nextPacket.value());
		}
		if (window.getStatus() != currentStatus){
			if (window.getStatus() == Status::CAPTURING){
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