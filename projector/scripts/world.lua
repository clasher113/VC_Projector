require "projector:synchronizer"
require "projector:display"
require "projector:instance_limit"

function on_world_open()
	SYNC.start_server()
	DISPLAY.initialize()
	LIMIT.load()
end

function on_world_save()
	SYNC.close_server()
	LIMIT.save()
end