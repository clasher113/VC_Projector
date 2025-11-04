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

local function pack_color(rgb)
	return bit.bor(bit.bor(math.floor(rgb[1] / 16), 
		bit.lshift(math.floor(rgb[2] / 16), 4)), 
		bit.lshift(math.floor(rgb[3] / 16), 8))
end

local function unpack_color(rgb)
	return { bit.band(rgb, 0x00F), bit.rshift(bit.band(rgb, 0x0F0), 4), bit.rshift(bit.band(rgb, 0xF00), 8) }
end

function rgb_addon.initialize()
	if (pack.is_installed(addon_id)) then
		for i=0, 4095 do
			rgb_addon.blocks_indices[i] = block.index(addon_id .. ":rgb_" .. tostring(i))
		end
		is_loaded = true
	end
	rgb_addon.blocks_indices[65535] = block.index("core:air")
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
			colors[i], colors[i+1], colors[i+2]
		}
		i = i + 3
	end
	for id, block_textures in pairs(blocks) do
		local avarage_color = { 0, 0, 0 }
		for _, block_texture in pairs(block_textures) do
			avarage_color = vec3.add(avarage_color, textures[block_texture])
		end
		avarage_color = vec3.div(avarage_color, #block_textures)
		rgb_addon.blocks_indices[pack_color(avarage_color)] = id
	end
	local palette = table.copy(rgb_addon.blocks_indices)
	for i=0,4095 do
		if (rgb_addon.blocks_indices[i] == nil) then
			local target_color = unpack_color(i)
			local best_color_key = 0
			local factor_min = 1000000000
			for k, v in pairs(palette) do
				local current_color = unpack_color(k)
				local factor = math.pow(current_color[1] - target_color[1], 2) +
							  math.pow(current_color[2] - target_color[2], 2) +
							  math.pow(current_color[3] - target_color[3], 2)
				
				if (factor < factor_min) then 
					factor_min = factor
					best_color_key = k
				end
			end
			rgb_addon.blocks_indices[i] = rgb_addon.blocks_indices[best_color_key]
		end
	end
end

function rgb_addon.is_loaded()
	return is_loaded
end

return rgb_addon