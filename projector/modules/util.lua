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

local orientation = {
	VERTICAL = 1,
	HORIZONTAL = 2
}

local axis = {
	X = 1,
	Z = 2
}

local update_method = {
	PIXELS = 0,
	CHUNKS = 1
}

local BYTE_ORDER = "LE"

local function get_block_entity(x, y, z)
    local entities_arr = entities.get_all_in_box( { x, y, z }, { 1, 1, 1 } )
    if (entities_arr ~= nil) then
        for k,v in pairs(entities_arr) do
            local entity = entities.get(v)
            if (entity:def_name() == "projector:projector_entity") then
                return entity
            end
        end
    end
    return nil
end

local function get_owner_pid(x, y, z)
    local entity = get_block_entity(x, y, z)
    if (entity) then
        local component = entity:get_component("projector:projector")
        if (component) then
            return component:get_owner_pid()
        end
    end
    return nil
end

return {
	synchronizer_status = synchronizer_status,
	status_info = status_info,
	packet_bitmask = packet_bitmask,
	orientation = orientation,
	axis = axis,
	update_method = update_method,
	BYTE_ORDER = BYTE_ORDER,
    get_block_entity = get_block_entity,
    get_owner_pid = get_owner_pid
}