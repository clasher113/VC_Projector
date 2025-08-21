local rgb_addon = {
	blocks_indices = {}
}

local is_loaded = false
local addon_id = "projector_rgb_addon"

function rgb_addon.initialize()
	if (pack.is_installed(addon_id)) then
		for i=0, 4095 do
			rgb_addon.blocks_indices[i] = block.index(addon_id .. ":rgb_" .. tostring(i))
		end
		is_loaded = true
	end
end

function rgb_addon.is_loaded()
	return is_loaded
end

return rgb_addon