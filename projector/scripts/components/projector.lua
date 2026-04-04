local config = require("projector:config")
local display = require("projector:display")
local synchronizer = require("projector:synchronizer")
local util = require("projector:util")
local multiplayer = require("projector:multiplayer")
local highlight = require("projector:highlight")
local rules = require("projector:rules")
local locale_change_listener = require("projector:locale_change_listener")
local instance_limit = require("projector:instance_limit")

local skeleton = entity.skeleton
local transform = entity.transform

local projector_bone_index = skeleton:index("projector")
local disk_1_bone_index = skeleton:index("disk_1")
local disk_2_bone_index = skeleton:index("disk_2")
local owner_text_preset = {
    display = "y_free_billboard",
    scale = 0.01,
    render_distance = 16
}

local owner_pid = SAVED_DATA.owner_pid or ARGS.owner_pid or 0

local text_id, TextObject
if (multiplayer.get_side() == multiplayer.sides.SERVER) then
    local api = multiplayer.get_api()
    text_id, TextObject = api.text3d.show(vec3.add(transform:get_pos(), { 0, 0.7, 0 } ), "Owner: " .. player.get_name(owner_pid), owner_text_preset)
end
entity.rigidbody:set_gravity_scale(0)

function on_used(player_id)
	if (multiplayer.get_side() == multiplayer.sides.CLIENT and multiplayer.logged_in == false) then
        console.chat("Not logged in")
        return
    end
	if (rules.get_rules(player_id).allow_use == false) then
        if (multiplayer.get_side() == multiplayer.sides.CLIENT) then
            console.chat("You are not permitted to use this block")
        end
        return
    end
	if (not owner_pid or owner_pid ~= player_id) then
        if (multiplayer.get_side() == multiplayer.sides.CLIENT) then
            console.chat("You are not owner")
        end
        return
    end
	local pos = transform:get_pos()
	display.set_position({ math.floor(pos[1]), math.floor(pos[2]), math.floor(pos[3]) }, player_id)
    instance_limit.set_entity(player_id, entity:get_uid())
    if (multiplayer.get_side() ~= multiplayer.sides.SERVER) then
        locale_change_listener.check("projector:projector")
        hud.show_overlay("projector:projector", false)
        highlight.refresh()
    end
end

function on_attacked(attacker, player_id)
	if (synchronizer.is_player_capturing(owner_pid)) then
        return
    end
	instance_limit.on_despawned(owner_pid, entity:get_uid())
	entity:despawn()
end

function on_update(tps)
    local player_config = config.get_player_config(owner_pid)
    local display_pos = display.get_position(owner_pid)
    if (not player_config or not display_pos) then return end
    local dst_pos = vec3.add(display_pos, player_config.offset)
	local size = { 0, 0, 0 }

	if (player_config.orientation == util.orientation.VERTICAL) then
		if (player_config.axis == util.axis.X) then
			size = { player_config.resolution[1], player_config.resolution[2], 1.0 }
		elseif (player_config.axis == util.axis.Z) then
			size = { 1.0, player_config.resolution[2], player_config.resolution[1] }
		end
	elseif (player_config.orientation == util.orientation.HORIZONTAL) then
		if (player_config.axis == util.axis.X) then
			size = { player_config.resolution[1], 1.0, player_config.resolution[2] }
		elseif (player_config.axis == util.axis.Z) then
			size = { player_config.resolution[2], 1.0, player_config.resolution[1] }
		end
	end
	vec3.add(dst_pos, vec3.div(size, 2), dst_pos)
	local entity_pos = transform:get_pos()

	local matrix = mat4.look_at(entity_pos, dst_pos, { 0, 1, 0 })
	matrix = mat4.translate(matrix, entity_pos)
	matrix = mat4.transpose(matrix)
	skeleton:set_matrix(projector_bone_index, matrix)

	if (synchronizer.is_player_capturing(owner_pid)) then
		matrix = mat4.rotate({1, 0, 0}, time.uptime() % 360 * 50)
		skeleton:set_matrix(disk_1_bone_index, matrix)
		skeleton:set_matrix(disk_2_bone_index, matrix)
	end
end

function get_owner_pid()
    return owner_pid
end

function set_owner_pid(new_pid)
    owner_pid = new_pid
end

function on_custom_field_update(name, val)
	if (name == "owner_pid") then
        owner_pid = val
    end
end

function on_despawn()
    if (multiplayer.get_side() == multiplayer.sides.SERVER) then
        local api = multiplayer.get_api()
        api.text3d.hide(text_id)
    elseif (owner_pid == hud.get_player()) then
        highlight.stop()
    end
end

function on_save()
    SAVED_DATA.owner_pid = owner_pid
end