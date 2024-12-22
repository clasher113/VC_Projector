RGB = {}
RGB.blocks_indices = {}

local is_loaded = false
local addon_id = "projector_rgb_addon"

function RGB.initialize()
	if (pack.is_installed(addon_id)) then
		for i=0, 4095 do
			RGB.blocks_indices[i] = block.index(addon_id .. ":rgb_" .. tostring(i))
		end
		is_loaded = true
	end
end

function RGB.is_loaded()
	return is_loaded
end
