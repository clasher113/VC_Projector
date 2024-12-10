require "projector:display"
require "projector:synchronizer"

local skeleton = entity.skeleton
local transform = entity.transform

local projector_bone_index = skeleton:index("projector")
local disk_1_bone_index = skeleton:index("disk_1")
local disk_2_bone_index = skeleton:index("disk_2")

function on_attacked(attacker, pid)
	entity:despawn()
end

function on_update(tps)
	local dst_pos = {DISPLAY.position_x + DISPLAY.offset_x, DISPLAY.position_y + DISPLAY.offset_y, DISPLAY.position_z + DISPLAY.offset_z}
	local size = {0, 0, 0}

	if (DISPLAY.orientation == 1) then
		if (DISPLAY.axis == 1) then
			size = {DISPLAY.resolution_x, DISPLAY.resolution_y, 1.0}
		elseif (DISPLAY.axis == 2) then
			size = {1.0, DISPLAY.resolution_y, DISPLAY.resolution_x}
		end
	elseif (DISPLAY.orientation == 2) then
		if (DISPLAY.axis == 1) then
			size = {DISPLAY.resolution_x, 1.0, DISPLAY.resolution_y}
		elseif (DISPLAY.axis == 2) then
			size = {DISPLAY.resolution_y, 1.0, DISPLAY.resolution_x}
		end
	end
	dst_pos = vec3.add(dst_pos, vec3.div(size, 2))

	local entity_pos = transform:get_pos()
	local direction = vec3.div(vec3.sub(dst_pos, entity_pos), math.sqrt(vec3.length(vec3.sub(dst_pos, entity_pos))))
	local rotation_x = math.atan2(-direction[1], -direction[3]) * 180.0 / math.pi
	local rotation_y = math.atan(direction[2]) * 180.0 / math.pi

	local matrix = mat4.rotate({0, 1, 0}, rotation_x)
	matrix = mat4.rotate(matrix, {1, 0, 0}, rotation_y)

	skeleton:set_matrix(projector_bone_index, matrix)
	if (SYNC.is_capturing == true) then
		matrix = mat4.rotate({1, 0, 0}, time.uptime() % 360 * 50)
		skeleton:set_matrix(disk_1_bone_index, matrix)
		skeleton:set_matrix(disk_2_bone_index, matrix)
	end
end
