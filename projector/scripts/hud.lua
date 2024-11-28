require "projector:synchronizer"
require "projector:display"

function on_hud_render()
	if (SYNC.synchronized == true) then
		if (SYNC.capture() == true) then
			DISPLAY.update()
		end
	end
end