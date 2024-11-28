require "projector:synchronizer"
require "projector:display"

local logs_panel
local status_label
local settings_container_1
local settings_container_2
local main_button
local sync_button
local orientation_button
local axis_button
local optimization_checkbox
local refresh_rate_trackbar
local refresh_rate_label
local projection_size_x_textbox
local projection_size_y_textbox
local projection_offset_x_textbox
local projection_offset_y_textbox
local projection_offset_z_textbox
local capture_size_x_textbox
local capture_size_y_textbox

local orientations = {"Vertical", "Horizontal"}
local axes = {"X", "Z"}

local logs_num = 1
local started = false
local on_game_update_added = false

function on_game_update()
	if (SYNC.isSyncing == true) then
		SYNC.sync()
	else 
		if (string.len(SYNC.status) > 0) then
			sync_button.enabled = true
			log_message(SYNC.status)
			SYNC.status = ""
		end
	end
end

function on_open()
	-- retreive elements
	logs_panel = document["logs"]
	status_label = document["status"]
	settings_container_1 = document["settings_1"]
	settings_container_2 = document["settings_2"]
	main_button = document["main_button"]
	sync_button = document["sync"]
	orientation_button = document["orientation"]
	axis_button = document["axis"]
	optimization_checkbox = document["optimization"]
	refresh_rate_trackbar = document["refresh_rate"]
	refresh_rate_label = document["refresh_rate_label"]
	projection_size_x_textbox = document["projection_size_x"]
	projection_size_y_textbox = document["projection_size_y"]
	projection_offset_x_textbox = document["projection_offset_x"]
	projection_offset_y_textbox = document["projection_offset_y"]
	projection_offset_z_textbox = document["projection_offset_z"]
	capture_size_x_textbox = document["capture_size_x"]
	capture_size_y_textbox = document["capture_size_y"]

	if (file.isdir(SYNC.working_directory) == false) then
		file.mkdir(SYNC.working_directory)
	end

	if (on_game_update_added == false) then
		settings_container_1:setInterval(1, on_game_update)
		on_game_update_added = true
	end
	set_status("Idle")
end

function log_message(string)
	local size = logs_panel.size
	local color = (logs_num % 2 == 0 and "#ffffff10" or "#ffffff00")
	logs_panel:add("<textbox id='log" .. tostring(logs_num) .. "' color='" .. color .. "' editable='false' multiline='true' text-wrap='true' autoresize='true'>" .. string .. "</textbox>")
	local current_elem = logs_num
	for i=logs_num,1,-1 do 
		document["log" .. tostring(i)]:moveInto(logs_panel)
	end
	logs_panel.size = size
	logs_num = logs_num + 1
end

function set_status(string)
	status_label.text = "Status: " .. string
end

function main_button_func()
	if (SYNC.synchronized == true and started == false) then
		settings_container_1.enabled = false
		settings_container_2.enabled = false
		sync_button.enabled = false
		SYNC.isCapturing = true
		main_button.text = "Stop"
		started = true
		SYNC.send("start")
		log_message("Capturing started")
	elseif (started == true) then
		settings_container_1.enabled = true
		settings_container_2.enabled = true
		sync_button.enabled = true
		SYNC.isCapturing = false
		main_button.text = "Start"
		started = false
		SYNC.send("stop")
		log_message("Capturing stopped")
	elseif (SYNC.synchronized == false)then
		log_message("You must synchronize first")
	end
end

function init_display()
	DISPLAY.refresh_rate = refresh_rate_trackbar.value
	DISPLAY.resolution_x = tonumber(projection_size_x_textbox.text)
	DISPLAY.resolution_y = tonumber(projection_size_y_textbox.text)
	DISPLAY.offset_x = tonumber(projection_offset_x_textbox.text)
	DISPLAY.offset_y = tonumber(projection_offset_y_textbox.text)
	DISPLAY.offset_z = tonumber(projection_offset_z_textbox.text)
end

function synchronize()
	sync_button.enabled = false
	log_message("Synchronization...")
	SYNC.send("sync")
	SYNC.isSyncing = true
	init_display()
end

function index_of(array, value)
    for i, v in ipairs(array) do
        if v == value then
            return i
        end
    end
    return nil
end

function toggle_orientation()
	local strings = string.split(orientation_button.text, ": ")
	local index = index_of(orientations, strings[2])
	index = index + 1
	if (index > #(orientations)) then 
		index = 1
	end
	orientation_button.text = "Orientation: " .. orientations[index]
	axis_button.visible = (index ~= 2)
	SYNC.synchronized = false
end

function toggle_axis()
	local strings = string.split(axis_button.text, ": ")
	local index = index_of(axes, strings[2])
	index = index + 1
	if (index > #(axes)) then 
		index = 1
	end
	axis_button.text = "Axis: " .. axes[index]
	SYNC.synchronized = false
end

function handle_textbox(string, min, max, textbox_id)
	local number = tonumber(string)
	if (number == nil) then
		log_message(textbox_id .. ": input must be a number")
		return false
	end
	if (number > max or number < min) then
		log_message(textbox_id .. ": the number must be less than " .. tostring(max) .. " and greater than " .. tostring(min))
		return false
	end
	return true
end

function fps_consumer(string)
	refresh_rate_label.text = "Projection refresh rate: " .. tostring(refresh_rate_trackbar.value)
	SYNC.synchronized = false
end

function projection_size_x_consumer(string)
	if (handle_textbox(string, 1, 255, "Projection size X") == false) then
		projection_size_x_textbox.text = default_projection_size_x
	end
	capture_size_x_textbox.text = projection_size_x_textbox.text
	SYNC.synchronized = false
end

function projection_size_y_consumer(string)
	if (handle_textbox(string, 1, 255, "Projection size Y") == false) then
		projection_size_y_textbox.text = default_projection_size_y
	end
	capture_size_y_textbox.text = projection_size_y_textbox.text
	SYNC.synchronized = false
end

function projection_offset_x_consumer(string)
	if (handle_textbox(string, 1, 255, "Projection offset X") == false) then
		projection_offset_x_textbox.text = default_projection_offset_x
	end
	SYNC.synchronized = false
end

function projection_offset_y_consumer(string)
	if (handle_textbox(string, 0, 255, "Projection offset Y") == false) then
		projection_offset_y_textbox.text = default_projection_offset_y
	end
	SYNC.synchronized = false
end

function clear_display()
	init_display()
	DISPLAY.clear()
	log_message("Display cleaned")
end
