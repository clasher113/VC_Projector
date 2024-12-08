require "projector:display"
require "projector:synchronizer"
require "projector:instance_limit"

function on_interact(x, y, z, pid)
    LIMIT.set_position({x, y, z})
    hud.show_overlay("projector:projector")
    DISPLAY.position_x = x
    DISPLAY.position_y = y
    DISPLAY.position_z = z
    return true
end

function on_placed(x, y, z, playerid)
    LIMIT.set_position({x, y, z})
    entities.spawn("projector:projector_entity", {math.floor(x) + 0.5, y + 0.5, math.floor(z) + 0.5})
end

function on_broken(x, y, z, playerid)
    if (SYNC.is_capturing == true) then
        block.set(x, y, z, block.index("projector:projector"), 0)
        return
    end
    local entities_arr = entities.get_all_in_box({x, y, z}, {1, 1, 1})
    if (entities_arr == nil) then
        return
    end
    for k,v in pairs(entities_arr) do
        local entity = entities.get(v)
        if (entity:def_name() == "projector:projector_entity") then
            entity:despawn()
        end
    end
    LIMIT.remove()
end