# USB CDC TX/RX 双缓冲池详细设计

## 1. 目标与边界

重新设计 USB CDC 发送和接收路径，使用两个独立的字节环形池：

- TX Pool：256B，保存业务线程等待发送的数据。
- RX Pool：256B，保存 USB 物理层收到、等待协议线程解析的数据。

AT32 和 Linux 使用同一套池结构、Push/Pop、64B 分块和线程模型，只替换最底层物理读写函数。本次只改 USB CDC；USART1、USART6、RK3588 协议及任务控制状态机不参与重构。

## 2. 最终线程模型

保留现有 `communication_thread`，另外新增两个独立线程：

```text
业务/任务线程 → TX Pool → usb_tx_thread → AT32 CDC / Linux PTY
AT32 CDC / Linux PTY → usb_rx_thread → RX Pool → communication_thread → 命令队列
```

- `usb_tx_thread`：只消费 TX Pool 并调用物理发送。
- `usb_rx_thread`：只调用物理接收并向 RX Pool 写入。
- `communication_thread`：不直接访问物理 USB，只消费 RX Pool并解析命令。
- 业务线程：只调用 `HAL_USB_ACM_Send()` 将完整消息写入 TX Pool。

AT32 使用 FreeRTOS 任务；Linux 使用相同线程入口。Linux pool mutex 使用 `pthread_mutex_t`。

## 3. 环形池结构

```c
#define USB_POOL_SIZE       256U
#define USB_CDC_PACKET_SIZE 64U

typedef struct {
    uint8_t data[USB_POOL_SIZE];
    uint16_t head;   /* 下一次写入位置 */
    uint16_t tail;   /* 下一次读取位置 */
    uint16_t count;  /* 有效字节数，范围0..256 */
    USB_Mutex mutex;
} USB_Byte_Pool;
```

使用 `count` 区分空和满，256B 数组全部可用：

```text
empty: count == 0
full:  count == 256
free:  256 - count
```

跨数组末尾时使用两段复制：

```text
first  = min(len, 256 - index)
second = len - first
```

TX 与 RX 各自拥有独立结构和 mutex，不共享索引或锁。

## 4. Mutex 规则

平台封装：

```c
USB_Mutex_Init();
USB_Mutex_Lock();
USB_Mutex_Unlock();
```

- AT32：`xSemaphoreCreateMutex/xSemaphoreTake/xSemaphoreGive`。
- Linux：`pthread_mutex_init/lock/unlock`。
- mutex 只保护 `data/head/tail/count/last_push_tick`。
- 物理 USB I/O、格式化、命令解析、`ControlDelay()` 不得在持锁期间执行。
- Pool API 不允许从 ISR 调用；USB ISR 只维护 CDC 完成状态。
- 不需要递归 mutex。

“等待池空间”必须先释放 mutex，再延时重新检查，绝不持锁等待。

## 5. TX Pool 接口与 Push

```c
uint8_t USB_TX_Push(const uint8_t *data, uint16_t len);
uint16_t USB_TX_Pop(uint8_t *out, uint16_t max_len);
uint16_t USB_TX_Count(void);
uint16_t USB_TX_Free(void);
```

`USB_TX_Push()`：

1. `data == NULL`、`len == 0` 或 `len > 256` 返回0。
2. 加锁计算剩余空间。
3. 空间足够时一次性写入完整消息，更新 `head/count/last_push_tick`，解锁返回1。
4. 空间不足时不修改池，解锁后 `ControlDelay(10)`，再重新检查。

禁止部分写入。多生产者并发时，一条消息在一个临界区内完整写入，因此 ACK、EVT 和日志不会互相穿插。

`HAL_USB_ACM_Send()` 改为：

```c
uint8_t HAL_USB_ACM_Send(uint8_t *data, uint16_t len)
{
    return USB_TX_Push(data, len);
}
```

返回1只表示消息已经完整进入 TX Pool，不表示主机已经收到。

## 6. TX 触发、聚合与64B分包

每次成功 Push 都更新 `last_push_tick`。TX线程规则：

```text
count >= 64
    → 立即Pop 64B并发送

0 < count < 64 且 now - last_push_tick >= 10ms
    → Pop当前全部剩余数据并发送

count == 0
    → 延时1ms
```

这表示10ms是短消息聚合窗口：持续写入达到64B立即发送；未达到64B则从最后一次Push开始等待10ms。

```c
for (;;) {
    count = USB_TX_Count();

    if (count >= 64) {
        len = USB_TX_Pop(packet, 64);
    } else if (count > 0 && TickElapsed(last_push_tick) >= 10) {
        len = USB_TX_Pop(packet, count);
    } else {
        ControlDelay(1);
        continue;
    }

    USB_Physical_Send(packet, len);
    /* 不检查结果、不重试、不回滚 */
}
```

数据在 Pop 时已经从池中删除。未连接、CDC忙或 Linux 写失败时，本块直接丢弃，后续数据继续处理。任意一次物理发送必须满足 `1 <= len <= 64`。

## 7. AT32 TX 完成检查与超时继续发送

