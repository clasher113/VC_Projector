local display = require("projector:display")
local synchronizer = require("projector:synchronizer")
local instance_limit = require("projector:instance_limit")
local util = require("projector:util")
local highlight = require("projector:highlight")
local multiplayer = require("projector:multiplayer")
local rules = require("projector:rules")
local locale_change_listener = require("projector:locale_change_listener")

locale_change_listener.listen(PACK_ID .. ":" .. PACK_ID, PACK_ID .. ":layouts/projector.xml")

function on_interact(x, y, z, player_id)
    if (multiplayer.get_side() == multiplayer.sides.CLIENT and multiplayer.logged_in == false) then
        console.chat("Not logged in")
        return true
    end
    if (rules.get_rules(player_id).allow_use == false) then
        if (multiplayer.get_side() == multiplayer.sides.CLIENT) then
            console.chat("You are not permitted to use this block")
        end
        return true
    end
    local owner_pid = util.get_owner_pid(x, y, z)
    if (not owner_pid or owner_pid ~= player_id) then
        if (multiplayer.get_side() == multiplayer.sides.CLIENT) then
            console.chat("You are not owner")
        end
        return true
    end
    display.set_position(x, y, z, player_id)
    instance_limit.set_position(player_id, { x, y, z } )
    if (multiplayer.get_side() ~= multiplayer.sides.SERVER) then
        locale_change_listener.check(PACK_ID .. ":" .. PACK_ID)
        hud.show_overlay(PACK_ID .. ":" .. PACK_ID, false)
        highlight.refresh()
    end
    return true
end

function on_placed(x, y, z, player_id)
    if (rules.get_rules(player_id).allow_use == false) then
        if (multiplayer.get_side() == multiplayer.sides.CLIENT) then
            console.chat("You are not permitted to use this block")
        end
        block.set(x, y, z, 0, 0, true)
        return
    end
    if (synchronizer.is_player_capturing(player_id)) then
        block.set(x, y, z, 0, 0, true)
        return
    end
    instance_limit.set_position(player_id, { x, y, z } )
    if (multiplayer.get_side() ~= multiplayer.sides.CLIENT) then
        entities.spawn("projector:projector_entity", {math.floor(x) + 0.5, y + 0.5, math.floor(z) + 0.5}, { projector__projector = {
            owner_pid = player_id
        }} )
    end
    display.set_position(x, y, z, player_id)
end

function on_broken(x, y, z, player_id)
    local side = multiplayer.get_side()
    if (synchronizer.is_player_capturing(util.get_owner_pid(x, y, z))) then
        if (side ~= multiplayer.sides.CLIENT) then
            block.set(x, y, z, block.index("projector:projector"), 0, true)
        end
        return
    end
    local owner_pid = nil
    local entity = util.get_block_entity(x, y, z)
    if (entity) then
        owner_pid = entity:get_component("projector:projector").get_owner_pid()
        if (side ~= multiplayer.sides.CLIENT) then
            entity:despawn()
        end
    end
    if (side ~= multiplayer.sides.CLIENT) then
        instance_limit.on_broken(x, y, z, player_id)
    end
    if (side ~= multiplayer.sides.SERVER and owner_pid == hud.get_player()) then
        highlight.stop()
    end
end