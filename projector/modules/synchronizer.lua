local bit_converter = require("core:bit_converter")
local data_buffer = require("core:data_buffer")
local config = require("projector:config")
local display = require("projector:display")
local rgb_addon = require("projector:rgb_addon")
local util = require("projector:util")
local multiplayer = require("projector:multiplayer")
local rules = require("projector:rules")

local synchronizer = {
    messages = {},
    on_disconnect_callback = nil,
    on_lag_callback = nil
}

local PROTOCOL_MAGIC = 0xAAFFFAA
local MAX_RECEIVE_SIZE = 1024 * 1024 -- bytes
local STATUS_TIMEOUT_DURATION = 5 -- seconds
local CONFIG_SIZE = 47
local STATUS_FALLBACK = util.synchronizer_status.CONNECTED
local server
local client
local refresh_timer = 0.0
local status_update_time = 0
local wait_for_respond = false
local multiplayer_frames_count = 0
local status = util.synchronizer_status.NOT_CONNECTED
local capturing_players = {}

local function send_to_players(api, byte_array, owner_pid, event_name)
    local player_config = config.get_player_config(owner_pid)
    local position = display.get_position(owner_pid)
    if (not player_config or not position) then return end

    byte_array:reserve(byte_array.size + CONFIG_SIZE)
    byte_array:append(bit_converter.int64_to_bytes(owner_pid))
    byte_array:append(bit_converter.int64_to_bytes(position[1]))
    byte_array:append(bit_converter.int64_to_bytes(position[2]))
    byte_array:append(bit_converter.int64_to_bytes(position[3]))
    byte_array:append(bit_converter.uint16_to_bytes(player_config.resolution[1]))
    byte_array:append(bit_converter.uint16_to_bytes(player_config.resolution[2]))
    byte_array:append(bit_converter.uint16_to_bytes(player_config.offset[1]))
    byte_array:append(bit_converter.uint16_to_bytes(player_config.offset[2]))
    byte_array:append(bit_converter.uint16_to_bytes(player_config.offset[3]))
    byte_array:append(bit_converter.uint16_to_bytes(player_config.axis))
    byte_array:append(bit_converter.uint16_to_bytes(player_config.orientation))
    byte_array:append( { player_config.rgb_mode } )

    local pos = vec3.add(position, player_config.offset)
	local radius = app.get_setting("chunks.load-distance") * 16 + math.max(player_config.resolution[1], player_config.resolution[2]) / 2

	if (player_config.orientation == util.orientation.VERTICAL) then
		if (player_config.axis == util.axis.X) then
			vec3.add(pos, { player_config.resolution[1] / 2, player_config.resolution[2] / 2, 1.0 }, pos)
		elseif (player_config.axis == util.axis.Z) then
			vec3.add({ 1.0, player_config.resolution[2] / 2, player_config.resolution[1] / 2 }, pos)
		end
	elseif (player_config.orientation == util.orientation.HORIZONTAL) then
		if (player_config.axis == util.axis.X) then
			vec3.add({ player_config.resolution[1] / 2, 1.0, player_config.resolution[2] / 2 }, pos)
		elseif (player_config.axis == util.axis.Z) then
			vec3.add(pos, { player_config.resolution[2] / 2, 1.0, player_config.resolution[1] / 2 }, pos)
		end
	end

    local clients = api.sandbox.players.get_in_radius( pos, radius)

    for _, idt in pairs(clients) do
        if (owner_pid ~= idt.pid) then
            local target_client = api.accounts.by_identity.get_client(idt.identity)
            api.events.tell("projector", event_name, target_client, byte_array)
        end
    end
end

local function unpack_config(byte_array)
    local player_config = data_buffer(byte_array:slice(byte_array.size - CONFIG_SIZE + 1, CONFIG_SIZE), "LE")
    local player_id = player_config:get_int64()
    display.set_position( { player_config:get_int64(), player_config:get_int64(), player_config:get_int64() }, player_id)
    config.set_player_config( {
        resolution = { player_config:get_uint16(), player_config:get_uint16() },
        offset = { player_config:get_uint16(), player_config:get_uint16(), player_config:get_uint16() },
        axis = player_config:get_uint16(),
        orientation = player_config:get_uint16(),
        rgb_mode = player_config:get_bool()
    }, player_id)
    byte_array:remove(byte_array.size - CONFIG_SIZE + 1, CONFIG_SIZE)
    byte_array:trim()
    return player_id
end

