local config = require("projector:config")
local display = require("projector:display")
local synchronizer = require("projector:synchronizer")
local util = require("projector:util")

local skeleton = entity.skeleton
local transform = entity.transform

local projector_bone_index = skeleton:index("projector")
local disk_1_bone_index = skeleton:index("disk_1")
local disk_2_bone_index = skeleton:index("disk_2")

function on_attacked(attacker, pid)
	entity:despawn()
end

function on_update(tps)
	local dst_pos = vec3.add(display.position, config.offset)
	local size = {0, 0, 0}

	if (config.orientation == util.orientation.VERTICAL) then
		if (config.axis == util.axis.X) then
			size = { config.resolution[1], config.resolution[2], 1.0 }
		elseif (config.axis == util.axis.Z) then
			size = { 1.0, config.resolution[2], config.resolution[1] }
		end
	elseif (config.orientation == util.orientation.HORIZONTAL) then
		if (config.axis == util.axis.X) then
			size = { config.resolution[1], 1.0, config.resolution[2] }
		elseif (config.axis == util.axis.Z) then
			size = { config.resolution[2], 1.0, config.resolution[1] }
		end
	end
	dst_pos = vec3.add(dst_pos, vec3.div(size, 2))
	local entity_pos = transform:get_pos()

	local matrix = mat4.look_at(entity_pos, dst_pos, { 0, 1, 0 })
	matrix = mat4.translate(matrix, entity_pos)
	matrix = mat4.transpose(matrix)
	skeleton:set_matrix(projector_bone_index, matrix)

	if (synchronizer.get_status() == util.synchronizer_status.CAPTURING) then
		matrix = mat4.rotate({1, 0, 0}, time.uptime() % 360 * 50)
		skeleton:set_matrix(disk_1_bone_index, matrix)
		skeleton:set_matrix(disk_2_bone_index, matrix)
	end
end
