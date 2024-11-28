require "projector:display"

function on_interact(x, y, z, pid)
    hud.show_overlay("projector:projector")
    DISPLAY.position_x = x
    DISPLAY.position_y = y
    DISPLAY.position_z = z
    return true
end