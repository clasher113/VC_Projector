local synchronizer = require("projector:synchronizer")

function on_hud_render()
	synchronizer.server_routine()
end