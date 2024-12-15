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

DISPLAY.update = function (pixels)

	local x_start = DISPLAY.position_x + CONFIG.offset_x
	local x_end = x_start + CONFIG.resolution_x - 1
	local y_start = DISPLAY.position_y + CONFIG.offset_y
	local y_end = y_start + CONFIG.resolution_y - 1
	local z_start = DISPLAY.position_z + CONFIG.offset_z
	local z_end = z_start + CONFIG.resolution_x - 1

	local i = 1

	if (CONFIG.orientation == 1) then -- vertical

		if (CONFIG.axis == 1) then	-- x-axis
			for x=x_start, x_end, 1 do
				for y=y_start, y_end, 1 do 
					block.set(x, y, z_start, blocks_indices[pixels[i]], 0)
					i = i + 1
				end
			end
		elseif (CONFIG.axis == 2) then -- z-axis
			for z=z_start, z_end, 1 do 
				for y=y_start, y_end, 1 do 
					block.set(x_start, y, z, blocks_indices[pixels[i]], 0)
					i = i + 1
				end
			end
		end

	elseif (CONFIG.orientation == 2) then -- horizontal
	
		if (CONFIG.axis == 1) then	-- x-axis
			z_end = z_start + CONFIG.resolution_y - 1
			for x=x_start, x_end, 1 do 
				for z=z_end, z_start, -1 do 
					block.set(x, y_start, z, blocks_indices[pixels[i]], 0)
					i = i + 1
				end
			end
		elseif (CONFIG.axis == 2) then -- z-axis
			x_end = x_start + CONFIG.resolution_y - 1
			for z=z_start, z_end, 1 do 
				for x=x_start, x_end, 1 do 
					block.set(x, y_start, z, blocks_indices[pixels[i]], 0)
					i = i + 1
				end
			end
		end

	end
end

DISPLAY.clear = function ()
	local pixels = {}
	for i=1,CONFIG.resolution_x * CONFIG.resolution_y do
		pixels[i] = -1
	end
	DISPLAY.update(pixels)
end