local config = require("projector:config")
local rgb_addon = require("projector:rgb_addon")
local synchronizer = require("projector:synchronizer")
local display = require("projector:display")
local instance_limit = require("projector:instance_limit")
local highlight = require("projector:highlight")

function on_world_open()
	config.read()
	rgb_addon.initialize()
	display.initialize()
	instance_limit.load()
	synchronizer.start_server()
end

function on_world_tick()
	highlight.update()
end

function on_world_save()
	synchronizer.close_server()
	instance_limit.save()
end