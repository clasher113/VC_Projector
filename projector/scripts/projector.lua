require "projector:display"

function on_interact(x, y, z, pid)
    hud.show_overlay("projector:projector")
    DISPLAY.position_x = x
    DISPLAY.position_y = y
    DISPLAY.position_z = z
    return true
end

function on_placed(x, y, z, playerid)
    entities.spawn("projector:projector_entity", {math.floor(x) + 0.5, y + 0.5, math.floor(z) + 0.5})
end

function on_block_break_by(x, y, z, playerid)
    on_broken(x, y, z, playerid)
    print("test")
end

function on_broken(x, y, z, playerid)
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
end