local util = require("projector:util")

local instance_limit = {}

local database = {
    --[player_id] = {
    --  positions = {}
    --}
}
local projector_block_index = block.index("projector:projector")
local file_path = nil

local function vec3_equal(v1, v2)
    return v1[1] == v2[1] and v1[2] == v2[2] and v1[3] == v2[3]
end

local function is_exists(array, position)
    for _, v in pairs(array) do
        if (vec3_equal(v, position)) then
            return true
        end
    end
    return false
end

function instance_limit.set_position(player_id, new_position)
    instance_limit.cleanup(player_id, new_position)
    if (database[player_id] == nil) then
        database[player_id] = {
            positions = {}
        }
    end
    if (not is_exists(database[player_id].positions, new_position)) then
        table.insert(database[player_id].positions, new_position)
    end
end

function instance_limit.cleanup(player_id, ignore_pos)
	if (database[player_id] ~= nil) then
        for key, position in pairs(database[player_id].positions) do
            local block_id = block.get(position[1], position[2], position[3])
            if (block_id == projector_block_index) then
                local owner_pid = util.get_owner_pid(position[1], position[2], position[3])
                if (owner_pid == player_id and (ignore_pos == nil or not vec3_equal(position, ignore_pos))) then
                    block.destruct(position[1], position[2], position[3], player_id)
                end
            elseif (block_id ~= -1) then
                database[player_id].positions[key] = nil
            end
        end
	end
    if (database[player_id] ~= nil and #database[player_id].positions == 0) then
        database[player_id] = nil
    end
end

function instance_limit.on_broken(x, y, z, player_id)
    if (database[player_id] ~= nil) then
        for key, position in pairs(database[player_id].positions) do
            if (position[1] == x and position[2] == y and position[3] == z) then
                database[player_id].positions[key] = nil
            end
        end
        if (#database[player_id].positions == 0) then
            database[player_id] = nil
        end
    end
end

function instance_limit.on_world_open()
    file_path = pack.data_file("projector", "instance_limit")
    
	if (file.isfile(file_path)) then
        local byte_array = file.read_bytes(file_path, false)
        if (#byte_array > 0) then
            local temp = bjson.frombytes(byte_array)
            for k, v in pairs(temp) do
                database[tonumber(k)] = v
            end
        end
	end
end

function instance_limit.save()
    local temp = {}
    for k, v in pairs(database) do
        temp[tostring(k)] = v
    end
    file.write_bytes(file_path, bjson.tobytes(temp, false))
end

return instance_limit