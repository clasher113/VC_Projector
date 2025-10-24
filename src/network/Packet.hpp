#pragma once

#include "Util.hpp"

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
};
static_assert(alignof(CapturePacketIn) == 1 && sizeof(CapturePacketIn) == 2);

#pragma pack(pop)

struct CapturePacketOut {
    uint8_t captureStatus = 1;
    std::vector<uint8_t> pixels;
};

struct InitPacketIn {
    InitPacketIn(const void*& data) {
        texturesData.resize(unpackData<uint32_t>(data));
        memcpy(texturesData.data(), data, texturesData.size());
        data = static_cast<const uint8_t*>(data) + texturesData.size();
    };
    std::vector<uint8_t> texturesData;
};

struct InitPacketOut {
    uint8_t initStatus = 1;
    std::vector<uint8_t> texturesColors;
};
