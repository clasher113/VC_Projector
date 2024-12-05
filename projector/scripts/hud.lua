require "projector:synchronizer"

function on_hud_render()
	SYNC.server_routine()
end