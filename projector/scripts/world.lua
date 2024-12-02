require "projector:synchronizer"
require "projector:display"

function on_world_open()
	SYNC.start_server()
	DISPLAY.init()
end

function on_world_save()
	SYNC.close_server()
end