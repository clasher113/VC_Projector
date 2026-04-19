local config = require("projector:config")
local rgb_addon = require("projector:rgb_addon")
local synchronizer = require("projector:synchronizer")
local display = require("projector:display")
local instance_limit = require("projector:instance_limit")
local highlight = require("projector:highlight")
local multiplayer = require("projector:multiplayer")
local rules = require("projector:rules")

function on_world_open()
    local api, side = multiplayer.on_world_open()
    config.on_world_open()
	local rgb_initialized = rgb_addon.on_world_open(side == multiplayer.sides.SERVER)
	display.on_world_open()
	instance_limit.on_world_open()
    if (side == multiplayer.sides.SERVER) then
        rules.on_world_open(rgb_initialized)
    end
    if (side ~= multiplayer.sides.SERVER) then
        synchronizer.start_server()
    end
    synchronizer.initialize_events()
end

function on_world_tick()
	highlight.update()
end

function on_world_save()
	instance_limit.save()
    if (multiplayer.get_side() == multiplayer.sides.SERVER) then
        config.write()
    end
end

function on_world_quit()
    if (multiplayer.get_side() ~= multiplayer.sides.SERVER) then
	    synchronizer.close_server()
    end
end