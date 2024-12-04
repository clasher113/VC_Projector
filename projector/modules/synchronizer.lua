bit_converter = require "core:bit_converter"
data_buffer = require "core:data_buffer"

SYNC = {}
SYNC.is_syncing = false
SYNC.is_capturing = false
SYNC.is_synchronized = false
SYNC.statuses = {}

local refresh_timer = 0.0
local server
local client
local PROTOCOL_MAGIC = 0xAAFFFAA
local MAX_RECEIVE_SIZE = 1024 * 1024

local BIT_MASK = {}
BIT_MASK.NONE = 0x0
BIT_MASK.PING_PONG = 0x1
BIT_MASK.SYNC = 0x2
BIT_MASK.CAPTURE = 0x4

SYNC.start_server = function()
	server = network.tcp_open(6969, function (socket)
			print("user connected")
			client = socket
		end
	)
	if (server:is_open()) then
		print("Projector server started")
		return true
	else
		print("Failed to start projector server")
		return false
	end
end

SYNC.close_server = function()
	if (server:is_open()) then
		if (client ~= nil and client:is_alive() == true) then
			client:close()
		end
		print("Projector server stopped")	
		server:close()
	end
end

SYNC.server_routine = function()
	
	refresh_timer = refresh_timer + time.delta()
	local refresh_interval = 1.0 / DISPLAY.refresh_rate
	if (refresh_timer < refresh_interval) then
		return
	end
	while (refresh_timer > refresh_interval) do
		refresh_timer = refresh_timer - refresh_interval
	end

	if (client == nil) then
		return
	elseif (client:is_connected() == false) then
		print("client disconnect")
		client = nil
		return
	end

	local packets_size = 0
	while(true) do
		local buffer = SYNC.receive()
		if (buffer == nil) then
			break
		end

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
				SYNC.statuses.insert("Capture error")
			else
				local pixels = buffer:get_bytes(DISPLAY.resolution_x * DISPLAY.resolution_y)
				DISPLAY.update(pixels)
			end
		end
	end

	local out_buffer = data_buffer()
	out_buffer:put_uint32(0) -- reserve 4 bytes for bitmask

	local bit_mask = BIT_MASK.PING_PONG
	out_buffer:put_bool(true)
	if (SYNC.is_syncing == true) then
		bit_mask = bit.bor(bit_mask, BIT_MASK.SYNC)
		out_buffer:put_uint16(DISPLAY.refresh_rate)
		out_buffer:put_uint16(DISPLAY.resolution_x)
		out_buffer:put_uint16(DISPLAY.resolution_y)
		SYNC.is_syncing = false
	end
	if (SYNC.is_capturing == true) then
		bit_mask = bit.bor(bit_mask, BIT_MASK.CAPTURE)
		out_buffer:put_bool(true)
	end
	out_buffer:set_position(1)
	out_buffer:put_uint32(bit_mask)

	SYNC.send(out_buffer)
end

SYNC.send = function(byte_arr)
	local additional = data_buffer()
	additional:put_uint32(PROTOCOL_MAGIC)
	additional:put_uint32(byte_arr:size())
	client:send(additional:get_bytes())
	client:send(byte_arr:get_bytes())

	--print("sent " .. tostring(byte_arr:size()) .. " bytes")
end

SYNC.receive = function()
	local data = client:recv(4, false)
	if (data == nil or #data == 0) then
		return nil
	end
	local protocol_magic = bit_converter.bytes_to_uint32(data)
	if (data == nil or #data == 0 or protocol_magic ~= PROTOCOL_MAGIC) then
		print("[WARNING]: No protocol magic or invalid protocol detected (" .. tostring(protocol_magic) ..")")
		return nil
	end
	data = client:recv(4, false)
	if (data == nil or #data ~= 4) then
		print("[WARNING]: Invalid message format");
		return nil
	end
	local message_size = bit_converter.bytes_to_uint32(data)
	if (message_size == 0 or message_size >= MAX_RECEIVE_SIZE) then
		print("[WARNING]: Invalid message size (" .. tostring(message_size) ..")")
		return nil
	end

	local out_data = data_buffer()
	while (message_size > 0) do
		local sub_buffer = client:recv(message_size, false)
		if (sub_buffer == nil) then
			return nil
		elseif (#sub_buffer == 0) then
			print("[WARNING]: Read buffer empty")
			return nil
		end

		message_size = message_size - #sub_buffer
		out_data:put_bytes(sub_buffer)
	end

	--print("received " .. tostring(out_data:size()) .. " bytes")
	out_data:set_position(1)
	return out_data	
end

SYNC.is_connected = function()
	return client ~= nil
end