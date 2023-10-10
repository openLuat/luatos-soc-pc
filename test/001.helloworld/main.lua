
_G.sys = require("sys")

print('Go')

sys.timerStart(function()
    log.info("timer", "timeout once")
end, 1000)

sys.timerLoopStart(function()
    log.info("timer", "3s repeat")
end, 3000)

sys.run()