function synchronizer.initialize_events()
    local api = multiplayer.get_api()
    if (multiplayer.get_side() == multiplayer.sides.SERVER) then
        api.events.on("projector", "send_pixels", function(Client, byte_array)
            local player_id = Client.player.pid
            local player_rules = rules.get_rules(player_id)
            if (player_rules.allow_use == false) then
                api.accounts.kick(Client.account, "Out of sync", false)
            end
            local pixels = compression.decode(byte_array)
            display.update_with_pixels(pixels, player_id)
            send_to_players(api, byte_array, player_id, "receive_pixels")

            api.events.tell("projector", "next_frame", Client, {})
        end)
        api.events.on("projector", "send_chunks", function(Client, byte_array)
            local player_id = Client.player.pid
            local player_rules = rules.get_rules(player_id)
            if (player_rules.allow_use == false) then
                api.accounts.kick(Client.account, "Out of sync", false)
            end
            local chunks = compression.decode(byte_array)
            display.update_with_chunks(chunks, player_id)
            send_to_players(api, byte_array, player_id, "receive_chunks")

            api.events.tell("projector", "next_frame", Client, {})
        end)
        api.events.on("projector", "capture_status", function (Client, byte_array)
            local player_id = Client.player.pid
            if (capturing_players[player_id] == nil) then
                capturing_players[player_id] = {}
            end
            local capturing = bit_converter.byte_to_bool(byte_array[1])
            if (capturing == true) then
                capturing_players[player_id] = 0
            else
                capturing_players[player_id] = nil
            end

            api.events.echo("projector", "capture_status", bjson.tobytes( { [tostring(player_id)] = capturing } ))
        end)
        events.on("server:on_player_ready", function (Client)
            if (#capturing_players > 0) then
                local capturing = {}
                for k, _ in pairs(capturing_players) do
                    capturing[tostring(k)] = true
                end
                api.events.tell("projector", "capture_status", Client, bjson.tobytes(capturing))
            end
            api.events.tell("projector", "logged_in", Client, bjson.tobytes(rules.get_rules(Client.player.pid)))
        end)
        events.on("server:client_disconnected", function (Client)
            local player_id = Client.player.pid
            if (capturing_players[player_id] ~= nil) then
                capturing_players[player_id] = nil
                api.events.echo("projector", "capture_status", bjson.tobytes( { [tostring(player_id)] = false } ))
            end
        end)
    elseif (multiplayer.get_side() == multiplayer.sides.CLIENT) then
        api.events.on("projector", "next_frame", function()
            if (multiplayer_frames_count > 0) then
                multiplayer_frames_count = multiplayer_frames_count - 1
            end
        end)
        api.events.on("projector", "receive_pixels", function(byte_array)
            local player_id = unpack_config(byte_array)
            display.update_with_pixels(compression.decode(byte_array), player_id)
        end)
        api.events.on("projector", "receive_chunks", function(byte_array)
            local player_id = unpack_config(byte_array)
            display.update_with_chunks(compression.decode(byte_array), player_id)
        end)
        api.events.on("projector", "capture_status", function(byte_array)
            for player_id, capturing in pairs(bjson.frombytes(byte_array)) do
                capturing_players[tonumber(player_id)] = (capturing == true and 0 or nil)
            end
        end)
        api.events.on("projector", "logged_in", function(byte_array)
            multiplayer.logged_in = true
            rules.apply_rules(bjson.frombytes(byte_array))
        end)
    end
end

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
    if (server and server:is_open()) then
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
        local temp = client:recv(1024, config.use_bytearray)
        if (temp == nil or #temp == 0) then
            return
        end
    end
end

local function receive()
    local data = client:recv(4, config.use_bytearray)
    if (data == nil or #data == 0) then
        return nil
    end
    local protocol_magic = bit_converter.bytes_to_uint32(data, util.BYTE_ORDER)
    if (data == nil or #data == 0 or protocol_magic ~= PROTOCOL_MAGIC) then
        debug.log("[WARNING]: No protocol magic or invalid protocol detected (" .. tostring(protocol_magic) ..")")
        cleanup_socket()
        return nil
    end
    data = client:recv(4, config.use_bytearray)
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
        local sub_buffer = client:recv(message_size, config.use_bytearray)
        if (sub_buffer == nil) then
            return nil
        elseif (#sub_buffer == 0) then
            debug.log("[WARNING]: Read buffer empty")
        end

        message_size = message_size - #sub_buffer
        out_data:put_bytes(sub_buffer)
    end

    --debug.log("received " .. tostring(out_data:size()) .. " bytes")
    out_data:set_position(1)
    return out_data
end

function synchronizer.server_routine()
	local refresh_interval = 1.0 / config.refresh_rate
	refresh_timer = refresh_timer + time.delta()
	if (refresh_timer < refresh_interval) then return end
	refresh_timer = math.fmod(refresh_timer - refresh_interval, refresh_interval)

    if (util.status_info[status].timeout and status_update_time + STATUS_TIMEOUT_DURATION < time.uptime()) then
        table.insert(synchronizer.messages, gui.str(util.status_info[status].string) .. " " .. gui.str("timeout", PACK_ID))
        if (status == util.synchronizer_status.INIT) then
            config.rgb_mode = false
        end
        status = STATUS_FALLBACK
    end

    if (status ~= util.synchronizer_status.NOT_CONNECTED and (client == nil or not client:is_connected())) then
        debug.log("client disconnect")
        if (synchronizer.on_disconnect_callback ~= nil) then
            synchronizer.on_disconnect_callback()
        end
        status = util.synchronizer_status.NOT_CONNECTED
        wait_for_respond = false
        client = nil
    end
    if (client == nil) then return end

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
                table.insert(synchronizer.messages, gui.str("Synchronization error", PACK_ID))
            else
                status = util.synchronizer_status.READY
                table.insert(synchronizer.messages, gui.str("Synchronization success", PACK_ID))
            end
        end
        if (bit.band(bit_mask, util.packet_bitmask.CAPTURE) > 0) then
            local capture_success = buffer:get_bool()
            if (capture_success == false) then
                table.insert(synchronizer.messages, gui.str("Capture error", PACK_ID))
            elseif (status == util.synchronizer_status.CAPTURING) then
                if (synchronizer.on_lag_callback ~= nil) then
                    if (synchronizer.on_lag_callback()) then
                        return
                    end
                end
                local update_method = buffer:get_uint16()
                if (update_method == util.update_method.PIXELS) then
                    local pixels_size = buffer:get_uint32()
                    local pixels = buffer:get_bytes(pixels_size)
                    if (multiplayer.get_side() == multiplayer.sides.CLIENT) then
                        local api = multiplayer.get_api()
                        api.events.send("projector", "send_pixels", compression.encode(pixels))
                        multiplayer_frames_count = multiplayer_frames_count + 1
                    end
                    display.update_with_pixels(pixels, hud.get_player())
                elseif (update_method == util.update_method.CHUNKS) then
                    local chunks_size = buffer:get_uint32()
                    local chunks = buffer:get_bytes(chunks_size)
                    if (multiplayer.get_side() == multiplayer.sides.CLIENT) then
                        local api = multiplayer.get_api()
                        api.events.send("projector", "send_chunks", compression.encode(chunks))
                        multiplayer_frames_count = multiplayer_frames_count + 1
                    end
                    display.update_with_chunks(chunks, hud.get_player())
                end
            end
        end
        if (bit.band(bit_mask, util.packet_bitmask.INIT) > 0) then
            local init_success = buffer:get_bool()
            if (init_success) then
                local colors_size = buffer:get_uint32()
                local colors = buffer:get_bytes(colors_size)
                rgb_addon.fetch_textures_color(colors)
                display.rgb_initialized = true
                table.insert(synchronizer.messages, gui.str("Initialization success", PACK_ID))
            else
                table.insert(synchronizer.messages, gui.str("Initialization error", PACK_ID))
                config.rgb_mode = false
            end
            status = util.synchronizer_status.CONNECTED
        end
    end

    if (not wait_for_respond and multiplayer_frames_count < config.multiplayer_buffer_size) then
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
        elseif (status == util.synchronizer_status.CAPTURING) then
            bit_mask = bit.bor(bit_mask, util.packet_bitmask.CAPTURE)
            out_buffer:put_bool(true)
            out_buffer:put_bool(config.rgb_mode)
            out_buffer:put_uint16(config.use_chunks and util.update_method.CHUNKS or util.update_method.PIXELS)
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

function synchronizer.is_player_capturing(player_id)
    if (multiplayer.get_side() == multiplayer.sides.SERVER) then
        return capturing_players[player_id] ~= nil
    else
        if (player_id == hud.get_player()) then
            return status == util.synchronizer_status.CAPTURING
        else
            return capturing_players[player_id] ~= nil
        end
    end
end

return synchronizer