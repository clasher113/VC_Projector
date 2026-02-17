local config = require("projector:config")
local rgb_addon = require("projector:rgb_addon")
local util = require("projector:util")
local multiplayer = require("projector:multiplayer")
local bit_converter = require("core:bit_converter")

local BLOCK_ID_PREVIOUS = -1
local CHUNK_SIZE = { 16, 16 }

local display = {
	current_framerate = 0,
	rgb_initialized = false
}

local database = {
    --[player_id] = {
    --  position = {}
    --}
}

local blocks_indices = {}
local framerate_time = time.uptime()
local framerate = 0

function display.on_world_open()
	for i=0, 15 do
		blocks_indices[i] = block.index("projector:black" .. tostring(i))
	end
	blocks_indices[255] = block.index("core:air")
	blocks_indices[254] = BLOCK_ID_PREVIOUS
end

local function get_rgb_block(pixels, index)
	return rgb_addon.blocks_indices[bit.bor(pixels[index], bit.lshift(pixels[index + 1], 8))]
end

local function get_mohochrome_block(pixels, index)
	return blocks_indices[pixels[index]]
end

local function update_framerate()
	local uptime = time.uptime()
	if (uptime > framerate_time) then
		framerate_time = uptime + 1
		display.current_framerate = framerate
		framerate = 0
	end
	framerate = framerate + 1
end

function display.update_with_pixels(pixels, player_id)
	update_framerate()

    local player_config = config.get_player_config(player_id)
    if (player_config == nil) then return end
	local step = (player_config.rgb_mode == true and 2 or 1)
	local get_block_func = (player_config.rgb_mode == true and get_rgb_block or get_mohochrome_block)
	local start_pos = vec3.add(database[player_id].position, player_config.offset)
	local i = 1

	if (player_config.orientation == util.orientation.VERTICAL) then
		local end_pos = { start_pos[1] + player_config.resolution[1] - 1, start_pos[2] + player_config.resolution[2] - 1,  start_pos[3] + player_config.resolution[1] - 1 }

		if (player_config.axis == util.axis.X) then
			for x = start_pos[1], end_pos[1] do
				for y = start_pos[2], end_pos[2] do 
					local block_id = get_block_func(pixels, i)
					if (block_id ~= BLOCK_ID_PREVIOUS) then
						block.set(x, y, start_pos[3], block_id, 0, true)
					end
					i = i + step
				end
			end
		elseif (player_config.axis == util.axis.Z) then
			for z = start_pos[3], end_pos[3] do 
				for y = start_pos[2], end_pos[2] do 
					local block_id = get_block_func(pixels, i)
					if (block_id ~= BLOCK_ID_PREVIOUS) then
						block.set(start_pos[1], y, z, block_id, 0, true)
					end
					i = i + step
				end
			end
		end
	
	elseif (player_config.orientation == util.orientation.HORIZONTAL) then
		
		if (player_config.axis == util.axis.X) then
			local end_pos = { start_pos[1] + player_config.resolution[1] - 1, start_pos[3] + player_config.resolution[2] - 1 }
			for x = start_pos[1], end_pos[1] do 
				for z = end_pos[2], start_pos[3], -1 do
					local block_id = get_block_func(pixels, i)
					if (block_id ~= BLOCK_ID_PREVIOUS) then
						block.set(x, start_pos[2], z, block_id, 0, true)
					end
					i = i + step
				end
			end
		elseif (player_config.axis == util.axis.Z) then
			local end_pos = { start_pos[1] + player_config.resolution[2] - 1, start_pos[3] + player_config.resolution[1] - 1 }
			for z = start_pos[3], end_pos[2] do 
				for x = start_pos[1], end_pos[1] do
					local block_id = get_block_func(pixels, i)
					if (block_id ~= BLOCK_ID_PREVIOUS) then
						block.set(x, start_pos[2], z, block_id, 0, true)
					end
					i = i + step
				end
			end
		end
	
	end
end

