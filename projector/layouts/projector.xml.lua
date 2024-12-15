local logs_panel
local status_label
local settings_container_1
local settings_container_2
local main_button
local sync_button
local orientation_button
local axis_button
local clear_on_stop_checkbox
local refresh_rate_trackbar
local refresh_rate_label
local projection_size_x_textbox
local projection_size_y_textbox
local projection_offset_x_textbox
local projection_offset_y_textbox
local projection_offset_z_textbox
local capture_size_x_textbox
local capture_size_y_textbox
local same_size_checkbox
local rgb_mode_checkbox

local default_projection_size_x = "180"
local default_projection_size_y = "120"
local default_capture_size_x = default_projection_size_x
local default_capture_size_y = default_projection_size_y
local default_projection_offset_x = "1"
local default_projection_offset_y = "0"
local default_projection_offset_z = "0"
local default_refresh_rate = 30

local orientations = {"Vertical", "Horizontal"}
local axes = {"X", "Z"}

local logs_num = 1
local single_time_init = false

function on_game_update()
	if (SYNC.is_connected()) then
		if (SYNC.is_capturing) then
			set_status("Projecting")
		elseif (SYNC.is_synchronized) then
			set_status("Ready")
		else
			set_status("Connected")
		end
	else
		set_status("Waiting for connection")
	end
	for k,v in pairs(SYNC.statuses) do
		log_message(v)
		SYNC.statuses[k] = nil
	end
end

function on_open()
	if (single_time_init == false) then

		-- retreive elements
		logs_panel = document["logs"]
		status_label = document["status"]
		settings_container_1 = document["settings_1"]
		settings_container_2 = document["settings_2"]
		main_button = document["main_button"]
		sync_button = document["sync"]
		orientation_button = document["orientation"]
		axis_button = document["axis"]
		clear_on_stop_checkbox = document["clear_on_stop"]
		refresh_rate_trackbar = document["refresh_rate"]
		refresh_rate_label = document["refresh_rate_label"]
		projection_size_x_textbox = document["projection_size_x"]
		projection_size_y_textbox = document["projection_size_y"]
		projection_offset_x_textbox = document["projection_offset_x"]
		projection_offset_y_textbox = document["projection_offset_y"]
		projection_offset_z_textbox = document["projection_offset_z"]
		capture_size_x_textbox = document["capture_size_x"]
		capture_size_y_textbox = document["capture_size_y"]
		same_size_checkbox = document["same_size"]
		rgb_mode_checkbox = document["rgb_mode"]

		single_time_init = true
		settings_container_1:setInterval(1, on_game_update)

		if (CONFIG.is_loaded() == true) then
			default_refresh_rate = CONFIG.refresh_rate
			default_projection_size_x = tostring(CONFIG.resolution_x)
			default_projection_size_y = tostring(CONFIG.resolution_y)
			default_capture_size_x = tostring(CONFIG.capture_size_x)
			default_capture_size_y = tostring(CONFIG.capture_size_y)
			default_projection_offset_x = tostring(CONFIG.offset_x)
			default_projection_offset_y = tostring(CONFIG.offset_y)
			default_projection_offset_z = tostring(CONFIG.offset_z)
			clear_on_stop_checkbox.checked = CONFIG.clear_on_stop
		else
			CONFIG.orientation = 1
			CONFIG.axis = 1
		end

		-- set default values
		refresh_rate_trackbar.value = default_refresh_rate
		projection_size_x_textbox.text = default_projection_size_x
		projection_size_y_textbox.text = default_projection_size_y
		projection_offset_x_textbox.text = default_projection_offset_x
		projection_offset_y_textbox.text = default_projection_offset_y
		projection_offset_z_textbox.text = default_projection_offset_z
		capture_size_x_textbox.text = projection_size_x_textbox.text
		capture_size_y_textbox.text = projection_size_y_textbox.text
		orientation_button.text = "Orientation: " .. orientations[CONFIG.orientation]
		axis_button.text = "Axis: " .. axes[CONFIG.axis]
		if (RGB.is_loaded() ~= true) then
			rgb_mode_checkbox.enabled = false
			rgb_mode_checkbox.checked = false
			settings_container_2:add("<container id='tooltip' color='#00000000' size='" .. rgb_mode_checkbox.size[1] .. "," .. rgb_mode_checkbox.size[2] .."' pos='".. rgb_mode_checkbox.pos[1] .. "," .. rgb_mode_checkbox.pos[2] .. "'></container>")
			document["tooltip"].tooltip = "RGB addon require"
			document["tooltip"].tooltipDelay = 0
		else
			rgb_mode_checkbox.checked = RGB.is_active
		end
		fps_consumer("")
		same_size_consumer(CONFIG.same_size)
		init_display()

		SYNC.on_disconnect_callback = function()
			if (SYNC.is_capturing == true) then
				stop()
			end
		end
	end
end

function set_gui_enabled(flag)
	settings_container_1.enabled = flag
	settings_container_2.enabled = flag
	sync_button.enabled = flag
end

