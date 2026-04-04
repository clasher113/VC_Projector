local display = require("projector:display")
local synchronizer = require("projector:synchronizer")
local instance_limit = require("projector:instance_limit")
local multiplayer = require("projector:multiplayer")
local rules = require("projector:rules")

function on_placed(x, y, z, player_id)
    block.set(x, y, z, 0, 0, true)
    if (rules.get_rules(player_id).allow_use == false) then
        if (multiplayer.get_side() == multiplayer.sides.CLIENT) then
            console.chat("You are not permitted to use this block")
        end
        return
    end
    if (synchronizer.is_player_capturing(player_id)) then
        return
    end
    if (multiplayer.get_side() ~= multiplayer.sides.CLIENT) then
        local entity = entities.spawn("projector:projector_entity", {math.floor(x) + 0.5, y + 0.5, math.floor(z) + 0.5}, { projector__projector = {
            owner_pid = player_id
        }} )
        instance_limit.set_entity(player_id,  entity:get_uid())
    end
    display.set_position( { x, y, z }, player_id)
end