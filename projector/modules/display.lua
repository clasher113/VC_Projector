require "projector:synchronizer"

DISPLAY = {}
DISPLAY.resolution_x = 0
DISPLAY.resolution_y = 0
DISPLAY.offset_x = 0
DISPLAY.offset_y = 0
DISPLAY.refresh_rate = 0
DISPLAY.position_x = 0
DISPLAY.position_y = 0
DISPLAY.position_z = 0

local blocks_indices = {}
local refresh_timer = 0.0

DISPLAY.init = function ()
	for i=0, 15 do
		blocks_indices[tostring(i)] = block.index("projector:black" .. tostring(i))
	end
end

DISPLAY.update = function ()
	local refresh_interval = 1.0 / DISPLAY.refresh_rate
	refresh_timer = refresh_timer + time.delta()
	if (refresh_timer > refresh_interval) then
		refresh_timer = refresh_timer - refresh_interval

		-- vertical
		local x_start = DISPLAY.position_x + DISPLAY.offset_x
		local x_end = x_start + DISPLAY.resolution_x - 1
		local y_start = DISPLAY.position_y + DISPLAY.offset_y
		local y_end = y_start + DISPLAY.resolution_y - 1
		local z_start = DISPLAY.position_z + DISPLAY.offset_z

		local pixels = string.split(SYNC.inData, ":")

		local i = 1
		for y=y_start, y_end, 1 do 
			for x=x_start, x_end, 1 do 
				block.set(x, y, z_start, blocks_indices[pixels[i]], 0)
				i = i + 1
			end
		end
	end
end

DISPLAY.clear = function ()
	local index = block.index("core:air")
	-- vertical
	local x_start = DISPLAY.position_x + DISPLAY.offset_x
	local x_end = x_start + DISPLAY.resolution_x
	local y_start = DISPLAY.position_y + DISPLAY.offset_y
	local y_end = y_start + DISPLAY.resolution_y
	for x=x_start, x_end, 1 do 
		for y=y_start, y_end, 1 do 
			block.set(x, y, DISPLAY.position_z + DISPLAY.offset_z, index, 0)
		end
	end
end