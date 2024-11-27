SYNC = {}
SYNC.isSyncing = false
SYNC.isCapturing = false
SYNC.working_directory = "export:VC_Projector/"
SYNC.inData = ""
SYNC.status = ""
SYNC.synchronized = false

local sync_timeout = 1.0
local sync_timer = 0.0

SYNC.send = function (string)
	file.write(SYNC.working_directory .. "out", string)
end

SYNC.receive = function ()
	local inFile = SYNC.working_directory .. "in"
	if (file.isfile(inFile) == false) then
		return false
	end
	SYNC.inData = file.read(inFile)
	if (string.len(SYNC.inData) == 0) then 
		return false
	end
	file.write(inFile, "")
	return true
end

SYNC.sync = function ()
	if (SYNC.isSyncing == true) then
		sync_timer = sync_timer + time.delta()
		if (sync_timer > sync_timeout) then
			sync_timer = 0.0
			SYNC.isSyncing = false
			SYNC.status = "Synchronization timeout"
		end
		if (SYNC.receive() == true) then
			if (SYNC.inData == "sync") then
				SYNC.send(tostring(DISPLAY.resolution_x) .. ":" .. tostring(DISPLAY.resolution_y))
				SYNC.isSyncing = false
				SYNC.status = "Synchronization success"
				SYNC.synchronized = true
			end
		end
	end
end

