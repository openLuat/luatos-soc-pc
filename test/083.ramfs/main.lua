PROJECT = "lcd_qspi"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

-- require "co5300"
-- require "jd9261t"
-- require "sh8601z"



sys.taskInit(function()
    local data = io.readFile("/luadb/test_qspi.jpg")
    log.info("文件大小(luadb)", #data)
    io.writeFile("/ram/test_qspi.jpg", data)
    local data2 = io.readFile("/ram/test_qspi.jpg")
    log.info("文件大小(ramfs)", #data2)

    log.info("是否相同", data == data2)
    log.info("md5", crypto.md5(data), crypto.md5(data2))

    local fd = io.open("/ram/test_qspi.jpg", "r")
    if fd then
        log.info("文件打开成功")
        for i = 1, 100000 do
            local chunk = fd:read(4095)
            if not chunk then break end
            log.info("读取数据块大小", #chunk)
        end

        -- 这里再读取一次，验证文件是否被关闭
        local chunk = fd:read(4095)
        if chunk then
            log.info("文件已经解决, 不应该能读出数据", #chunk)
        end

        fd:close()
        log.info("文件关闭成功")
    end
    -- io.writeFile("dataout.jpg", data2)


    os.exit(1)
end)

-- 用户代码已结束---------------------------------------------
-- 结尾总是这一句
sys.run()
-- sys.run()之后后面不要加任何语句!!!!!

