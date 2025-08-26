local synchronizer_status = {
	NOT_CONNECTED = 0,
	CONNECTED = 1,
	SYNCING = 2,
	READY = 3,
	CAPTURING = 4,
	INIT = 5
}

local status_info = {
	[synchronizer_status.NOT_CONNECTED] = { string = "Waiting for connection", gui_enabled = true, timeout = false },
	[synchronizer_status.CONNECTED] = { string = "Connected", gui_enabled = true, timeout = false },
	[synchronizer_status.SYNCING] = { string = "Synchronization", gui_enabled = false, timeout = true },
	[synchronizer_status.READY] = { string = "Ready", gui_enabled = true, timeout = false },
	[synchronizer_status.CAPTURING] = { string = "Capturing", gui_enabled = false, timeout = false },
	[synchronizer_status.INIT] = { string = "Initializing", gui_enabled = false, timeout = true }
}

local packet_bitmask = {
	NONE = 0x0,
	PING_PONG = 0x1,
	SYNC = 0x2,
	CAPTURE = 0x4,
	INIT = 0x8
}

local BYTE_ORDER = "LE"

return {
	synchronizer_status = synchronizer_status,
	status_info = status_info,
	packet_bitmask = packet_bitmask,
	BYTE_ORDER = BYTE_ORDER
}