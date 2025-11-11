local display = require("projector:display")
local synchronizer = require("projector:synchronizer")
local instance_limit = require("projector:instance_limit")
local util = require("projector:util")
local highlight = require("projector:highlight")

function on_interact(x, y, z, pid)
    instance_limit.set_position({x, y, z})
    hud.show_overlay("projector:projector")
    display.position = { x, y, z }
    highlight.refresh()
    return true
end

function on_placed(x, y, z, playerid)
    if (synchronizer.get_status() == util.synchronizer_status.CAPTURING) then
        block.set(x, y, z, 0, 0)
        return
    end
    instance_limit.set_position({x, y, z})
    display.position = { x, y, z }
    entities.spawn("projector:projector_entity", {math.floor(x) + 0.5, y + 0.5, math.floor(z) + 0.5})
end

function on_broken(x, y, z, playerid)
    if (synchronizer.get_status() == util.synchronizer_status.CAPTURING) then
        block.set(x, y, z, block.index("projector:projector"), 0)
        return
    end
    local entities_arr = entities.get_all_in_box({x, y, z}, {1, 1, 1})
    if (entities_arr ~= nil) then
        for k,v in pairs(entities_arr) do
            local entity = entities.get(v)
            if (entity:def_name() == "projector:projector_entity") then
                entity:despawn()
            end
        end
    end
    instance_limit.remove()
    highlight.stop()
end