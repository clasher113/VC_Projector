local multiplayer = require("projector:multiplayer")
local rules = require("projector:rules")

local config = {
    resolution = { 180, 120 },
	capture_size = { 180, 120 },
	offset = { 1, 0, 0 },
	refresh_rate = 30,
	orientation = 1,
	axis = 1,
	stop_on_lag_duration = 250,
    multiplayer_buffer_size = 10,
	same_size = true,
	rgb_mode = false,
	clear_on_stop = true,
	use_bytearray = true,
	highlight_area = true,
	use_chunks = true
}

local database = {}
local file_path = nil

local function deserialize(bytes)
    local temp = bjson.frombytes(bytes)
    for k, v in pairs(config) do
        if (type(v) ~= "function") then
            if (temp[k] == nil or type(v) ~= type(temp[k]) or (type(v) == "table" and #v ~= #temp[k])) then
                return nil
            end
        end
    end

    return temp
end

local function serialize(src_table)
    local temp = {}
    for k, v in pairs(src_table) do
        if (type(v) ~= "function") then
            temp[k] = v
        end
    end
    return bjson.tobytes(temp, false)
end

function config.on_world_open()
    if (multiplayer.get_side() == multiplayer.sides.SINGLEPLAYER) then
        file_path = pack.shared_file("projector", "config")

        if (file.isfile(file_path) == false) then
            config.write()
            return
        end
        local temp = deserialize(file.read_bytes(file_path))
        if temp == nil then
            config.write()
        else
            for k, v in pairs(temp) do
                config[k] = v
            end
        end
    elseif (multiplayer.get_side() == multiplayer.sides.SERVER) then
        file_path = pack.data_file("projector", "config")

        if (file.isfile(file_path)) then
            local byte_array = file.read_bytes(file_path, false)
            if (#byte_array > 0) then
                local temp = bjson.frombytes(byte_array)
                for k, v in pairs(temp) do
                    database[tonumber(k)] = v
                end
            end
        end

        local api = multiplayer.get_api()
        api.events.on("projector", "config_request", function(Client, bytes)
            local player_id = Client.player.pid
            local player_config = config.get_player_config(player_id)
            local player_rules = rules.get_rules(player_id)
            player_config.resolution[1] = math.clamp(player_config.resolution[1], player_rules.resolution_min[1], player_rules.resolution_max[1])
            player_config.resolution[2] = math.clamp(player_config.resolution[2], player_rules.resolution_min[2], player_rules.resolution_max[2])
            player_config.offset[1] = math.clamp(player_config.offset[1], player_rules.offset_min[1], player_rules.offset_max[1])
            player_config.offset[2] = math.clamp(player_config.offset[2], player_rules.offset_min[2], player_rules.offset_max[2])
            player_config.offset[3] = math.clamp(player_config.offset[3], player_rules.offset_min[3], player_rules.offset_max[3])
            player_config.refresh_rate = math.clamp(player_config.refresh_rate, 1, player_rules.fps_max)
            if (player_rules.allow_rgb_mode == false) then
                player_rules.rgb_mode = false
            end
            if (#player_rules.allowed_orientations == 1) then
                player_config.orientation = player_rules.allowed_orientations[1]
            end
            if (#player_rules.allowed_axes == 1) then
                player_config.axis = player_rules.allowed_axes[1]
            end
            api.events.tell("projector", "config_request", Client, serialize(player_config))
        end)
        api.events.on("projector", "config_send", function(Client, bytes)
            local player_config = deserialize(bytes)
            if (player_config ~= nil) then
                local player_id = Client.player.pid
                local player_rules = rules.get_rules(player_id)
                if (player_rules.allow_use == false or
                    player_config.resolution[1] > player_rules.resolution_max[1] or
                    player_config.resolution[2] > player_rules.resolution_max[2] or
                    player_config.offset[1] > player_rules.offset_max[1] or
                    player_config.offset[2] > player_rules.offset_max[2] or
                    player_config.offset[3] > player_rules.offset_max[3] or
                    (player_rules.allow_rgb_mode == false and player_config.rgb_mode == true) or
                    player_config.refresh_rate > player_rules.fps_max or
                    (#player_rules.allowed_orientations == 1 and player_config.orientation ~= player_rules.allowed_orientations[1]) or
                    (#player_rules.allowed_axes == 1 and player_config.axis ~= player_rules.allowed_axes[1])
                ) then
                    api.accounts.kick(Client.account, "Out of sync", false)
                end
                database[player_id] = player_config
            end
        end)
    elseif (multiplayer.get_side() == multiplayer.sides.CLIENT) then
        local api = multiplayer.get_api()
        api.events.on("projector", "config_request", function(bytes)
            local temp = deserialize(bytes)
            if temp == nil then
                api.events.send("projector", "config_send", serialize(config))
            else
                for k, v in pairs(temp) do
                    config[k] = v
                end
            end
        end)

        api.events.send("projector", "config_request", {})
    end
end

function config.write()
    if (multiplayer.get_side() == multiplayer.sides.SINGLEPLAYER) then
        file.write_bytes(file_path, serialize(config))
    elseif (multiplayer.get_side() == multiplayer.sides.SERVER) then
        local temp = {}
        for k, v in pairs(database) do
            temp[tostring(k)] = v
        end
        file.write_bytes(file_path, bjson.tobytes(temp, false))
    elseif (multiplayer.get_side() == multiplayer.sides.CLIENT) then
        local api = multiplayer.get_api()
        api.events.send("projector", "config_send", serialize(config))
    end
end

function config.get_player_config(player_id)
    if (multiplayer.get_side() == multiplayer.sides.SERVER) then
        if (database[player_id] == nil) then
            database[player_id] = deserialize(serialize(config))
        end
        return database[player_id]
    else
        if (player_id == hud.get_player()) then
            return config
        else
            return database[player_id]
        end
    end
end

function config.set_player_config(player_config, player_id)
    database[player_id] = player_config
end

return config