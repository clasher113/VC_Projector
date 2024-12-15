DISPLAY = {}
DISPLAY.position_x = 0
DISPLAY.position_y = 0
DISPLAY.position_z = 0

local blocks_indices = {}

DISPLAY.initialize = function ()
	for i=0, 15 do
		blocks_indices[i] = block.index("projector:black" .. tostring(i))
	end
	blocks_indices[-1] = block.index("core:air")
end

local function get_rgb_block(pixels, index)
	return RGB.blocks_indices[bit.bor(pixels[index], bit.lshift(pixels[index + 1], 8))]
end

local function get_mohochrome_block(pixels, index)
	return blocks_indices[pixels[index]]
end

DISPLAY.update = function (pixels)

	local step = (CONFIG.rgb_mode == true and 2 or 1)
	local x_start = DISPLAY.position_x + CONFIG.offset_x
	local x_end = x_start + CONFIG.resolution_x - 1
	local y_start = DISPLAY.position_y + CONFIG.offset_y
	local y_end = y_start + CONFIG.resolution_y - 1
	local z_start = DISPLAY.position_z + CONFIG.offset_z
	local z_end = z_start + CONFIG.resolution_x - 1

	local i = 1

	local get_block_func
	if (CONFIG.rgb_mode == true) then
		get_block_func = get_rgb_block
	else
		get_block_func = get_mohochrome_block
	end

	if (CONFIG.orientation == 1) then -- vertical

		if (CONFIG.axis == 1) then	-- x-axis
			for x=x_start, x_end, 1 do
				for y=y_start, y_end, 1 do 
					block.set(x, y, z_start, get_block_func(pixels, i), 0)
					i = i + step
				end
			end
		elseif (CONFIG.axis == 2) then -- z-axis
			for z=z_start, z_end, 1 do 
				for y=y_start, y_end, 1 do 
					block.set(x_start, y, z, get_block_func(pixels, i), 0)
					i = i + step
				end
			end
		end

	elseif (CONFIG.orientation == 2) then -- horizontal
	
		if (CONFIG.axis == 1) then	-- x-axis
			z_end = z_start + CONFIG.resolution_y - 1
			for x=x_start, x_end, 1 do 
				for z=z_end, z_start, -1 do
					block.set(x, y_start, z, get_block_func(pixels, i), 0)
					i = i + step
				end
			end
		elseif (CONFIG.axis == 2) then -- z-axis
			x_end = x_start + CONFIG.resolution_y - 1
			for z=z_start, z_end, 1 do 
				for x=x_start, x_end, 1 do
					block.set(x, y_start, z, get_block_func(pixels, i), 0)
					i = i + step
				end
			end
		end

	end
end

DISPLAY.clear = function ()
	local pixels = {}
	if (CONFIG.rgb_mode == true) then
		for i=1,CONFIG.resolution_x * CONFIG.resolution_y * 2, 2 do
			pixels[i] = 0
			pixels[i + 1] = -1
		end
	else
		for i=1,CONFIG.resolution_x * CONFIG.resolution_y do
			pixels[i] = -1
		end
	end
	DISPLAY.update(pixels)
end