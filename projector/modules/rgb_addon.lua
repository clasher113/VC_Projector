local config = require("projector:config")
local multiplayer = require("projector:multiplayer")
local libpng = nil

local rgb_addon = {
	blocks_indices = {}
}

local is_loaded = false
local RGB_ADDON_ID = "projector_rgb_addon"
local LIBPNG_ID = "libpng"

local function image_get_pixel(image, x, y, server_side)
	if (server_side) then
		return { image:get(x, y) }
	end

	local color = image:at(x, y)
	local pixel = { 0, 0, 0, 0 }
	local offset = 0

	for i = 1, 4 do
		pixel[i] = bit.rshift(bit.band(color, bit.lshift(0xFF, offset)), offset)
		offset = offset + 8
	end

	return pixel
end

function rgb_addon.on_world_open()
	local result = true
	if (pack.is_installed(RGB_ADDON_ID)) then
		debug.log("Projector: RGB addon found, initializing.")
		for i=0, 4095 do
			rgb_addon.blocks_indices[i] = block.index(RGB_ADDON_ID .. ":rgb_" .. tostring(i))
		end
		is_loaded = true
	else
		if (multiplayer.get_side() == multiplayer.sides.SERVER and pack.is_installed(LIBPNG_ID)) then
			libpng = require(LIBPNG_ID .. ":image")
		end
		debug.log("Projector: RGB addon not installed. Initializing without RGB addon.")
		result = rgb_addon.initialize()
	end
	if (result == true) then
		debug.log("Projector: RGB mode successfully initialized.")
	else
		debug.error("Projector: Error initializing RGB mode.")
	end
	return result
end

function rgb_addon.initialize()
	rgb_addon.blocks_indices = {}

	local blocks_textures = {}
	local all_textures = {}
    local textures_paths = {}
	local server_side = multiplayer.get_side() == multiplayer.sides.SERVER

	if (server_side) then
		if (libpng == nil) then
			debug.error("Projector: libpng not installed. RGB mode will not work.")
			return false
		end
		local packs_paths = {}
		for _, pack_id in pairs(pack.get_installed()) do
			packs_paths[pack_id] = pack.get_info(pack_id).path
		end
		packs_paths["core"] = "res:"

        for _, pack_path in pairs(packs_paths) do
            local textures_path = pack_path .. "/textures/blocks"
            if (file.isdir(textures_path)) then
                for _, file_name in pairs(file.list(textures_path)) do
					if (string.ends_with(file_name, ".png")) then
						textures_paths[file.stem(file_name)] = file_name
					end
				end
            end
		end
	end

	for block_id = 1, block.defs_count() - 1 do
		if (config.allow_non_obstacle_blocks == false) then
			local obstacle = true
			if (block.properties[block_id].obstacle ~= nil) then
				obstacle = block.properties[block_id].obstacle
			end
			obstacle = obstacle or config.allow_non_obstacle_blocks
			if (obstacle == false) then
				goto continue
			end
		end

		if (config.allow_hidden_blocks == false) then
			local hidden = item.index(block.name(block_id) .. ".item") == nil
			if (hidden == true) then
				goto continue
			end
		end

		if (config.allow_emissive_blocks == false) then
			if (block.properties[block_id].emission ~= nil) then
				goto continue
			end
		end

		if (config.allow_shadeless_blocks == false) then
			if (block.properties[block_id].shadeless == true) then
				goto continue
			end
		end

		if (block.is_extended(block_id) or block.get_model(block_id) ~= "block") then goto continue end

		local block_textures = block.get_textures(block_id)
		blocks_textures[block_id] = block_textures

		for _, texture_name in pairs(block_textures) do
			if (all_textures[texture_name] == nil) then
				local avarage_color = { 0, 0, 0, 0 }

				local image = nil

				if (server_side) then
					image = libpng.from_png(textures_paths[texture_name] or textures_paths["notfound"])
				else
					image = assets.to_canvas("blocks:" .. texture_name)
				end

				for x = 0, image.width - 1 do
					for y = 0, image.height - 1 do
						local pixel = image_get_pixel(image, x, y, server_side)
						local alpha = pixel[4]

						if (config.allow_translucent_blocks == false) then
							if (alpha ~= 255) then
								blocks_textures[block_id] = nil
								goto continue
							end
						end

						for i = 1, 3 do
							avarage_color[i] = avarage_color[i] + (pixel[i] + ((255 - pixel[i]) * ((255 - alpha) / 255)))
						end
						avarage_color[4] = avarage_color[4] + alpha
					end
				end

				vec4.div(avarage_color, image.width * image.height, avarage_color)

				all_textures[texture_name] = avarage_color
			end
		end

		::continue::
	end

	for id, block_textures in pairs(blocks_textures) do
		local avarage_color = { 0, 0, 0, 0 }
		for _, block_texture in pairs(block_textures) do
			vec4.add(avarage_color, all_textures[block_texture], avarage_color)
		end
		vec4.div(avarage_color, #block_textures, avarage_color)
		local color = bit.bor(bit.bor(math.floor(avarage_color[1] / 16),
			bit.lshift(math.floor(avarage_color[2] / 16), 4)),
			bit.lshift(math.floor(avarage_color[3] / 16), 8))

		rgb_addon.blocks_indices[color] = id
	end

	local function unpack_color(rgb)
		return { bit.band(rgb, 0x00F), bit.rshift(bit.band(rgb, 0x0F0), 4), bit.rshift(bit.band(rgb, 0xF00), 8) }
	end

	local palette = table.copy(rgb_addon.blocks_indices)
	for i=0,4095 do
		if (rgb_addon.blocks_indices[i] == nil) then
			local target_color = unpack_color(i)
			local best_color_key = 0
			local factor_min = 1000000000
			for key, _ in pairs(palette) do
				local current_color = unpack_color(key)
				local factor = math.pow(current_color[1] - target_color[1], 2) +
					math.pow(current_color[2] - target_color[2], 2) +
					math.pow(current_color[3] - target_color[3], 2)

				if (factor < factor_min) then
					factor_min = factor
					best_color_key = key
				end
			end
			rgb_addon.blocks_indices[i] = rgb_addon.blocks_indices[best_color_key]
		end
	end

	rgb_addon.blocks_indices[65535] = block.index("core:air")
	rgb_addon.blocks_indices[65534] = -1

	return true
end

function rgb_addon.is_loaded()
	return is_loaded
end

return rgb_addon
