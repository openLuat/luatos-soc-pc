
sys = require "sys"

sys.taskInit(function()
    local str = "$GNRMC,005814.000,V,,,,,,,251123,,,M,V*24\r\n"
    libgnss.parse(str)
    log.info("rmc", json.encode(libgnss.getRmc(), "7f"))
end)

sys.run()

