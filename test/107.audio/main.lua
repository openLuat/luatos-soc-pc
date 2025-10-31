
-- LuaTools需要PROJECT和VERSION这两个信息
PROJECT = "i2sdemo"
VERSION = "1.0.0"

-- sys库是标配
sys = require("sys")
exaudio = require("exaudio")
local taskName = "task_audio"

if wdt then
    --添加硬狗防止程序卡死，在支持的设备上启用这个功能
    wdt.init(9000)--初始化watchdog设置为9s
    sys.timerLoopStart(wdt.feed, 3000)--3s喂一次狗
end

-- 音频初始化设置参数,exaudio.setup 传入参数
local audio_setup_param ={
    model= "es8311",          -- 音频编解码类型,可填入"es8311","es8211"
    i2c_id = 0,          -- i2c_id,可填入0，1 并使用pins 工具配置对应的管脚
    pa_ctrl = gpio.AUDIOPA_EN,         -- 音频放大器电源控制管脚
    dac_ctrl = 20,        --  音频编解码芯片电源控制管脚,780ehv 默认使用20
}

--  播放结束回调
local function play_end(event)
    if event == exaudio.PLAY_DONE then
        log.info("播放完成",exaudio.is_end())
    end
end

--  音频播放的配置
local audio_play_param ={
    type= 0,                -- 播放类型，有0，播放文件，1.播放tts 2. 流式播放
                            -- 如果是播放文件,支持mp3,amr,wav格式
                            -- 如果是tts,内容格式见:https://wiki.luatos.com/chips/air780e/tts.html?highlight=tts
                            -- 流式播放，仅支持PCM 格式音频,如果是流式播放，则sampling_rate, sampling_depth,signed_or_unsigned 必填写
    --content = {"../../test/107.audio/1.amr", "../../test/107.audio/1.mp3"},
    content = "../../test/107.audio/1.mp3",        -- 如果播放类型为0时，则填入string 是播放单个音频文件,如果是表则是播放多段音频文件。
    cbfnc = play_end,            -- 播放完毕回调函数
}

---------------------------------
-----主task,处理播放音频---------
---------------------------------
local index_number = 1
local audio_path = nil
local function audio_task()
    log.info("开始播放音频文件")
    if exaudio.setup(audio_setup_param) then
        exaudio.play_start(audio_play_param) -- 仅仅支持task 中运行
        -- 播放完成后任务退出，让主线程处理回调
    end
end
sys.taskInitEx(audio_task, taskName)


-- 用户代码已结束---------------------------------------------
-- 结尾总是这一句
sys.run()
-- sys.run()之后后面不要加任何语句!!!!!
