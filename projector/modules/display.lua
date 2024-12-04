require "projector:synchronizer"

DISPLAY = {}
DISPLAY.resolution_x = 0
DISPLAY.resolution_y = 0
DISPLAY.offset_x = 0
DISPLAY.offset_y = 0
DISPLAY.offset_z = 0
DISPLAY.refresh_rate = 0
DISPLAY.position_x = 0
DISPLAY.position_y = 0
DISPLAY.position_z = 0
DISPLAY.orientation = 0
DISPLAY.axis = 0
DISPLAY.orientation = 0

local blocks_indices = {}

DISPLAY.init = function ()
	for i=0, 15 do
		blocks_indices[i] = block.index("projector:black" .. tostring(i))
	end
	blocks_indices[16] = block.index("core:air")
end

DISPLAY.update = function (pixels)

	local x_start = DISPLAY.position_x + DISPLAY.offset_x
	local x_end = x_start + DISPLAY.resolution_x - 1
	local y_start = DISPLAY.position_y + DISPLAY.offset_y
	local y_end = y_start + DISPLAY.resolution_y - 1
	local z_start = DISPLAY.position_z + DISPLAY.offset_z
	local z_end = z_start + DISPLAY.resolution_x - 1

	local i = 1

	if (DISPLAY.orientation == 1) then -- vertical

		if (DISPLAY.axis == 1) then	-- x-axis
			for y=y_start, y_end, 1 do 
				for x=x_start, x_end, 1 do 
					block.set(x, y, z_start, blocks_indices[pixels[i]], 0)
					i = i + 1
				end
			end
		elseif (DISPLAY.axis == 2) then -- z-axis
			for y=y_start, y_end, 1 do 
				for z=z_start, z_end, 1 do 
					block.set(x_start, y, z, blocks_indices[pixels[i]], 0)
					i = i + 1
				end
			end
		end

	elseif (DISPLAY.orientation == 2) then -- horizontal
	
		if (DISPLAY.axis == 1) then	-- x-axis
			z_end = z_start + DISPLAY.resolution_y - 1
			for z=z_end, z_start, -1 do 
				for x=x_start, x_end, 1 do 
					block.set(x, y_start, z, blocks_indices[pixels[i]], 0)
					i = i + 1
				end
			end
		elseif (DISPLAY.axis == 2) then -- z-axis
			x_end = x_start + DISPLAY.resolution_y - 1
			for x=x_start, x_end, 1 do 
				for z=z_start, z_end, 1 do 
					block.set(x, y_start, z, blocks_indices[pixels[i]], 0)
					i = i + 1
				end
			end
		end

	end
end

DISPLAY.clear = function ()
	local pixels = {}
	for i=1,DISPLAY.resolution_x * DISPLAY.resolution_y do
		pixels[i] = 16
	end
	DISPLAY.update(pixels)
end