#pragma once

#include "../Util.hpp"
#include "../Enum.hpp"

#include <cstdint>
#include <cstring>
#include <SFML/System/Vector2.hpp>

#pragma pack(push, 1)

struct HeaderPacket {
    uint8_t ping = 0;
};
static_assert(alignof(HeaderPacket) == 1 && sizeof(HeaderPacket) == 1);

struct SyncPacketIn {
    uint16_t framerate = 30;
    sf::Vector2<uint16_t> projectionSize {240, 120};
    sf::Vector2<uint16_t> captureSize {this->projectionSize};
};
static_assert(alignof(SyncPacketIn) == 1 && sizeof(SyncPacketIn) == 10);

struct SyncPacketOut {
    uint8_t syncStatus = 0;
};
static_assert(alignof(SyncPacketOut) == 1 && sizeof(SyncPacketOut) == 1);

struct CapturePacketIn {
    uint8_t capture = 0;
    uint8_t rgbMode = 0;
    UpdateMethod updateMethod = UpdateMethod::PIXELS;
};
static_assert(alignof(CapturePacketIn) == 1 && sizeof(CapturePacketIn) == 4);

#pragma pack(pop)

struct CapturePacketOut {
    struct Chunk {
        Chunk(const sf::Vector2u& size, bool rgb) {
            data.resize(size.x * size.y * (rgb ? 2 : 1));
        }
        std::vector<uint8_t> data;
    };
    uint8_t captureStatus = 1;
    UpdateMethod updateMethod = UpdateMethod::PIXELS;
    std::vector<uint8_t> pixels;
    std::vector<Chunk> chunks;
};