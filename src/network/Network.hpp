#pragma once

#include "../Enum.hpp"
#include "Packet.hpp"

#include <variant>
#include <optional>
#include <queue>
#include <SFML/Network/TcpSocket.hpp>

using OutPacketTypes = std::variant<HeaderPacket, SyncPacketOut, CapturePacketOut>;
using InPacketTypes = std::variant<HeaderPacket, SyncPacketIn, CapturePacketIn>;

class Network {
public:
    Network();
    ~Network();

    void update(float deltaTime);
    void pull();
    void push();
    void disconnect();

    bool isConnected() const;
    std::optional<InPacketTypes> getNextPacket();
    void pushPacket(OutPacketTypes outPacket);

private:
    float m_reconnectTimer = 0.f;
    sf::TcpSocket m_socket;

    std::queue<OutPacketTypes> m_outPacketQueue;
    std::queue<InPacketTypes> m_inPacketQueue;

    sf::Socket::Status receive(std::vector<uint8_t>& dst);
    sf::Socket::Status send(const void* const data, uint32_t size);

    void cleanUpSocket();
    void disassemble(const void* data);
    std::vector<uint8_t> assemble();
};