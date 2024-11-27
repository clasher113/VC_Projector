require "projector:synchronizer"
require "projector:display"

function on_world_tick()
	if (SYNC.isSyncing == true) then
		if (SYNC.receive() == true) then
			if (SYNC.inData == "sync") then
				SYNC.send(tostring(DISPLAY.resolution_x) .. ":" .. tostring(DISPLAY.resolution_y))
				SYNC.isSyncing = false
			end
		end
	end
end

function on_world_save()
	SYNC.send("disconnect")
end
