local config = require("projector:config")
local rgb_addon = require("projector:rgb_addon")
local util = require("projector:util")

local BLOCK_ID_PREVIOUS = -1

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

function display.update(pixels)
	local uptime = time.uptime()
	if (uptime > framerate_time) then
		framerate_time = uptime + 1
		display.current_framerate = framerate
		framerate = 0
	end
	framerate = framerate + 1

	local step = (config.rgb_mode == true and 2 or 1)
	local start_pos = vec3.add(display.position, config.offset)
	local end_pos = { start_pos[1] + config.resolution[1] - 1, start_pos[2] + config.resolution[2] - 1,  start_pos[3] + config.resolution[1] - 1 }
	local i = 1

	local get_block_func
	if (config.rgb_mode == true) then
		get_block_func = get_rgb_block
	else
		get_block_func = get_mohochrome_block
	end

	if (config.orientation == util.orientation.VERTICAL) then
	
		if (config.axis == util.axis.X) then
			for x=start_pos[1], end_pos[1], 1 do
				for y=start_pos[2], end_pos[2], 1 do 
					local block_id = get_block_func(pixels, i)
					if (block_id ~= BLOCK_ID_PREVIOUS) then
						block.set(x, y, start_pos[3], block_id, 0)
					end
					i = i + step
				end
			end
		elseif (config.axis == util.axis.Z) then
			for z=start_pos[3], end_pos[3], 1 do 
				for y=start_pos[2], end_pos[2], 1 do 
					local block_id = get_block_func(pixels, i)
					if (block_id ~= BLOCK_ID_PREVIOUS) then
						block.set(start_pos[1], y, z, block_id, 0)
					end
					i = i + step
				end
			end
		end
	
	elseif (config.orientation == util.orientation.HORIZONTAL) then
	
		if (config.axis == util.axis.X) then
			end_pos[3] = start_pos[3] + config.resolution[2] - 1
			for x=start_pos[1], end_pos[1], 1 do 
				for z=end_pos[3], start_pos[3], -1 do
					local block_id = get_block_func(pixels, i)
					if (block_id ~= BLOCK_ID_PREVIOUS) then
						block.set(x, start_pos[2], z, block_id, 0)
					end
					i = i + step
				end
			end
		elseif (config.axis == util.axis.Z) then
			end_pos[1] = start_pos[1] + config.resolution[2] - 1
			for z=start_pos[3], end_pos[3], 1 do 
				for x=start_pos[1], end_pos[1], 1 do
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

function display.clear()
	local pixels = {}
	local rgb_enabled = config.rgb_mode
	config.rgb_mode = false
		for i=1,config.resolution[1] * config.resolution[2] do
		pixels[i] = 255
		end
	display.update(pixels)
	config.rgb_mode = rgb_enabled
end

return display