require "projector:synchronizer"
require "projector:display"

function on_hud_render()
	SYNC.server_routine()
end