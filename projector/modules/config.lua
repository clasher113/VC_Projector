CONFIG = {}
CONFIG.resolution_x = 0
CONFIG.resolution_y = 0
CONFIG.capture_size_x = 0
CONFIG.capture_size_y = 0
CONFIG.offset_x = 0
CONFIG.offset_y = 0
CONFIG.offset_z = 0
CONFIG.refresh_rate = 0
CONFIG.orientation = 0
CONFIG.axis = 0
CONFIG.same_size = true
CONFIG.rgb_mode = false
CONFIG.clear_on_stop = true

local is_loaded = false
local config_file = pack.shared_file("projector", "config")

function CONFIG.read()
	if (file.isfile(config_file) == false) then
		return
	end
	local temp = bjson.frombytes(file.read_bytes(config_file))
	for k, v in pairs(temp) do
		-- print(k .. " = " .. tostring(v))
		CONFIG[k] = v
	end
	is_loaded = true
end

function CONFIG.write()
	local temp = {}
	for k, v in pairs(CONFIG) do
		if (type(v) ~= "function") then
			temp[k] = v
		end
	end
	file.write_bytes(config_file, bjson.tobytes(temp))
end

function CONFIG.is_loaded()
	return is_loaded
end
