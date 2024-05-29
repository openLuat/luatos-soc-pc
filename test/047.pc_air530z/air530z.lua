--[[

]]

local air530z = {}

local sys = require("sys")

function air530z.setup(opts)
    air530z.opts = opts
end

function air530z.start()
    -- 初始化串口
    local gps_uart_id = air530z.opts.uart_id or 2

    -- 切换波特率
    uart.setup(gps_uart_id, 9600)
    uart.write(gps_uart_id,"$PCAS01,5*19\r\n")
    sys.wait(100)
    uart.close(gps_uart_id)
    sys.wait(100)
    uart.setup(gps_uart_id, 115200)
    -- 是否为调试模式
    if air530z.opts.debug then
        libgnss.debug(true)
    end
    libgnss.bind(gps_uart_id)

    -- 配置NMEA版本, 4.1的GSA有额外的标识
    if not air530z.opts.nmea_ver or air530z.opts.nmea_ver >= 41 then
        air530z.writeCmd("PCAS05,2")
    else
        air530z.writeCmd("PCAS05,5")
    end
    -- 打开全部NMEA语句
    if air530z.opts.rmc_only then
        air530z.writeCmd("PCAS03,0,0,0,0,1,0,0,0,0,0,,,0,0,,,,0")
        air530z.writeCmd("PCAS03,,,,,,,,,,,0")
    elseif air530z.opts.no_nmea then
        air530z.writeCmd("PCAS03,0,0,0,0,0,0,0,0,0,0,,,0,0,,,,0")
        air530z.writeCmd("PCAS03,,,,,,,,,,,0")
    else
        air530z.writeCmd("PCAS03,1,1,1,1,1,1,1,1,0,0,,,0,0,,,,1")
        air530z.writeCmd("PCAS03,,,,,,,,,,,1")
    end
    -- 打开GPS+BD定位
    air530z.writeCmd("PCAS04,7")
    air530z.writeCmd("PCAS15,4,FFFF")
    air530z.writeCmd("PCAS15,5,1F")
    -- TODO打开星历信息语句
    sys.wait(20)

    -- air530z.writeCASIC(0x05, 0x01, string.char(0x0))
    air530z.writeQuery(0x06, 0x01)
    -- air530z.writeACK(0x06, 0x01)

    air530z.writeCASIC(0x06, 0x01, string.char(0x01, 0x00, 0x00, 0x00))
    sys.wait(10)
    air530z.writeCASIC(0x06, 0x01, string.char(0x01, 0x01, 0x00, 0x00))
    sys.wait(10)
    air530z.writeCASIC(0x06, 0x01, string.char(0x01, 0x02, 0x00, 0x00))
    sys.wait(10)
    air530z.writeCASIC(0x06, 0x01, string.char(0x01, 0x03, 0x00, 0x00))
    sys.wait(10)

    air530z.writeCASIC(0x06, 0x01, string.char(0x01, 0x10, 0x00, 0x00))
    sys.wait(10)

    air530z.writeCASIC(0x06, 0x01, string.char(0x01, 0x20, 0x00, 0x00))
    sys.wait(10)
    air530z.writeCASIC(0x06, 0x01, string.char(0x01, 0x21, 0x00, 0x00))
    sys.wait(10)

    air530z.writeCASIC(0x06, 0x01, string.char(0x03, 0x10, 0x00, 0x00))
    sys.wait(10)
    air530z.writeCASIC(0x06, 0x01, string.char(0x03, 0x11, 0x00, 0x00))
    sys.wait(10)
    -- 结束
end

function air530z.writeCmd(cmd, full)
    if not full then
        local ck = crypto.checksum(cmd)
        cmd = string.format("$%s*%02X\r\n", cmd, ck)
    end
    log.info("air530z", "写入指令", cmd:trim())
    uart.write(air530z.opts.uart_id, cmd)
end

