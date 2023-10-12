
_G.sys = require("sys")
require "sysplus"

sys.taskInit(function()
    sys.wait(100)
    log.info("创建 netc")
    local running = true
    local rxbuff = zbuff.create(1024)
    local netc = socket.create(nil, function(sc, event)
        log.info("socket", sc, event)
        if event == socket.EVENT then
            local ok = socket.rx(sc, rxbuff)
            if ok then
                log.info("socket", "读到数据", rxbuff:query())
                rxbuff:del()
            else
                log.info("socket", "服务器断开了连接")
                running = false
            end
        end
    end)
    log.info("netc", netc)
    socket.config(netc, nil, true)
    socket.debug(netc, true)
    log.info("执行连接")
    local ok = socket.connect(netc, "112.125.89.8", 44569)
    log.info("socket connect", ok)

    sys.wait(100)

    -- socket.tx(netc, "ABC", "112.125.89.8", 45022)
    socket.tx(netc, "ABC1234567890")

    while running do
        sys.wait(100)
    end
    log.info("连接中断, 关闭资源")
    socket.close(netc)
    socket.release(netc)
    log.info("全部结束")
end)

sys.run()
