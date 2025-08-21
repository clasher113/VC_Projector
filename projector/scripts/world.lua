local config = require("projector:config")
local rgb_addon = require("projector:rgb_addon")
local synchronizer = require("projector:synchronizer")
local display = require("projector:display")
local instance_limit = require("projector:instance_limit")

function on_world_open()
	config.read()
	rgb_addon.initialize()
	synchronizer.start_server()
	display.initialize()
	instance_limit.load()
end

function on_world_save()
	synchronizer.close_server()
	instance_limit.save()
end