function log_message(string)
	local size = logs_panel.size
	local color = (logs_num % 2 == 0 and "#ffffff10" or "#ffffff00")
	logs_panel:add("<textbox id='log" .. tostring(logs_num) .. "' color='" .. color .. "' editable='false' multiline='true' text-wrap='true' autoresize='true'>" .. string .. "</textbox>")
	for i=logs_num,1,-1 do 
		document["log" .. tostring(i)]:moveInto(logs_panel)
	end
	logs_panel.size = size
	logs_num = logs_num + 1
end

function set_status(string)
	status_label.text = "Status: " .. string
end

function stop()
	set_gui_enabled(true)
	SYNC.is_capturing = false
	main_button.text = "Start"
	if (clear_on_stop_checkbox.checked == true) then
		DISPLAY.clear()
	end
	log_message("Capturing stopped")
end

function start()
	set_gui_enabled(false)
	SYNC.is_capturing = true
	main_button.text = "Stop"
	init_display()
	log_message("Capturing started")
end

function main_button_func()
	if (SYNC.is_synchronized == true and SYNC.is_capturing == false) then
		start()
	elseif (SYNC.is_capturing  == true) then
		stop()
	elseif (SYNC.is_synchronized == false) then
		log_message("You must synchronize first")
	end
end

function get_orientation_index()
	return index_of(orientations, string.split(orientation_button.text, ": ")[2])
end

function get_axis_index()
	return index_of(axes, string.split(axis_button.text, ": ")[2])
end

function init_display()
	CONFIG.refresh_rate = refresh_rate_trackbar.value
	CONFIG.resolution_x = tonumber(projection_size_x_textbox.text)
	CONFIG.resolution_y = tonumber(projection_size_y_textbox.text)
	CONFIG.capture_size_x = tonumber(capture_size_x_textbox.text)
	CONFIG.capture_size_y = tonumber(capture_size_y_textbox.text)
	CONFIG.offset_x = tonumber(projection_offset_x_textbox.text)
	CONFIG.offset_y = tonumber(projection_offset_y_textbox.text)
	CONFIG.offset_z = tonumber(projection_offset_z_textbox.text)
	CONFIG.axis = get_axis_index()
	CONFIG.orientation = get_orientation_index()
	CONFIG.same_size = same_size_checkbox.checked
	CONFIG.clear_on_stop = clear_on_stop_checkbox.checked
	CONFIG.rgb_mode = rgb_mode_checkbox.checked
	CONFIG.write()
end

function synchronize()
	if (SYNC.is_connected() == false) then
		log_message("Not connected")
		return
	end
	log_message("Synchronization...")
	SYNC.is_syncing = true
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
	local index = get_orientation_index()
	index = index + 1
	if (index > #(orientations)) then 
		index = 1
	end
	orientation_button.text = "Orientation: " .. orientations[index]
end

function toggle_axis()
	local index = get_axis_index()
	index = index + 1
	if (index > #(axes)) then 
		index = 1
	end
	axis_button.text = "Axis: " .. axes[index]
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
	SYNC.is_synchronized = false
end

function projection_size_x_consumer(string)
	if (handle_textbox(string, 1, 255, "Projection size X") == false) then
		projection_size_x_textbox.text = default_projection_size_x
	end
	if (same_size_checkbox.checked == true) then
		capture_size_x_textbox.text = projection_size_x_textbox.text
	end
	SYNC.is_synchronized = false
end

function projection_size_y_consumer(string)
	if (handle_textbox(string, 1, 255, "Projection size X") == false) then
		projection_size_y_textbox.text = default_projection_size_y
	end
	if (same_size_checkbox.checked == true) then
		capture_size_y_textbox.text = projection_size_y_textbox.text
	end
	SYNC.is_synchronized = false
end

function capture_size_x_consumer(string)
	if (handle_textbox(string, 1, 1920, "Capture size X") == false) then
		capture_size_x_textbox.text = default_capture_size_x
	end
	SYNC.is_synchronized = false
end

function capture_size_y_consumer(string)
	if (handle_textbox(string, 1, 1080, "Capture size Y") == false) then
		capture_size_y_textbox.text = default_capture_size_y
	end
	SYNC.is_synchronized = false
end

function projection_offset_x_consumer(string)
	if (handle_textbox(string, 1, 255, "Projection offset X") == false) then
		projection_offset_x_textbox.text = default_projection_offset_x
	end
end

function projection_offset_y_consumer(string)
	if (handle_textbox(string, 0, 255, "Projection offset Y") == false) then
		projection_offset_y_textbox.text = default_projection_offset_y
	end
end

function projection_offset_z_consumer(string)
	if (handle_textbox(string, 0, 255, "Projection offset Z") == false) then
		projection_offset_z_textbox.text = default_projection_offset_z
	end
end

function same_size_consumer(checked)
	same_size_checkbox.checked = checked
	capture_size_x_textbox.enabled = (checked == false)
	capture_size_y_textbox.enabled = (checked == false)
	if (checked == true) then
		capture_size_x_textbox.text = projection_size_x_textbox.text
		capture_size_y_textbox.text = projection_size_y_textbox.text
	end
end

function rgb_mode_consumer(checked)
	
end

function clear_display()
	if (SYNC.is_capturing == true) then
		log_message("Does it make sense while the projector is running?")
	else
		init_display()
		DISPLAY.clear()
		log_message("Display cleaned")
	end
end
