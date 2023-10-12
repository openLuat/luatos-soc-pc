
_G.sys = require("sys")
require "sysplus"

sys.taskInit(function()
    sys.wait(100)
    local code, headers, body = http.request("GET", "https://www.air32.cn").wait()
    log.info("http", code, json.encode(headers), body)

    sys.wait(1000)
    
    local code, headers, body = http.request("GET", "https://air32.cn").wait()
    log.info("http", code, json.encode(headers), body)
end)

sys.run()
