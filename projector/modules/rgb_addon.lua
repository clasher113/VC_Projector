local data_buffer = require("core:data_buffer")
local config = require("projector:config")
local util = require("projector:util")

local rgb_addon = {
	blocks_indices = {}
}

local is_loaded = false
local addon_id = "projector_rgb_addon"
local textures = {}
local blocks = {}

function rgb_addon.initialize()
	if (pack.is_installed(addon_id)) then
		for i=0, 4095 do
			rgb_addon.blocks_indices[i] = block.index(addon_id .. ":rgb_" .. tostring(i))
		end
		is_loaded = true
	end
end

function rgb_addon.get_textures_data()
	local packs = {}
	local all_textures = {}
	for k, v in pairs(pack.get_installed()) do
		packs[v] = pack.get_info(v)
	end
	packs["core"] = { path = "res:" }
	for pack_name, pack in pairs(packs) do
		for __, texture_file in pairs(file.list(pack.path .. "/textures/blocks/")) do
			all_textures[file.stem(texture_file)] = texture_file
		end
	end
	for id = 1, block.defs_count() do
		local block_name = block.name(id)
		if (block_name == nil) then break end
		local obstacle = true
		if (block.properties[id]["obstacle"] ~= nil) then
			obstacle = block.properties[id]["obstacle"]
		end
		if (block.is_extended(id) == false and (block.get_model(id) == "block" ) and
			obstacle) then
			local block_textures = block.get_textures(id)
			blocks[id] = block_textures
			for k, texture_name in pairs(block_textures) do
				local pack_name = string.split(block_name, ":")[1]
				textures[texture_name] = all_textures[texture_name]
			end
		end
	end

	local data = data_buffer(nil, util.BYTE_ORDER, config.use_bytearray)
	for k, file_path in pairs(textures) do
		local bytes = file.read_bytes(file_path, false)
		data:put_uint32(#bytes)
		data:put_bytes(bytes)
	end
	return data
end

function rgb_addon.fetch_textures_color(colors)
	local i = 1
	for k, v in pairs(textures) do
		textures[k] = {
			colors[i], colors[i+1], colors[i+2], colors[i+3]
		}
		i = i + 4
	end
	for id, block_textures in pairs(blocks) do
		local avarage_color = { 0, 0, 0, 0 }
		for _, block_texture in pairs(block_textures) do
			for i=1,4 do
				avarage_color[i] = avarage_color[i] + textures[block_texture][i]
			end
		end
		avarage_color = vec4.div(avarage_color, #block_textures)
		local final_color = math.floor(avarage_color[1] / 16)
		final_color = bit.bor(final_color, bit.lshift(math.floor(avarage_color[2] / 16), 4))
		final_color = bit.bor(final_color, bit.lshift(math.floor(avarage_color[3] / 16), 8))
		rgb_addon.blocks_indices[final_color] = id
	end
	for i=0,4095 do
		if (rgb_addon.blocks_indices[i] == nil) then
			local closest_key = 0
			local smallest_difference = 4095
			for k, _ in pairs(rgb_addon.blocks_indices) do
				local difference = math.abs(k - i)
				if (difference < smallest_difference) then
					closest_key = k
					smallest_difference = difference
				end
			end
			rgb_addon.blocks_indices[i] = rgb_addon.blocks_indices[closest_key]
		end
	end
end

function rgb_addon.is_loaded()
	return is_loaded
end

return rgb_addon