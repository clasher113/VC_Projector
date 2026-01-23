#include "Network.hpp"

#include "../Util.hpp"

#include <SFML/Network/IpAddress.hpp>
#ifdef _DEBUG
#include <iostream>
#define COUT(EXP) std::cout << EXP << std::endl;
#else
#define COUT(EXP)
#endif // _DEBUG

const sf::IpAddress REMOTE_ADDRESS(127, 0, 0, 1);
constexpr unsigned short REMOTE_PORT = 6969;
constexpr uint32_t PROTOCOL_MAGIC = 0xAAFFFAA;
constexpr float RECONNECT_INTERVAL = 1.f; // seconds
constexpr uint32_t RECEIVE_MAX_SIZE = 1024 * 1024; // bytes

Network::Network() {
}

Network::~Network() {
	disconnect();
}

void Network::update(float deltaTime) {
	if (!isConnected()) {
		m_reconnectTimer += deltaTime;
		if (m_reconnectTimer > RECONNECT_INTERVAL) {
			m_reconnectTimer = 0.f;
			if (m_socket.connect(REMOTE_ADDRESS, REMOTE_PORT, sf::seconds(0.01f)) == sf::Socket::Status::Done) {
				COUT("Connected to the server");
			}
		}
	}
}

void Network::pull() {
	if (!isConnected()) return;
	std::vector<uint8_t> inBuffer;
	if (receive(inBuffer) == sf::Socket::Status::Disconnected) {
		COUT("Server disconnected");
		disconnect();
	}
	if (!inBuffer.empty()) {
		while (!m_inPacketQueue.empty()) m_inPacketQueue.pop();
		disassemble(inBuffer.data());
	}
}

void Network::push() {
	if (!isConnected()) return;
	if (!m_outPacketQueue.empty()) {
		std::vector<uint8_t> data = assemble();
		send(data.data(), static_cast<uint32_t>(data.size()));
	}
}

void Network::disconnect() {
	if (!isConnected()) return;
	m_socket.disconnect();
	m_socket.setBlocking(true);
}

bool Network::isConnected() const {
	return m_socket.getRemotePort() != 0;
}

std::optional<InPacketTypes> Network::getNextPacket() {
	if (m_inPacketQueue.empty()) return std::nullopt;
	InPacketTypes ret = m_inPacketQueue.front();
	m_inPacketQueue.pop();
	return ret;
}

void Network::pushPacket(OutPacketTypes outPacket) {
	m_outPacketQueue.push(outPacket);
}

sf::Socket::Status Network::send(const void* const data, uint32_t size) {
	m_socket.setBlocking(true);

	std::vector<uint8_t> additional;
	packData(additional, REFNSIZE(PROTOCOL_MAGIC));
	packData(additional, REFNSIZE(size));

	sf::Socket::Status status = sf::Socket::Status::Done;
	status = m_socket.send(additional.data(), additional.size());
	if (status != sf::Socket::Status::Done) {
		COUT("send error, status: " << toString(status));
		return status;
	}
	status = m_socket.send(data, size);
	if (status != sf::Socket::Status::Done) {
		COUT("send error, status: " << toString(status));
		return status;
	}
	COUT("sent: " << size << " bytes, send status: " << toString(status));

	return status;
}

sf::Socket::Status Network::receive(std::vector<uint8_t>& dst) {
	sf::Socket::Status status = sf::Socket::Status::Disconnected;
	uint32_t protocolMagic = 0;
	uint32_t messageSize = 0;

	m_socket.setBlocking(false);
	size_t received = 0;
	status = m_socket.receive(REFNSIZE(protocolMagic), received);
	if (received == 0 || status != sf::Socket::Status::Done) {
		return status;
	}
	else if (protocolMagic != PROTOCOL_MAGIC || received != sizeof(protocolMagic)) {
		COUT("[WARNING]: No protocol magic or invalid protocol detected");
		cleanUpSocket();
		return status;
	}

	status = m_socket.receive(REFNSIZE(messageSize), received);
	if (received != sizeof(messageSize) || status != sf::Socket::Status::Done) {
		COUT("[WARNING]: Invalid message format");
		cleanUpSocket();
		return status;
	}
	if (messageSize == 0 || messageSize >= RECEIVE_MAX_SIZE) {
		COUT("[WARNING]: Invalid message size");
		cleanUpSocket();
		return status;
	}

	dst.resize(messageSize);

	size_t offset = 0;
	size_t l_received = received = 0;
	while (messageSize) {
		m_socket.setBlocking(true);

		status = m_socket.receive(dst.data() + offset, messageSize, l_received);
		received += l_received;
		offset += l_received;
		messageSize -= static_cast<uint32_t>(l_received);

		if (l_received == 0) {
			COUT("[WARNING]: Read buffer empty");
			return status;
		}

		if (status != sf::Socket::Status::Done) return status;
	}
	COUT("received " << received << " bytes");

	return status;
}

