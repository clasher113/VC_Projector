local bit_converter = require("core:bit_converter")
local data_buffer = require("core:data_buffer")
local config = require("projector:config")
local display = require("projector:display")
local rgb_addon = require("projector:rgb_addon")
local util = require("projector:util")

local synchronizer = {
	messages = {},
	on_disconnect_callback = nil
}

local PROTOCOL_MAGIC = 0xAAFFFAA
local MAX_RECEIVE_SIZE = 1024 * 1024 -- bytes
local STATUS_TIMEOUT_DURATION = 5 -- seconds
local STATUS_FALLBACK = util.synchronizer_status.CONNECTED
local server
local client
local refresh_timer = 0.0
local status_update_time = 0
local wait_for_respond = false
local status = util.synchronizer_status.NOT_CONNECTED

function synchronizer.start_server()
	server = network.tcp_open(6969, function (socket)
			if (client == nil) then
				debug.log("user connected")
				client = socket
				status = util.synchronizer_status.CONNECTED
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

function synchronizer.close_server()
	if (server:is_open()) then
		if (client ~= nil and client:is_alive() == true) then
			client:close()
		end
		debug.log("Projector server stopped")	
		server:close()
	end
end

local function send(byte_arr)
	local additional = data_buffer(nil, util.BYTE_ORDER, config.use_bytearray)
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
	local protocol_magic = bit_converter.bytes_to_uint32(data, util.BYTE_ORDER)
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
	local message_size = bit_converter.bytes_to_uint32(data, util.BYTE_ORDER)
	if (message_size == 0 or message_size >= MAX_RECEIVE_SIZE) then
		debug.log("[WARNING]: Invalid message size (" .. tostring(message_size) ..")")
		cleanup_socket()
		return nil
	end

	local out_data = data_buffer(nil, util.BYTE_ORDER, config.use_bytearray)
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

function synchronizer.server_routine()
	local uptime = time.uptime()
	if (uptime < refresh_timer) then return end
	local refresh_interval = 1.0 / config.refresh_rate
	refresh_timer = uptime + refresh_interval

	if (util.status_info[status].timeout and status_update_time + STATUS_TIMEOUT_DURATION < uptime) then
		table.insert(synchronizer.messages, util.status_info[status].string .. " timeout")
		if (status == util.synchronizer_status.INIT) then
			config.rgb_mode = false
		end
		status = STATUS_FALLBACK
	end

	if (client == nil) then
		return
	elseif (client:is_connected() == false) then
		debug.log("client disconnect")
		if (synchronizer.on_disconnect_callback ~= nil) then
			synchronizer.on_disconnect_callback()
		end
		status = util.synchronizer_status.NOT_CONNECTED
		wait_for_respond = false
		client = nil
		return
	end

	while(true) do
		local buffer = receive()

		if (buffer == nil) then
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
		if (bit.band(bit_mask, util.packet_bitmask.SYNC) > 0) then
			local sync_success = buffer:get_bool()
			if (sync_success == false) then
				status = util.synchronizer_status.CONNECTED
				table.insert(synchronizer.messages, "Synchronization error")
			else
				status = util.synchronizer_status.READY
				table.insert(synchronizer.messages, "Synchronization success")
			end
		end
		if (bit.band(bit_mask, util.packet_bitmask.CAPTURE) > 0) then
			local capture_success = buffer:get_bool()
			if (capture_success == false) then
				table.insert(synchronizer.messages, "Capture error")
			elseif (status == util.synchronizer_status.CAPTURING) then
				local pixelsSize = buffer:get_uint32()
				local pixels = buffer:get_bytes(pixelsSize)
				display.update(pixels)
			end
		end
		if (bit.band(bit_mask, util.packet_bitmask.INIT) > 0) then
			local init_success = buffer:get_bool()
			if (init_success) then
				display.rgb_initialized = true
				table.insert(synchronizer.messages, "Initialization success")
				local colors_size = buffer:get_uint32()
				local colors = buffer:get_bytes(colors_size)
				rgb_addon.fetch_textures_color(colors)
			else
				table.insert(synchronizer.messages, "Initialization error")
				config.rgb_mode = false
			end
			status = util.synchronizer_status.CONNECTED
		end
	end

	if (not wait_for_respond) then
		local out_buffer = data_buffer(nil, util.BYTE_ORDER, config.use_bytearray)

		local bit_mask = util.packet_bitmask.PING_PONG
		out_buffer:put_bool(true)
		if (status == util.synchronizer_status.SYNCING) then
			bit_mask = bit.bor(bit_mask, util.packet_bitmask.SYNC)
			out_buffer:put_uint16(config.refresh_rate)
			out_buffer:put_uint16(config.resolution[1])
			out_buffer:put_uint16(config.resolution[2])
			out_buffer:put_uint16(config.capture_size[1])
			out_buffer:put_uint16(config.capture_size[2])
			synchronizer.is_syncing = false
		elseif (status == util.synchronizer_status.CAPTURING) then
			bit_mask = bit.bor(bit_mask, util.packet_bitmask.CAPTURE)
			out_buffer:put_bool(true)
			out_buffer:put_bool(config.rgb_mode)
		elseif (status == util.synchronizer_status.INIT) then
			bit_mask = bit.bor(bit_mask, util.packet_bitmask.INIT)
			local texture_data = rgb_addon.get_textures_data()
			out_buffer:put_uint32(texture_data:size())
			out_buffer:put_bytes(texture_data:get_bytes())
		end
		out_buffer:set_position(1)
		out_buffer:put_uint32(bit_mask)

		send(out_buffer)
		wait_for_respond = true
	end
end

function synchronizer.set_status(new_status)
	status = new_status
	if (util.status_info[status].timeout) then
		status_update_time = time.uptime()
	end
end

function synchronizer.get_status()
	return status
end

return synchronizer