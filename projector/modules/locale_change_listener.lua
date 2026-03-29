local locale_change_listener = {}
local checklist = {}

function locale_change_listener.listen(layout_name, path)
    checklist[layout_name] = {
        locale = gui.str("name", "locale"),
        path = path
    }
end

function locale_change_listener.check(layout_name)
    if (checklist[layout_name]) then
        local current_locale = gui.str("name", "locale")
        if (current_locale ~= checklist[layout_name].locale) then
            checklist[layout_name].locale = current_locale
            gui.load_document(checklist[layout_name].path, layout_name, {})
        end
    end
end

function locale_change_listener.unlisten(layout_name)
    checklist[layout_name] = nil
end

return locale_change_listener