void Network::cleanUpSocket() {
	m_socket.setBlocking(false);
	char buffer[1024];
	size_t received = 0;
	do {
		m_socket.receive(buffer, sizeof(buffer), received);
	} while (received);
}

void Network::disassemble(const void* data) {
	uint32_t bitMask = unpackData<uint32_t>(data);

	if (bitMask & BitMask::PING_PONG) 
		m_inPacketQueue.emplace(unpackData<HeaderPacket>(data));
	if (bitMask & BitMask::SYNC) 
		m_inPacketQueue.emplace(unpackData<SyncPacketIn>(data));
	if (bitMask & BitMask::CAPTURE)
		m_inPacketQueue.emplace(unpackData<CapturePacketIn>(data));
	if (bitMask & BitMask::INIT)
		m_inPacketQueue.emplace(InitPacketIn(data));
}

std::vector<uint8_t> Network::assemble() {
	uint32_t bitMask = BitMask::NONE;
	std::vector<uint8_t> data(sizeof(bitMask));

	while (!m_outPacketQueue.empty()) {
		std::visit([&](const auto& packet) {
			if constexpr (std::is_same_v<std::decay_t<decltype(packet)>, HeaderPacket>) {
				bitMask |= BitMask::PING_PONG;
				packData(data, REFNSIZE(packet));
			}
			else if constexpr (std::is_same_v<std::decay_t<decltype(packet)>, SyncPacketOut>) {
				bitMask |= BitMask::SYNC;
				packData(data, REFNSIZE(packet));
			}
			else if constexpr (std::is_same_v<std::decay_t<decltype(packet)>, CapturePacketOut>) {
				bitMask |= BitMask::CAPTURE;
				packData(data, REFNSIZE(packet.captureStatus));
				if (packet.captureStatus) {
					packData(data, REFNSIZE(packet.updateMethod));
					if (packet.updateMethod == UpdateMethod::PIXELS) {
						const uint32_t size = static_cast<uint32_t>(packet.pixels.size());
						packData(data, REFNSIZE(size));
						packData(data, packet.pixels.data(), size);
					}
					else if (packet.updateMethod == UpdateMethod::CHUNKS) {
						uint32_t totalSize = static_cast<uint32_t>(packet.chunks.size());
						for (const auto& chunk : packet.chunks) {
							totalSize += chunk.data.size();
						}
						packData(data, REFNSIZE(totalSize));
						for (const auto& chunk : packet.chunks) {
							const uint8_t hasData = !chunk.data.empty();
							packData(data, REFNSIZE(hasData));
							if (!hasData) continue;
							const uint32_t size = static_cast<uint32_t>(chunk.data.size());
							packData(data, chunk.data.data(), size);
						}
					}
				}
			}
			else if constexpr (std::is_same_v<std::decay_t<decltype(packet)>, InitPacketOut>) {
				bitMask |= BitMask::INIT;
				packData(data, REFNSIZE(packet.initStatus));
				if (packet.initStatus) {
					const uint32_t size = static_cast<uint32_t>(packet.texturesColors.size()) * sizeof(*packet.texturesColors.data());
					packData(data, REFNSIZE(size));
					packData(data, packet.texturesColors.data(), size);				
				}
			}
		}, m_outPacketQueue.front());
		m_outPacketQueue.pop();
	}
	memcpy(data.data(), REFNSIZE(bitMask));

	return data;
}