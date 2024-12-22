bit_converter = require "core:bit_converter"
data_buffer = require "core:data_buffer"

SYNC = {}
SYNC.is_syncing = false
SYNC.is_capturing = false
SYNC.is_synchronized = false
SYNC.statuses = {}
SYNC.on_disconnect_callback = nil

local refresh_timer = 0.0
local server
local client
local PROTOCOL_MAGIC = 0xAAFFFAA
local MAX_RECEIVE_SIZE = 1024 * 1024
local byte_order = "BE"
local wait_for_respond = false

local BIT_MASK = {}
BIT_MASK.NONE = 0x0
BIT_MASK.PING_PONG = 0x1
BIT_MASK.SYNC = 0x2
BIT_MASK.CAPTURE = 0x4

SYNC.start_server = function()
	server = network.tcp_open(6969, function (socket)
			if (client == nil) then
				debug.log("user connected")
				client = socket
			else
				socket:close()
				debug.log("closed extra connection")
			end
		end
	)
	if (server:is_open()) then
		debug.log("Projector server started")
		return true
	else
		debug.log("Failed to start projector server")
		return false
	end
end

SYNC.close_server = function()
	if (server:is_open()) then
		if (client ~= nil and client:is_alive() == true) then
			client:close()
		end
		debug.log("Projector server stopped")	
		server:close()
	end
end

local function send(byte_arr)
	local additional = data_buffer()
	additional:set_order(byte_order)
	additional:put_uint32(PROTOCOL_MAGIC)
	additional:put_uint32(byte_arr:size())
	client:send(additional:get_bytes())
	client:send(byte_arr:get_bytes())

	--debug.log("sent " .. tostring(byte_arr:size()) .. " bytes")
end

local function cleanup_socket()
	wait_for_respond = false
	while (true) do
		local temp = client:recv(1024, false)
		if (temp == nil or #temp == 0) then
			return
		end
	end
end

local function receive()
	local data = client:recv(4, false)
	if (data == nil or #data == 0) then
		return nil
	end
	local protocol_magic = bit_converter.bytes_to_uint32(data, byte_order)
	if (data == nil or #data == 0 or protocol_magic ~= PROTOCOL_MAGIC) then
		debug.log("[WARNING]: No protocol magic or invalid protocol detected (" .. tostring(protocol_magic) ..")")
		cleanup_socket()
		return nil
	end
	data = client:recv(4, false)
	if (data == nil or #data ~= 4) then
		debug.log("[WARNING]: Invalid message format");
		cleanup_socket()
		return nil
	end
	local message_size = bit_converter.bytes_to_uint32(data, byte_order)
	if (message_size == 0 or message_size >= MAX_RECEIVE_SIZE) then
		debug.log("[WARNING]: Invalid message size (" .. tostring(message_size) ..")")
		cleanup_socket()
		return nil
	end

	local out_data = data_buffer()
	out_data:set_order(byte_order)
	while (message_size > 0) do
		local sub_buffer = client:recv(message_size, false)
		if (sub_buffer == nil) then
			return nil
		elseif (#sub_buffer == 0) then
			debug.log("[WARNING]: Read buffer empty")
			cleanup_socket()
			return nil
		end

		message_size = message_size - #sub_buffer
		out_data:put_bytes(sub_buffer)
	end

	--debug.log("received " .. tostring(out_data:size()) .. " bytes")
	out_data:set_position(1)
	return out_data	
end

local ups = 0
local ups_timer = 0.0

SYNC.server_routine = function()
	refresh_timer = refresh_timer + time.delta()
	--ups_timer = ups_timer + time.delta()
	local refresh_interval = 1.0 / CONFIG.refresh_rate
	if (refresh_timer < refresh_interval) then
		return
	end
	while (refresh_timer > refresh_interval) do
		refresh_timer = refresh_timer - refresh_interval
	end

	--if (ups_timer > 1) then
	--	ups_timer = ups_timer - 1
	--	print(ups)
	--	ups = 0
	--end

	if (client == nil) then
		return
	elseif (client:is_connected() == false) then
		debug.log("client disconnect")
		if (SYNC.on_disconnect_callback ~= nil) then
			SYNC.on_disconnect_callback()
		end
		SYNC.is_syncing = false
		SYNC.is_capturing = false
		SYNC.is_synchronized = false
		wait_for_respond = false
		client = nil
		return
	end

	while(true) do
		local buffer = receive()

		if (buffer == nil or buffer:size() == 0) then
			break
		end

		wait_for_respond = false
		local bit_mask = buffer:get_uint32()

		local ping = buffer:get_bool()
		if (ping == false) then
			client:close()		
			client = nil
			return
		end
		if (bit.band(bit_mask, BIT_MASK.SYNC) > 0) then
			local sync_success = buffer:get_bool()
			if (sync_success == false) then
				SYNC.is_synchronized = false
				table.insert(SYNC.statuses, "Synchronization error")
			else
				SYNC.is_synchronized = true
				table.insert(SYNC.statuses, "Synchronization success")
			end
		end

		if (bit.band(bit_mask, BIT_MASK.CAPTURE) > 0) then
			local capture_success = buffer:get_bool()
			if (capture_success == false) then
				table.insert(SYNC.statuses, "Capture error")
			elseif (SYNC.is_capturing == true) then
				local pixelsSize = buffer:get_uint32()
				local pixels = buffer:get_bytes(pixelsSize)
				DISPLAY.update(pixels)
				--ups = ups + 1
			end
		end
	end

	if (wait_for_respond == false) then
		local out_buffer = data_buffer()
		out_buffer:set_order(byte_order)

		local bit_mask = BIT_MASK.PING_PONG
		out_buffer:put_bool(true)
		if (SYNC.is_syncing == true) then
			bit_mask = bit.bor(bit_mask, BIT_MASK.SYNC)
			out_buffer:put_uint16(CONFIG.refresh_rate)
			out_buffer:put_uint16(CONFIG.resolution_x)
			out_buffer:put_uint16(CONFIG.resolution_y)
			out_buffer:put_uint16(CONFIG.capture_size_x)
			out_buffer:put_uint16(CONFIG.capture_size_y)
			SYNC.is_syncing = false
		end
		if (SYNC.is_capturing == true) then
			bit_mask = bit.bor(bit_mask, BIT_MASK.CAPTURE)
			out_buffer:put_bool(true)
			out_buffer:put_bool(CONFIG.rgb_mode)
		end
		out_buffer:set_position(1)
		out_buffer:put_uint32(bit_mask)

		send(out_buffer)
		wait_for_respond = true
	end
end

SYNC.is_connected = function()
	return client ~= nil
end