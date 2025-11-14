local config = {
	resolution = { 180, 120 },
	capture_size = { 180, 120 },
	offset = { 1, 0, 0 },
	refresh_rate = 30,
	orientation = 1,
	axis = 1,
	stop_on_lag_duration = 150,
	same_size = true,
	rgb_mode = false,
	clear_on_stop = true,
	use_bytearray = true,
	highlight_area = true,
	use_chunks = true
}

local config_file = pack.shared_file("projector", "config")

function config.read()
	if (file.isfile(config_file) == false) then
		config.write()
		return
	end
	local rewrite = false
	local temp = bjson.frombytes(file.read_bytes(config_file))
	for k, v in pairs(config) do
		if (type(v) ~= "function") then
			if (temp[k] ~= nil and type(v) == type(temp[k]) and not (type(v) == "table" and #v ~= #temp[k])) then
				config[k] = temp[k]
			else
				rewrite = true
			end
		end
	end
    if rewrite then
        config.write()
    end
end

function config.write()
	local temp = {}
	for k, v in pairs(config) do
		if (type(v) ~= "function") then
			temp[k] = v
		end
	end
	file.write_bytes(config_file, bjson.tobytes(temp))
end

return config