require "projector:synchronizer"
require "projector:display"

function on_world_open()
	DISPLAY.init()	
end

function on_world_save()
	SYNC.send("disconnect")
end