AT32 USB 函数会把本次数据写入 USB 专属 FIFO。发送函数返回后源缓冲区可以安全复用，因此不增加额外的 64B 全局稳定副本；TX 线程使用自身的 64B 局部数组即可。

`pcdc->g_tx_completed` 的约定：

- `1`：上一包已完成，可以提交下一包。
- `0`：上一包仍在发送，或完成回调尚未到达。

发送规则：

1. TX 线程每隔 2 ms 检查一次 `pcdc->g_tx_completed`。
2. 标志为 `1` 且满足发送条件时，从 TX Pool 弹出最多 64B 并调用 USB CDC 发送接口。
3. USB 发送开始时，驱动将 `g_tx_completed` 置为 `0`。
4. IN 完成回调 flush 对应端点 FIFO，并将 `g_tx_completed` 置为 `1`。
5. 如果该标志连续超过 10 ms 仍为 `0`，不做 flush、不清理端点，也不修改完成标志；直接从 TX Pool 取得下一包并再次调用发送函数。
6. 超时包已经从软件 TX Pool 弹出，按本次发送结束处理，不重新入池、不阻塞后续消息。USB 函数会自行把新数据写入 USB 专属 FIFO。

伪代码：

```c
for (;;) {
    if (USB_CDC_TxCompleted()) {
        if (USB_TX_Should_Send()) {
            uint8_t packet[64];
            uint16_t len = USB_TX_Pop(packet, sizeof(packet));

            tx_wait_start = ControlGetTick();
            (void)USB_Physical_Send(packet, len);
        }
    } else if (((ControlGetTick() - tx_wait_start) >= 10U) &&
               USB_TX_Should_Send()) {
        uint8_t packet[64];
        uint16_t len = USB_TX_Pop(packet, sizeof(packet));

        (void)USB_Physical_Send(packet, len);
        tx_wait_start = ControlGetTick();
    }

    ControlDelay(2);
}
```

10 ms超时只用于允许软件继续提交下一包，不触发任何USB清理动作。Linux实验端使用socat，不存在该发送超时问题，因此不实现 `g_tx_completed`、2 ms完成检查或10 ms超时分支；Linux TX线程只按第6节规则从Pool取出最多64B并调用一次 `LinuxSerial_Write()`。

## 8. RX Pool 与 RX 线程

接口：

```c
uint8_t USB_RX_Push(const uint8_t *data, uint16_t len);
uint16_t USB_RX_Pop(uint8_t *out, uint16_t max_len);
uint16_t USB_RX_Count(void);
uint16_t USB_RX_Free(void);
```

RX线程使用独立暂存包：

```c
uint8_t rx_packet[64];
uint16_t pending_len = 0;
```

流程：

1. AT32 每隔 10 ms 检查一次 `pcdc->g_rx_completed`；仅当标志为 `1` 时调用物理读取，最大请求64B。读取接口清除完成标志并重新使能 CDC OUT 端点。
2. Linux 每隔 10 ms 执行一次非阻塞物理读取，最大请求64B。
3. 收到数据后保存到 `rx_packet` 并设置 `pending_len`。
4. 加RX mutex检查剩余空间：
   - `free >= pending_len`：一次性写入完整物理包并清零 `pending_len`。
   - 空间不足：不写入、不覆盖，解锁后延时10ms再检查。
5. 当前暂存包成功进入 RX Pool 后才读取下一个物理包。

这样即使物理数据已经取出而池暂时放不下，也会保留在 `rx_packet` 中，不会丢失或部分写入。

AT32 的 `USB_CDC_GET()` 每次最多返回64B；Linux 的 `LinuxSerial_Read()`请求长度固定为64B。两端使用相同的10ms检查周期和 pending 逻辑。

## 9. RX 消费与命令行解析

`HAL_USB_ACM_Read()`只从 RX Pool 取数据：

```c
uint16_t HAL_USB_ACM_Read(uint8_t *buffer, uint16_t maxlen)
{
    return USB_RX_Pop(buffer, maxlen);
}
```

统一 AT32 和 Linux 当前重复的命令累计代码。状态为：

```c
uint8_t command[128];
uint16_t index;
uint8_t overflow;
uint8_t ignore_next_lf;
```

规则：

- 普通可打印ASCII追加到命令缓冲。
- Backspace/Delete删除一个尚未提交的字符。
- `\r`提交当前行，并设置 `ignore_next_lf=1`。
- 紧随 `\r` 的 `\n`只清除标志，不重复提交。
- 单独的 `\n`提交当前行。
- 超过127字符后设置overflow，并丢弃到下一行结束符。
- overflow行结束时只返回一次 `LINE_TOO_LONG`。
- 空行不产生命令和回复。

一次 `HAL_USB_Command_Handler()`持续 Pop，直到 RX Pool 为空，避免 RX 线程长期等待。

## 10. VOR事件路径调整

删除 `usb_vor_control.c` 中的 `event_queue/event_head/event_tail` 二级FIFO。

统一为：

```text
Event_Push_Response/Event_Push_State/Send_Immediate
    → 局部数组格式化（不超过128B）
    → HAL_USB_ACM_Send
    → TX Pool
```

