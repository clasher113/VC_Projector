local synchronizer = require("projector:synchronizer")
local locale_change_listener = require("projector:locale_change_listener")

locale_change_listener.listen(PACK_ID .. ":" .. PACK_ID, PACK_ID .. ":layouts/projector.xml")

function on_hud_render()
	synchronizer.server_routine()
end