function air530z.writeCASIC(class, id, data)
    local tmp = string.pack("<Hbb", #data, class, id) .. data
    local ckSum = (id << 24) + (class << 16) + #data
    if #data > 0 then
        for i = 1, #data, 4 do
            local stmp = data:sub(i, i + 3)
            log.info("stmp", stmp:toHex(), i, i + 3)
            ckSum = ckSum + string.unpack("<I", stmp)
        end
    end
    log.info("air530z", "checksum", string.format("%08X", ckSum))
    local cmd = "\xBA\xCE" .. tmp .. string.pack("<I", ckSum)
    log.info("air530z", "写入CASIC指令", class, id, (data:toHex()), (cmd:toHex()))
    uart.write(air530z.opts.uart_id, cmd)
end

function air530z.writeACK(class, id)
    air530z.writeCASIC(0x05, 0x01, string.char(class, id, 0, 0))
end

function air530z.writeNCK(class, id)
    air530z.writeCASIC(0x05, 0x00, string.char(class, id, 0, 0))
end


function air530z.writeQuery(class, id)
    air530z.writeCASIC(class, id, "")
end

function air530z.reboot(mode)
    local cmd = string.format("PCAS10,%d", mode or 0)
    air530z.writeCmd(cmd)
    if mode and mode == 2 then
        air530z.agps_tm = nil
        libgnss.clear()
    end
end

function air530z.stop()
    uart.close(air530z.opts.uart_id)
end

local function do_agps()
    -- 首先, 发起位置查询
    local lat, lng
    if mobile then
        mobile.reqCellInfo(6)
        sys.waitUntil("CELL_INFO_UPDATE", 6000)
    elseif wlan then
        wlan.scan()
        sys.waitUntil("WLAN_SCAN_DONE", 5000)
        local lbsLoc2 = require("lbsLoc2")
        lat, lng = lbsLoc2.request(5000)
        -- local lat, lng, t = lbsLoc2.request(5000, "bs.openluat.com")
        log.info("lbsLoc2", lat, lng)
    end
    if not lat then
        -- 获取最后的本地位置
        local locStr = io.readFile("/gnssloc")
        if locStr then
            local jdata = json.decode(locStr)
            if jdata and jdata.lat then
                lat = jdata.lat
                lng = jdata.lng
            end
        end
    end
    -- 然后, 判断星历时间和下载星历
    local now = os.time()
    local agps_time = tonumber(io.readFile("/zkw_tm") or "0") or 0
    if now - agps_time > 7200 then
        local code = http.request("GET", "http://download.openluat.com/9501-xingli/CASIC_data.dat", nil, nil, {dst="/ZKW.dat"}).wait()
        -- local code = http.request("GET", "http://download.openluat.com/9501-xingli/CASIC_data_bds.dat", nil, nil, {dst="/ZKW.dat"}).wait()
        log.info("air530z", "download agps file", code, io.fileSize("/ZKW.dat"))
    end

    local gps_uart_id = air530z.opts.uart_id or 2

    -- 写入星历
    local agps_data = io.readFile("/ZKW.dat")
    if agps_data and #agps_data > 1024 then
        log.info("air530z", "写入星历数据", "长度", #agps_data)
        for offset=1,#agps_data,512 do
            -- log.info("gnss", "AGNSS", "write >>>", #agps_data:sub(offset, offset + 511))
            uart.write(gps_uart_id, agps_data:sub(offset, offset + 511))
            sys.wait(100) -- 等100ms反而更成功
        end
    else
        log.info("air530z", "没有星历数据")
        return
    end

    -- 写入参考位置
    -- "lat":23.4068813,"min":27,"valid":true,"day":27,"lng":113.2317505
    if not lat or not lng then
        lat, lng = 23.4068813, 113.2317505
    end
    local dt = os.date("!*t")
    local lla = {lat=lat, lng=lng}
    local aid = libgnss.casic_aid(dt, lla)
    uart.write(gps_uart_id, aid.."\r\n")

    -- 写入参考时间

    -- 结束
    air530z.agps_tm = now
end

function air530z.agps(force)
    -- 如果不是强制写入AGPS信息, 而且是已经定位成功的状态,那就没必要了
    if not force and libgnss.isFix() then return end
    -- 先判断一下时间
    local now = os.time()
    if force or not air530z.agps_tm or now - air530z.agps_tm > 3600 then
        -- 执行AGPS
        log.info("air530z", "开始执行AGPS")
        do_agps()
    else
        log.info("air530z", "暂不需要写入AGPS")
    end
end

function air530z.saveloc(lat, lng)
    if not lat or not lng then
        if libgnss.isFix() then
            local rmc = libgnss.getRmc(3)
            if rmc then
                lat, lng = rmc.lat, rmc.lng
            end
        end
    end
    log.info("待保存的GPS位置", lat, lng)
    if lat and lng then
        local locStr = string.format('{"lat":%7f,"lng":%7f}', lat, lng)
        log.info("air530z", "保存GPS位置", locStr)
        io.writeFile("/gnssloc", locStr)
    end
end

sys.subscribe("GNSS_STATE", function(event)
    if event == "FIXED" then
        air530z.saveloc()
    end
end)


return air530z