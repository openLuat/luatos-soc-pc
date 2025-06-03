--加载sys库
_G.sys = require("sys")


sys.taskInit(function()

    local uiJson = io.open("/luadb/ui.json")
    local ui = json.decode(uiJson:read("*a"))
    log.info("ui", ui, ui.pages[1].children[1].name)
    log.info("初始化lvgl", lvgl.init(ui.project_settings.resolution.width, ui.project_settings.resolution.height))
    
    airui.init("/luadb/ui.json")
end)


sys.run()
