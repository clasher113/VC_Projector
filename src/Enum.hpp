#pragma once

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
	CAPTURE = 0x4,
	INIT = 0x8
};