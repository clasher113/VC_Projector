local display = require("projector:display")
local synchronizer = require("projector:synchronizer")
local instance_limit = require("projector:instance_limit")
local util = require("projector:util")
local highlight = require("projector:highlight")

function on_interact(x, y, z, player_id)
    instance_limit.set_position(player_id, { x, y, z } )
    hud.show_overlay("projector:projector")
    display.position = { x, y, z }
    highlight.refresh()
    return true
end

function on_placed(x, y, z, player_id)
    if (synchronizer.get_status() == util.synchronizer_status.CAPTURING) then
        block.set(x, y, z, 0, 0)
        return
    end
    instance_limit.set_position(player_id, { x, y, z } )
    display.position = { x, y, z }
    entities.spawn("projector:projector_entity", {math.floor(x) + 0.5, y + 0.5, math.floor(z) + 0.5}, { projector__projector = {
        owner_pid = player_id
    }} )
end

function on_broken(x, y, z, player_id)
    if (synchronizer.get_status() == util.synchronizer_status.CAPTURING) then
        block.set(x, y, z, block.index("projector:projector"), 0)
        return
    end
    local entity = util.get_block_entity(x, y, z)
    if (entity) then
        entity:despawn()
    end
    instance_limit.on_broken(x, y, z, player_id)
    highlight.stop()
end