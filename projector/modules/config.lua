local multiplayer = require("projector:multiplayer")

local config = {
	resolution = { 180, 120 },
	capture_size = { 180, 120 },
	offset = { 1, 0, 0 },
	refresh_rate = 30,
	orientation = 1,
	axis = 1,
	stop_on_lag_duration = 150,
	same_size = true,
	rgb_mode = false,
	clear_on_stop = true,
	use_bytearray = true,
	highlight_area = true,
	use_chunks = true
}

local database = nil
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
            for k, v in table(temp) do
                config[k] = v
            end
        end
    elseif (multiplayer.get_side() == multiplayer.sides.SERVER) then
        database = {}
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
            if (database[player_id] == nil) then
                database[player_id] = deserialize(serialize(config))
            end
            api.events.tell("projector", "config_request", Client, serialize(database[player_id]))
        end)
        api.events.on("projector", "config_send", function(Client, bytes)
            local temp = deserialize(bytes)
            if (temp ~= nil) then
                local player_id = Client.player.pid    
                database[player_id] = temp
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

return config