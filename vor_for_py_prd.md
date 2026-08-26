# USB PC 控制协议预览

所有命令使用 ASCII 单行格式、115200 波特率，以 `\r\n` 结束。`ID` 为请求序号。

## 启动命令

每个 `Start` 只运行一个任务。任务完成、停止或失败后释放 USB 所有权并返回 `IDLE`。

```text
Ctrl:VOR.Start  ID=1 C=100 V=50 F=1.6 E=0
Ctrl:CONT.Start ID=2 S=30  V=50 O=0
Ctrl:OVAR.Start ID=3 S=30  V=50 I=10
Ctrl:VHIT.Start ID=4 C=20  D=1
Ctrl:TC.Start   ID=5 C=20  V=50
```

参数：

- `C`：次数，`1..1000`
- `S`：秒数，`1..1000`
- `V`：主速度，`-360..360`
- `O`：CONT 的 OKR 速度，`-360..360`
- `F`：VOR 频率，`0 < F <= 11`
- `E`：VOR 扩展模式，`0..3`
- `I`：OVAR 倾角，`0..50`
- `D`：VHIT 方向，`0=CCW`、`1=CW`

## 运行命令

命令前缀必须与当前模式一致：

```text
Ctrl:CONT.Pause  ID=10
Ctrl:CONT.Resume ID=11
Ctrl:CONT.Status ID=12
Ctrl:CONT.Stop   ID=13
```

VOR、CONT、OVAR、TC 支持 Pause/Resume；所有模式支持 Stop/Status。VHIT 使用：

```text
Ctrl:VHIT.Next ID=20
```

## 回复和事件

```text
Ctrl:ACK ID=1 CMD=VOR.Start CODE=0 STATE=STARTING MSG=STARTED
Ctrl:NACK ID=2 CMD=CONT.Pause CODE=1003 STATE=RUNNING MSG=INVALID_STATE
Ctrl:EVT SEQ=1 EVENT=RUNNING MODE=OVAR STATE=RUNNING CODE=0 PCT=0 REM=30
```

正常任务事件最终为 `COMPLETED → IDLE`；人工停止为 `STOPPING → STOPPED → IDLE`。
