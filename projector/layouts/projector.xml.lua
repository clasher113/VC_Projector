local config = require("projector:config")
local display = require("projector:display")
local rgb_addon = require("projector:rgb_addon")
local synchronizer = require("projector:synchronizer")
local util = require("projector:util")
local highlight = require("projector:highlight")
local multiplayer = require("projector:multiplayer")
local rules = require("projector:rules")

local orientations = {
	[util.orientation.VERTICAL] = "Vertical",
	[util.orientation.HORIZONTAL] = "Horizontal"
}
local axes = {
	[util.axis.X] = "X",
	[util.axis.Z] = "Z"
}

local show_additional_settings = false
local logs_num = 1
local single_time_init = false
local lag_timer = 0

local LAG_MAX_MILLISECONDS = 3000
local LOGS_MAX_COUNT = 20

local function desync()
	if (synchronizer.get_status() == util.synchronizer_status.READY) then
		synchronizer.set_status(util.synchronizer_status.CONNECTED)
	end
end

local function refresh_orientation_button()
	document["orientation"].text = gui.str("Orientation", PACK_ID) .. ": " .. gui.str(orientations[config.orientation], PACK_ID)
end

local function refresh_axis_button()
	document["axis"].text = gui.str("Axis", PACK_ID) .. ": " .. axes[config.axis]
end

local function refresh_additional_settings_button()
	document["additional_settings"].text = gui.str("Additional settings", PACK_ID) .. (show_additional_settings and " <<" or " >>")
end

local function on_gui_render()
	local anim_speed = 2000 -- pixels per second
	local size = document["root"].size
	if (show_additional_settings == true and size[1] < 860) then
		size[1] = math.min(860, size[1] + anim_speed * time.delta())
	end
	if (show_additional_settings == false and size[1] > 540) then
		size[1] = math.max(540, size[1] - anim_speed * time.delta())
	end
	document["root"].size = size
end

