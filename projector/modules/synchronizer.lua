SYNC = {}
SYNC.isSyncing = false
SYNC.isCapturing = false
SYNC.working_directory = "export:VC_Projector/"
SYNC.inData = ""

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
