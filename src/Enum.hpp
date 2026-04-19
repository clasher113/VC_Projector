#pragma once

#include <cstdint>

enum class Status {
	NONE = 0,
	WAITING,
	CONNECTED,
	SYNCING,
	READY,
	CAPTURING,
	LOADING_GIF
};

enum BitMask : uint32_t {
	NONE = 0x0,
	PING_PONG = 0x1,
	SYNC = 0x2,
	CAPTURE = 0x4
};

enum class Mode {
	SCREEN = 1,
	IMAGE
};

enum UpdateMethod : uint16_t {
	PIXELS = 0,
	CHUNKS
};

template<typename T>
T incrementEnumClass(T& enumClass, uint64_t factor, T max, T fallback){
	enumClass = static_cast<T>(static_cast<int>(enumClass) + factor);
	if (enumClass > max) enumClass = fallback;
	return enumClass;
}