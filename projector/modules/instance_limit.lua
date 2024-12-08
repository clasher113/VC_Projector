LIMIT = {}

local position = nil
local block_index = block.index("projector:projector")
local file_path = pack.data_file("projector", "limit") 

LIMIT.set_position = function(pos)
	if (pos ~= nil and position ~= nil) then
		if (position[1] == pos[1] and position[2] == pos[2] and position[3] == pos[3]) then
			return
		end
	end
	LIMIT.remove()
	position = pos
end

LIMIT.remove = function()
	if (LIMIT.exist()) then
		block.destruct(position[1], position[2], position[3])
	end
	position = nil
end

LIMIT.exist = function()
	if (position ~= nil) then
		local index = block.get(position[1], position[2], position[3])
		if (index > 0 and index == block_index) then
			return true
		end
	end	
	return false
end

LIMIT.load = function()
	if (file.isfile(file_path)) then
		local buffer = data_buffer()
		buffer:put_bytes(file.read_bytes(file_path))
		buffer:set_position(1)
		position = {buffer:get_int64(), buffer:get_int64(), buffer:get_int64()}
	end
end

LIMIT.save = function()
	if (position ~= nil) then
		local buffer = data_buffer()
		for i=1,3 do
			buffer:put_int64(position[i])
		end
		file.write_bytes(file_path, buffer:get_bytes())
	end
end