local instance_limit = {}

local database = {
    --[player_id] = {
    --  entity_uids = {}
    --}
}
local projector_entity_id = entities.def_index("projector:projector_entity")
local file_path = nil

function instance_limit.set_entity(player_id, new_entity_uid)
    if (database[player_id] == nil) then
        database[player_id] = {
            entity_uids = {}
        }
    else
        for key, entity_uid in pairs(database[player_id].entity_uids) do
            local entity = entities.get(entity_uid)

            if (entity ~= nil and entity:def_index() == projector_entity_id and entity:get_uid() ~= new_entity_uid) then
                entity:despawn()
            end
            database[player_id].entity_uids[key] = nil
        end
    end
    if (not table.has(database[player_id].entity_uids)) then
        table.insert(database[player_id].entity_uids, new_entity_uid)
    end
end

function instance_limit.on_despawned(owner_pid, entity_uid)
    if (database[owner_pid] ~= nil) then
        for key, uid in pairs(database[owner_pid].entity_uids) do
            if (uid == entity_uid) then
                database[owner_pid].entity_uids[key] = nil
            end
        end
        if (#database[owner_pid].entity_uids == 0) then
            database[owner_pid] = nil
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
        if (table.count_pairs(v) > 0) then
            temp[tostring(k)] = v
        end
    end
    file.write_bytes(file_path, bjson.tobytes(temp, false))
end

return instance_limit