function display.update_with_chunks(chunks, player_id)
	update_framerate()

    local player_config = config.get_player_config(player_id)
    if (player_config == nil) then return end
    local step = (player_config.rgb_mode == true and 2 or 1)
	local get_block_func = (player_config.rgb_mode == true and get_rgb_block or get_mohochrome_block)
	local start_pos = vec3.add(database[player_id].position, player_config.offset)
	local chunks_count = {
		math.ceil(player_config.resolution[1] / CHUNK_SIZE[1]),
		math.ceil(player_config.resolution[2] / CHUNK_SIZE[2])
	}
	local i = 1

	for cx = 0, chunks_count[1] - 1 do
		for cy = 0, chunks_count[2] - 1 do
			local hasData = bit_converter.byte_to_bool(chunks[i])
			i = i + 1
			if (not hasData) then goto continue end

			local chunk_size = {
				math.min((cx + 1) * CHUNK_SIZE[1], player_config.resolution[1]) % CHUNK_SIZE[1],
				math.min((cy + 1) * CHUNK_SIZE[2], player_config.resolution[2]) % CHUNK_SIZE[2]
			}
			chunk_size[1] = (chunk_size[1] == 0 and CHUNK_SIZE[1] or chunk_size[1]) - 1
			chunk_size[2] = (chunk_size[2] == 0 and CHUNK_SIZE[2] or chunk_size[2]) - 1

			if (player_config.orientation == util.orientation.VERTICAL) then

				local chunk_pos = {
					start_pos[1] + CHUNK_SIZE[1] * cx,
					start_pos[2] + CHUNK_SIZE[2] * cy,
					start_pos[3] + CHUNK_SIZE[1] * cx
				}
				if (player_config.axis == util.axis.X) then
					for x = chunk_pos[1], chunk_pos[1] + chunk_size[1] do
						for y = chunk_pos[2], chunk_pos[2] + chunk_size[2] do 
							local block_id = get_block_func(chunks, i)
							if (block_id ~= BLOCK_ID_PREVIOUS) then
								block.set(x, y, start_pos[3], block_id, 0, true)
							end
							i = i + step
						end
					end
				elseif (player_config.axis == util.axis.Z) then
					for z = chunk_pos[3], chunk_pos[3] + chunk_size[1] do 
						for y = chunk_pos[2], chunk_pos[2] + chunk_size[2] do 
							local block_id = get_block_func(chunks, i)
							if (block_id ~= BLOCK_ID_PREVIOUS) then
								block.set(start_pos[1], y, z, block_id, 0, true)
							end
							i = i + step
						end
					end
				end

			elseif (player_config.orientation == util.orientation.HORIZONTAL) then

				if (player_config.axis == util.axis.X) then
					for x = start_pos[1] + CHUNK_SIZE[1] * cx, start_pos[1] + CHUNK_SIZE[1] * cx + chunk_size[1], 1 do 
						local _cy = chunks_count[2] - cy - 1
						local _chunk_size = math.min(chunks_count[2] * CHUNK_SIZE[2], player_config.resolution[2]) % CHUNK_SIZE[2]
						local offset = cy + 1 == chunks_count[2] and 0 or CHUNK_SIZE[2] - (_chunk_size == 0 and 16 or _chunk_size)
						for z = start_pos[3] + CHUNK_SIZE[2] * _cy + chunk_size[2] - offset, start_pos[3] + CHUNK_SIZE[2] * _cy - offset, -1 do
							local block_id = get_block_func(chunks, i)
							if (block_id ~= BLOCK_ID_PREVIOUS) then
								block.set(x, start_pos[2], z, block_id, 0, true)
							end
							i = i + step
						end
					end
				elseif (player_config.axis == util.axis.Z) then
					for z = start_pos[3] + CHUNK_SIZE[1] * cx, start_pos[3] + CHUNK_SIZE[1] * cx + chunk_size[1], 1 do 
						for x = start_pos[1] + CHUNK_SIZE[2] * cy, start_pos[1] + CHUNK_SIZE[2] * cy + chunk_size[2], 1 do
							local block_id = get_block_func(chunks, i)
							if (block_id ~= BLOCK_ID_PREVIOUS) then
								block.set(x, start_pos[2], z, block_id, 0, true)
							end
							i = i + step
						end
					end
				end
			end
			::continue::
		end
	end
end

function display.clear(player_id)
	local pixels = {}
    local player_config = config.get_player_config(player_id)
	local rgb_enabled = player_config.rgb_mode
	player_config.rgb_mode = false
	for i=1,player_config.resolution[1] * player_config.resolution[2] do
		pixels[i] = 255
	end
	display.update_with_pixels(pixels, player_id)
	if (multiplayer.get_side() == multiplayer.sides.CLIENT) then
		local api = multiplayer.get_api()
        config.write()
		api.events.send("projector", "send_pixels", pixels)
        player_config.rgb_mode = rgb_enabled
        config.write()
	end
	player_config.rgb_mode = rgb_enabled
	framerate = 0
end

function display.set_position(x, y, z, player_id)
    if (database[player_id] == nil) then
        database[player_id] = {}
    end
    database[player_id].position = { x, y, z }
end

function display.get_position(player_id)
    if (database[player_id]) then
        return database[player_id].position
    end
    return nil
end

return display