`USB_VOR_Communication_Poll()`不再搬运事件；可直接删除，并同步删除 communication 线程中的调用。命令队列继续保留。

## 11. 物理层接口

```c
uint8_t USB_Physical_Init(void);
uint8_t USB_Physical_Send(const uint8_t *data, uint16_t len);
uint16_t USB_Physical_Read(uint8_t *data, uint16_t maxlen);
```

- `USB_Physical_Send`拒绝0字节或超过64B的请求。
- `USB_Physical_Read`最多返回 `min(maxlen, 64)`。
- AT32实现调用 `USB_CDC_Send/USB_CDC_GET`。
- Linux实现调用 `LinuxSerial_Write/LinuxSerial_Read`。
- 上层Pool与线程代码不得出现平台专用 I/O。

## 12. 初始化和退出

初始化顺序：

1. 清零 TX/RX Pool和线程运行标志。
2. 创建 TX mutex、RX mutex。
3. 初始化AT32 CDC或打开Linux PTY。
4. 创建 `usb_tx_thread`。
5. 创建 `usb_rx_thread`。
6. communication线程进入协议循环。

任一步失败，`HAL_USB_ACM_Init()`返回0，不启动后续线程。

Linux退出时设置线程退出标志、join两个线程、关闭PTY、销毁mutex。AT32运行期间不销毁任务和mutex。

AT32 当前可用内存余量按约5KB设计：TX/RX静态池固定各256B，共512B；索引、计数、mutex和时间戳等元数据应保持精简。64B TX/RX临时包放在线程栈中，不增加全局发送副本。复用已有128B命令缓冲，不使用动态链表、动态消息节点或多组包槽。

建议栈和优先级起点：

```text
usb_rx_thread: 256 words，priority 11
usb_tx_thread: 256 words，priority 11
communication: 保持现有配置
```

栈空间相对充足，但仍应通过 `uxTaskGetStackHighWaterMark()` 实测后调整，避免扩大静态内存。

## 13. API返回值

| API                     | 成功                              | 失败/无数据                    |
| ----------------------- | --------------------------------- | ------------------------------ |
| `HAL_USB_ACM_Init`    | 池、mutex、物理层和线程初始化完成 | 任一步初始化失败               |
| `HAL_USB_ACM_Send`    | 完整消息进入TX Pool               | 参数非法或未初始化             |
| `HAL_USB_ACM_Read`    | 返回从RX Pool读取的字节数         | 0                              |
| `HAL_USB_ACM_IsReady` | 软件池和线程已初始化              | 0                              |
| `USB_Physical_Send`   | 物理层接受本块                    | 未连接、忙或写失败；TX线程忽略 |

`HAL_USB_ACM_IsReady()`不代表USB主机已连接。

## 14. 测试清单

### Pool

- 写入/读取1、63、64、65、128、255、256B。
- 覆盖数组末尾时验证两段复制。
- 验证 `count` 始终为 `0..256`。
- 空间不足时不发生部分写入。
- 多生产者并发Push时消息不交叉。

### TX

- 小于64B在最后一次Push后10ms发送。
- 累计达到64B立即发送。
- 65B按64B和1B发送。
- 128B按两次64B发送。
- 模拟未连接、CDC忙和Linux写失败，确认数据仍消费且线程继续。
- AT32 TX线程每2ms检查一次 `g_tx_completed`。
- 模拟 `g_tx_completed` 连续为0超过10ms，确认不执行flush/reset，直接提交下一包。
- Linux socat路径只验证64B分包和顺序写入，不测试发送完成等待或发送超时恢复。

### RX

- 1、32、63、64B物理包写入RX Pool。
- pending包大于剩余空间时等待且不覆盖。
- communication消费后pending包继续写入。
- 命令跨多个64B包只解析一次。
- 连续命令、分片命令及 `\r`、`\n`、`\r\n`分别验证。
- 超长行只返回一次 `LINE_TOO_LONG`。
- AT32每10ms检查一次 `g_rx_completed`，Linux同样按10ms周期读取。

### 平台一致性

- Linux PTY和AT32使用相同Pool API、64B分包和解析代码；AT32专有的TX完成检查与10ms超时继续机制不移植到Linux。
- 两个平台每次物理TX/RX调用均不超过64B。
- USART1和USART6行为不变。

## 15. 最终约束

- TX Pool和RX Pool各256B，均为字节环形池。
- TX和RX各有一个独立线程。
- 每次物理USB发送和读取最大64B。
- Push整条消息；空间不足时释放mutex并每10ms重试。
- mutex内禁止延时、物理I/O和格式化。
- TX发送不重试，失败数据直接丢弃。
- RX不覆盖，使用64B暂存包等待池空间。
- AT32 USB驱动内部FIFO接管发送数据，不需要额外稳定发送副本。
- AT32 TX每2ms检查完成标志；连续10ms未完成时不清理USB，直接尝试发送下一包。
- Linux socat发送路径不增加完成标志、等待或超时恢复逻辑。
- AT32与Linux RX均按10ms周期检查或读取。
- TX/RX池固定各256B，共512B，不扩展为大池或动态包队列。
