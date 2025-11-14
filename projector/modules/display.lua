local config = require("projector:config")
local rgb_addon = require("projector:rgb_addon")
local util = require("projector:util")
local bit_converter = require("core:bit_converter")

local BLOCK_ID_PREVIOUS = -1
local CHUNK_SIZE = { 16, 16 }

local display = {
	position = { 0, 0, 0 },
	current_framerate = 0,
	rgb_initialized = false
}

local blocks_indices = {}
local framerate_time = time.uptime()
local framerate = 0

function display.initialize()
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

function display.update_with_pixels(pixels)
	update_framerate()

	local step = (config.rgb_mode == true and 2 or 1)
	local get_block_func = (config.rgb_mode == true and get_rgb_block or get_mohochrome_block)
	local start_pos = vec3.add(display.position, config.offset)
	local i = 1

	if (config.orientation == util.orientation.VERTICAL) then
		local end_pos = { start_pos[1] + config.resolution[1] - 1, start_pos[2] + config.resolution[2] - 1,  start_pos[3] + config.resolution[1] - 1 }

		if (config.axis == util.axis.X) then
			for x = start_pos[1], end_pos[1] do
				for y = start_pos[2], end_pos[2] do 
					local block_id = get_block_func(pixels, i)
					if (block_id ~= BLOCK_ID_PREVIOUS) then
						block.set(x, y, start_pos[3], block_id, 0)
					end
					i = i + step
				end
			end
		elseif (config.axis == util.axis.Z) then
			for z = start_pos[3], end_pos[3] do 
				for y = start_pos[2], end_pos[2] do 
					local block_id = get_block_func(pixels, i)
					if (block_id ~= BLOCK_ID_PREVIOUS) then
						block.set(start_pos[1], y, z, block_id, 0)
					end
					i = i + step
				end
			end
		end
	
	elseif (config.orientation == util.orientation.HORIZONTAL) then
		local end_pos = { start_pos[1] + config.resolution[2] - 1, start_pos[3] + config.resolution[2] - 1 }

		if (config.axis == util.axis.X) then
			for x = start_pos[1], end_pos[1] do 
				for z = end_pos[2], start_pos[3], -1 do
					local block_id = get_block_func(pixels, i)
					if (block_id ~= BLOCK_ID_PREVIOUS) then
						block.set(x, start_pos[2], z, block_id, 0)
					end
					i = i + step
				end
			end
		elseif (config.axis == util.axis.Z) then
			for z = start_pos[3], end_pos[2] do 
				for x = start_pos[1], end_pos[1] do
					local block_id = get_block_func(pixels, i)
					if (block_id ~= BLOCK_ID_PREVIOUS) then
						block.set(x, start_pos[2], z, block_id, 0)
					end
					i = i + step
				end
			end
		end
	
	end
end

function display.update_with_chunks(chunks)
	update_framerate()

	local step = (config.rgb_mode == true and 2 or 1)
	local get_block_func = (config.rgb_mode == true and get_rgb_block or get_mohochrome_block)
	local start_pos = vec3.add(display.position, config.offset)
	local chunks_count = {
		math.ceil(config.resolution[1] / CHUNK_SIZE[1]),
		math.ceil(config.resolution[2] / CHUNK_SIZE[2])
	}
	local i = 1

	for cx = 0, chunks_count[1] - 1 do
		for cy = 0, chunks_count[2] - 1 do
			local hasData = bit_converter.byte_to_bool(chunks[i])
			i = i + 1
			if (not hasData) then goto continue end

			local chunk_size = {
				math.min((cx + 1) * CHUNK_SIZE[1], config.resolution[1]) % CHUNK_SIZE[1],
				math.min((cy + 1) * CHUNK_SIZE[2], config.resolution[2]) % CHUNK_SIZE[2]
			}
			chunk_size[1] = (chunk_size[1] == 0 and CHUNK_SIZE[1] or chunk_size[1]) - 1
			chunk_size[2] = (chunk_size[2] == 0 and CHUNK_SIZE[2] or chunk_size[2]) - 1

			if (config.orientation == util.orientation.VERTICAL) then

				local chunk_pos = {
					start_pos[1] + CHUNK_SIZE[1] * cx,
					start_pos[2] + CHUNK_SIZE[2] * cy,
					start_pos[3] + CHUNK_SIZE[1] * cx
				}
				if (config.axis == util.axis.X) then
					for x = chunk_pos[1], chunk_pos[1] + chunk_size[1] do
						for y = chunk_pos[2], chunk_pos[2] + chunk_size[2] do 
							local block_id = get_block_func(chunks, i)
							if (block_id ~= BLOCK_ID_PREVIOUS) then
								block.set(x, y, start_pos[3], block_id, 0)
							end
							i = i + step
						end
					end
				elseif (config.axis == util.axis.Z) then
					for z = chunk_pos[3], chunk_pos[3] + chunk_size[1] do 
						for y = chunk_pos[2], chunk_pos[2] + chunk_size[2] do 
							local block_id = get_block_func(chunks, i)
							if (block_id ~= BLOCK_ID_PREVIOUS) then
								block.set(start_pos[1], y, z, block_id, 0)
							end
							i = i + step
						end
					end
				end

			elseif (config.orientation == util.orientation.HORIZONTAL) then

				if (config.axis == util.axis.X) then
					for x = start_pos[1] + CHUNK_SIZE[1] * cx, start_pos[1] + CHUNK_SIZE[1] * cx + chunk_size[1], 1 do 
						local _cy = chunks_count[2] - cy - 1
						local _chunk_size = math.min(chunks_count[2] * CHUNK_SIZE[2], config.resolution[2]) % CHUNK_SIZE[2]
						local offset = cy + 1 == chunks_count[2] and 0 or CHUNK_SIZE[2] - (_chunk_size == 0 and 16 or _chunk_size)
						for z = start_pos[3] + CHUNK_SIZE[2] * _cy + chunk_size[2] - offset, start_pos[3] + CHUNK_SIZE[2] * _cy - offset, -1 do
							local block_id = get_block_func(chunks, i)
							if (block_id ~= BLOCK_ID_PREVIOUS) then
								block.set(x, start_pos[2], z, block_id, 0)
							end
							i = i + step
						end
					end
				elseif (config.axis == util.axis.Z) then
					for z = start_pos[3] + CHUNK_SIZE[1] * cx, start_pos[3] + CHUNK_SIZE[1] * cx + chunk_size[1], 1 do 
						for x = start_pos[1] + CHUNK_SIZE[2] * cy, start_pos[1] + CHUNK_SIZE[2] * cy + chunk_size[2], 1 do
							local block_id = get_block_func(chunks, i)
							if (block_id ~= BLOCK_ID_PREVIOUS) then
								block.set(x, start_pos[2], z, block_id, 0)
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

function display.clear()
	local pixels = {}
	local rgb_enabled = config.rgb_mode
	config.rgb_mode = false
		for i=1,config.resolution[1] * config.resolution[2] do
		pixels[i] = 255
		end
	display.update_with_pixels(pixels)
	config.rgb_mode = rgb_enabled
end

return display