
_G.sys = require("sys")
require "sysplus"
--[[
PC接Air530Z-BD
]]

local air530z = require("air530z")

local gps_uart_id = 35

sys.taskInit(function()
    air530z.setup({
        uart_id = gps_uart_id,
        debug = true,
        -- rmc_only = true,
        -- nmea_ver = 40,
        -- no_nmea = true
    })
    air530z.start()
    -- air530z.agps()
end)

local function reboot_wait(mode, tag, timeout)
    -- log.info("air530z", "测试热重启一次,并等待15秒")
    air530z.reboot(mode)
    local tnow = os.time()
    sys.waitUntil("GNSS_STATE", 3000)
    sys.waitUntil("GNSS_STATE", timeout)
    if libgnss.isFix() then
        log.info("air530z", tag .. "耗时",  os.time() - tnow)
    else
        log.info("air530z", tag .. "后定位超时")
    end
end

sys.taskInit(function()
    local tnow
    local result
    sys.waitUntil("GNSS_STATE")
    while true do
        sys.wait(3000)
        -- log.info("air530z", "测试热重启一次,并等待30秒")
        -- reboot_wait(0, "热重启", 30000)
        -- log.info("air530z", "测试温重启一次,并等待60秒")
        -- reboot_wait(1, "温重启", 60000)
        -- log.info("air530z", "测试冷重启一次,并等待600秒")
        -- reboot_wait(2, "冷重启", 600000)

        -- 下面的测试一般没必要
        -- log.info("air530z", "测试出厂重启一次,并等待240秒")
        -- air530z.reboot(3)
        -- sys.wait(100)
        -- air530z.stop()
        -- sys.wait(1000)
        -- air530z.start()
        -- libgnss.clear()
        -- -- sys.wait(1000)
        -- air530z.agps(true)
        -- sys.waitUntil("GNSS_STATE", 600000)
        sys.wait(60000)
    end
end)

sys.taskInit(function()
    while 1 do
        sys.wait(2000)
        -- log.info("RMC", json.encode(libgnss.getRmc(2) or {}, "7f"))
        -- log.info("RMC", (libgnss.getRmc(2) or {}).valid, os.date())
        -- log.info("INT", libgnss.getIntLocation())
        -- log.info("GGA", libgnss.getGga(3))
        -- log.info("GLL", json.encode(libgnss.getGll(2) or {}, "7f"))
        -- log.info("GSA", json.encode(libgnss.getGsa(1) or {}, "3f"))
        -- log.info("GSA", #libgnss.getGsa(0).sats)
        -- log.info("GSV", json.encode(libgnss.getGsv(2) or {}, "7f"))
        -- log.info("VTG", json.encode(libgnss.getVtg(2) or {}, "7f"))
        -- log.info("ZDA", json.encode(libgnss.getZda(2) or {}, "7f"))
        -- log.info("date", os.date())
        -- log.info("sys", rtos.meminfo("sys"))
        -- log.info("lua", rtos.meminfo("lua"))

        -- 打印全部卫星
        -- local gsv = libgnss.getGsv() or {sats={}}
        -- for i, v in ipairs(gsv.sats) do
        --     log.info("sat", i, v.nr, v.snr, v.azimuth, v.elevation)
        -- end
    end
end)

sys.run()