function on_open()
	if (single_time_init == true) then return end
	single_time_init = true

	document["allow_non_obstacle_blocks_checkbox"].checked = config.allow_non_obstacle_blocks
	document["allow_translucent_blocks_checkbox"].checked = config.allow_translucent_blocks
	document["allow_hidden_blocks_checkbox"].checked = config.allow_hidden_blocks
	document["allow_emissive_blocks_checkbox"].checked = config.allow_emissive_blocks
	document["allow_shadeless_blocks_checkbox"].checked = config.allow_shadeless_blocks
	document["use_bytearray"].checked = config.use_bytearray
	document["use_chunks"].checked = config.use_chunks
	document["highlight_area_checkbox"].checked = config.highlight_area
	document["clear_on_stop_checkbox"].checked = config.clear_on_stop
	document["refresh_rate_trackbar"].value = config.refresh_rate
	refresh_orientation_button()
	refresh_axis_button()
	rgb_consumer(config.rgb_mode)
	same_size_consumer(config.same_size)
	fps_consumer(config.refresh_rate)
	stop_on_lag_consumer(config.stop_on_lag_duration)
	multiplayer_buffer_size_consumer(config.multiplayer_buffer_size)
	refresh_additional_settings_button()
	refresh_status_label(synchronizer.get_status())
	refresh_framerate_label(0)

	if (not rgb_addon.is_loaded()) then
		document["rgb_mode"].tooltip = gui.str("RGB addon not installed", PACK_ID)
		document["rgb_mode"].tooltipDelay = 0
	end

	document["root"]:setInterval(1, on_gui_render)

	synchronizer.on_lag_callback = function()
		local enabled = config.stop_on_lag_duration ~= document["stop_on_lag_trackbar"].max
		local delta = time.delta() * 1000
		if (enabled and delta > config.stop_on_lag_duration) then
			lag_timer = lag_timer + delta
		else
			lag_timer = 0
		end
		if (lag_timer > LAG_MAX_MILLISECONDS) then
			stop()
			log_message(gui.str("Stopped due to lag", PACK_ID) .. " " .. tostring(math.round(delta, 0)) .. "ms, " ..
				gui.str("limit", PACK_ID) .. " " .. tostring(config.stop_on_lag_duration) .. "ms")
			lag_timer = 0
			return true
		end
		return false
	end

	local current_rules = rules.get_rules()
	document["rgb_mode"].enabled = current_rules.allow_rgb_mode
	document["refresh_rate_trackbar"].max = current_rules.fps_max
	if (#current_rules.allowed_orientations == 1) then
		document["orientation"].enabled = false
	end
	if (#current_rules.allowed_axes == 1) then
		document["axis"].enabled = false
	end
	if (multiplayer.get_side() == multiplayer.sides.SINGLEPLAYER) then
		document["multiplayer_buffer_size_trackbar"]:destruct()
		document["multiplayer_buffer_size_label"]:destruct()
	end
	if (rgb_addon.is_loaded()) then
		document["rgb_mode_settings_label"]:destruct()
		document["allow_non_obstacle_blocks_checkbox"]:destruct()
		document["allow_translucent_blocks_checkbox"]:destruct()
		document["allow_hidden_blocks_checkbox"]:destruct()
		document["allow_emissive_blocks_checkbox"]:destruct()
		document["allow_shadeless_blocks_checkbox"]:destruct()
	elseif (multiplayer.get_side() == multiplayer.sides.CLIENT) then
		document["allow_non_obstacle_blocks_checkbox"].enabled = false
		document["allow_translucent_blocks_checkbox"].enabled = false
		document["allow_hidden_blocks_checkbox"].enabled = false
		document["allow_emissive_blocks_checkbox"].enabled = false
		document["allow_shadeless_blocks_checkbox"].enabled = false
	end
end

function set_gui_enabled(flag)
	document["settings_1"].enabled = flag
	document["settings_2"].enabled = flag
	document["sync"].enabled = flag
end

function log_message(string)
	local color = (logs_num % 2 == 0 and "#ffffff10" or "#ffffff00")
	document["logs"]:add("<textbox id='log" .. tostring(logs_num) .. "' color='" .. color .. "' editable='false' multiline='true' text-wrap='true' autoresize='true'>" .. string .. "</textbox>")
	if (logs_num > LOGS_MAX_COUNT) then
		document["log" .. tostring(logs_num - LOGS_MAX_COUNT)]:destruct()
	end
	for i=logs_num,math.max(1, logs_num - LOGS_MAX_COUNT),-1 do 
		document["log" .. tostring(i)]:moveInto(document["logs"])
	end
	logs_num = logs_num + 1
end

function refresh_status_label(status)
	document["status_label"].text = gui.str("Status", PACK_ID) .. ": " .. gui.str(util.status_info[status].string, PACK_ID)
end

function stop()
	synchronizer.set_status(util.synchronizer_status.READY)
	document["main_button"].text = gui.str("Start", PACK_ID)
	refresh_framerate_label(0)
	if (config.clear_on_stop) then
		display.clear(hud.get_player())
	end
	log_message(gui.str("Capturing stopped", PACK_ID))
	highlight.refresh()
    if (multiplayer.get_side() == multiplayer.sides.CLIENT) then
        local api = multiplayer.get_api()
        api.events.send(PACK_ID, "capture_status", Bytearray( { 0 } ))
    end
end

function start()
	synchronizer.set_status(util.synchronizer_status.CAPTURING)
	document["main_button"].text = gui.str("Stop", PACK_ID)
	log_message(gui.str("Capturing started", PACK_ID))
	config.write()
	highlight.stop()
    if (multiplayer.get_side() == multiplayer.sides.CLIENT) then
        local api = multiplayer.get_api()
        api.events.send(PACK_ID, "capture_status", Bytearray( { 1 } ))
    end
end

function main_button_func()
	if (synchronizer.get_status() == util.synchronizer_status.READY) then
		start()
	elseif (synchronizer.get_status() == util.synchronizer_status.CAPTURING) then
		stop()
	else
		log_message(gui.str("You must synchronize first", PACK_ID))
	end
end

function synchronize()
	if (synchronizer.get_status() == util.synchronizer_status.NOT_CONNECTED) then
		log_message(gui.str("Not connected", PACK_ID))
		return
	end
	log_message(gui.str("Synchronization...", PACK_ID))
	synchronizer.set_status(util.synchronizer_status.SYNCING)
	config.write()
end

function toggle_orientation()
	config.orientation = config.orientation + 1
	if (config.orientation > #(orientations)) then
		config.orientation = 1
	end
	refresh_orientation_button()
	highlight.refresh()
end

function toggle_axis()
	config.axis = config.axis + 1
	if (config.axis > #(axes)) then
		config.axis = 1
	end
	refresh_axis_button()
	highlight.refresh()
end

function fps_consumer(value)
	document["refresh_rate_label"].text = gui.str("Projection refresh rate", PACK_ID) .. ": " .. tostring(value)
	if (config.refresh_rate == value) then return end
	config.refresh_rate = value
	desync()
end

function rgb_consumer(checked)
	config.rgb_mode = checked
	document["rgb_mode"].checked = config.rgb_mode
end

local function validate_textbox(input, min, max, textbox)
	local number = tonumber(input)
	textbox.tooltipDelay = 0
	if (number == nil) then
		textbox.tooltip = gui.str("Input must be a number", PACK_ID)
		return false
	end
	if (number > max) then
		textbox.tooltip = gui.str("Number too big. Possible maximum", PACK_ID) .. " - " .. tostring(max)
		return false
	elseif (number < min) then
		textbox.tooltip = gui.str("Number too low. Required minimum", PACK_ID) .. " - " .. tostring(min)
		return false
	end
	textbox.tooltip = ""
	return true
end

function projection_size_x_validator(string)
    local current_rules = rules.get_rules()
	return validate_textbox(string, current_rules.resolution_min[1], current_rules.resolution_max[1], document["projection_size_x"])
end

function projection_size_x_consumer(string)
	if (not projection_size_x_validator(string)) then return end
	config.resolution[1] = tonumber(string)
	highlight.refresh()
	if (config.same_size == true) then
		capture_size_x_consumer(string)
	end
	desync()
end

function projection_size_x_supplier()
	local temp = document["projection_size_x"].valid
	return tostring(config.resolution[1])
end

function projection_size_y_validator(string)
    local current_rules = rules.get_rules()
	return validate_textbox(string, current_rules.resolution_min[2], current_rules.resolution_max[2], document["projection_size_y"])
end

function projection_size_y_consumer(string)
	if (not projection_size_y_validator(string)) then return end
	config.resolution[2] = tonumber(string)
	highlight.refresh()
	if (config.same_size == true) then
		capture_size_y_consumer(string)
	end
	desync()
end

function projection_size_y_supplier()
	local temp = document["projection_size_y"].valid
	return tostring(config.resolution[2])
end

function capture_size_x_validator(string)
	return validate_textbox(string, 1, 1920, document["capture_size_x"])
end

function capture_size_x_consumer(string)
	if (not capture_size_x_validator(string)) then return end
	config.capture_size[1] = tonumber(string)
	desync()
end

function capture_size_x_supplier()
	local temp = document["capture_size_x"].valid
	return tostring(config.capture_size[1])
end

function capture_size_y_validator(string)
	return validate_textbox(string, 1, 1080, document["capture_size_y"])
end

function capture_size_y_consumer(string)
	if (not capture_size_y_validator(string)) then return end
	config.capture_size[2] = tonumber(string)
	desync()
end

function capture_size_y_supplier()
	local temp = document["capture_size_y"].valid
	return tostring(config.capture_size[2])
end

function projection_offset_x_validator(string)
    local current_rules = rules.get_rules()
	return validate_textbox(string, current_rules.offset_min[1], current_rules.offset_max[1], document["projection_offset_x"])
end

function projection_offset_x_consumer(string)
	if (not projection_offset_x_validator(string)) then return end
	config.offset[1] = tonumber(string)
	highlight.refresh()
end

function projection_offset_x_supplier()
	local temp = document["projection_offset_x"].valid
	return tostring(config.offset[1])
end

function projection_offset_y_validator(string)
    local current_rules = rules.get_rules()
	return validate_textbox(string, current_rules.offset_min[2], current_rules.offset_max[2], document["projection_offset_y"])
end

function projection_offset_y_consumer(string)
	if (not projection_offset_y_validator(string)) then return end
	config.offset[2] = tonumber(string)
	highlight.refresh()
end

function projection_offset_y_supplier()
	local temp = document["projection_offset_y"].valid
	return tostring(config.offset[2])
end

function projection_offset_z_validator(string)
    local current_rules = rules.get_rules()
	return validate_textbox(string, current_rules.offset_min[3], current_rules.offset_max[3], document["projection_offset_z"])
end

function projection_offset_z_consumer(string)
	if (not projection_offset_z_validator(string)) then return end
	config.offset[3] = tonumber(string)
	highlight.refresh()
end

function projection_offset_z_supplier()
	local temp = document["projection_offset_z"].valid
	return tostring(config.offset[3])
end

function same_size_consumer(checked)
	config.same_size = checked
	document["same_size"].checked = config.same_size
	document["capture_size_x"].enabled = not checked
	document["capture_size_y"].enabled = not checked
	if (checked) then
		capture_size_x_consumer(config.resolution[1])
		capture_size_y_consumer(config.resolution[2])
		highlight.refresh()
	end
end

function clear_on_stop_consumer(checked)
	config.clear_on_stop = checked
end

function refresh_framerate_label(framerate)
	document["framerate_label"].text = gui.str("Current framerate", PACK_ID) .. ": " .. tostring(framerate)
end

function toggle_additional_settings()
	show_additional_settings = not show_additional_settings
	refresh_additional_settings_button()
end

function use_bytearray_consumer(checked)
	config.use_bytearray = checked
end

function use_chunks_consumer(checked)
	config.use_chunks = checked
end

function highlight_area_consumer(checked)
	config.highlight_area = checked
	highlight.refresh()
end

function stop_on_lag_consumer(value)
	local is_max_value = value == document["stop_on_lag_trackbar"].max
	document["stop_on_lag_label"].text = gui.str("Stop on lag", PACK_ID) .. ": " .. (is_max_value and gui.str("Disabled", PACK_ID) or tostring(value) .. "ms")
	document["stop_on_lag_trackbar"].value = value
	config.stop_on_lag_duration = value
end

function multiplayer_buffer_size_consumer(value)
	document["multiplayer_buffer_size_label"].text = gui.str("Multiplayer send buffer size", PACK_ID) .. ": " .. tostring(value)
	document["multiplayer_buffer_size_trackbar"].value = value
	config.multiplayer_buffer_size = value
end

function allow_non_obstacle_blocks_consumer(flag)
	config.allow_non_obstacle_blocks = flag
	rgb_addon.initialize(false)
end

function allow_translucent_blocks_consumer(flag)
	config.allow_translucent_blocks = flag
	rgb_addon.initialize(false)
end

function allow_hidden_blocks_consumer(flag)
	config.allow_hidden_blocks = flag
	rgb_addon.initialize(false)
end

function allow_emissive_blocks_consumer(flag)
	config.allow_emissive_blocks = flag
	rgb_addon.initialize(false)
end

function allow_shadeless_blocks_consumer(flag)
	config.allow_shadeless_blocks = flag
	rgb_addon.initialize(false)
end

function clear_display()
	if (synchronizer.get_status() == util.synchronizer_status.CAPTURING) then
		log_message(gui.str("Projector is running now", PACK_ID))
	else
        config.write()
		display.clear(hud.get_player())
		log_message(gui.str("Display cleaned", PACK_ID))
	end
end

function on_close()
	if (multiplayer.get_side() == multiplayer.sides.SINGLEPLAYER) then
		config.write()
	end
end