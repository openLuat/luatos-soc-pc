
-- LuaTools需要PROJECT和VERSION这两个信息
PROJECT = "httpdemo"
VERSION = "1.0.0"

_G.sys = require("sys")
require "sysplus"
httpplus = require "httpplus"

sys.taskInit(function()
    sys.waitUntil("IP_READY")

    local code, resp = httpplus.request({url="http://httpbin.air32.cn/put", method="PUT", bodyfile="/luadb/wifi.json"})
    log.info("httpplus", code, resp.body:query())
end)

sys.run()
