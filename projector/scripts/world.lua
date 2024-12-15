require "projector:synchronizer"
require "projector:display"
require "projector:instance_limit"
require "projector:config"
require "projector:rgb_addon"

function on_world_open()
	CONFIG.read()
	RGB.initialize()
	SYNC.start_server()
	DISPLAY.initialize()
	LIMIT.load()
end

function on_world_save()
	SYNC.close_server()
	LIMIT.save()
end