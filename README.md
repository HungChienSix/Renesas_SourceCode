
设计目标:一款可以检测多个环境数据以及血氧的智能手环,并且可以通过蓝牙将数据传输到手机上,以便用户随时查看自己的健康状况。

---

## Q: 使用非时间片抢占式调度,我该如何规划各个线程的功能以及优先级?

### 1. 调度器选型回顾

项目 `FreeRTOSConfig.h` 关键参数:

| 参数 | 值 | 含义 |
|------|----|------|
| `configUSE_PREEMPTION`   | 1   | 抢占式 |
| `configUSE_TIME_SLICING` | **0** | **同优先级任务不轮转** |
| `configTICK_RATE_HZ`     | 1000 | 1 ms tick |
| `configMAX_PRIORITIES`   | 5   | 优先级 0~4 |
| `configTIMER_TASK_PRIORITY` | 3 | 软件定时器守护在 p3 |
| `configIDLE_SHOULD_YIELD` | 1  | idle 让出同优先级 |
| `configUSE_MUTEXES`      | 0   | 不能用 mutex(改用"单写多读" + 任务通知) |
| `configUSE_TASK_NOTIFICATIONS` | 1 | 任务通知可用 |
| `configSUPPORT_STATIC_ALLOCATION` | 1 | 全静态分配 |

**"非时间片抢占" 的核心语义**:
- 任务一直跑到 **自己阻塞**(vTaskDelay / 信号量 / 队列),才让出 CPU
- **同优先级两个 ready 任务,先 ready 的独占,后 ready 的饿死**
- 高优先级 ready → 立刻抢低优先级
- 不存在 "用完一个 tick 就强制切走" 的时间片机制

### 2. 由此推出的设计铁律

1. **每个 while(1) 必须有阻塞点** — `vTaskDelay` / `vTaskDelayUntil` / `xQueueReceive(..., timeout)` 任选
2. **周期任务用 `vTaskDelayUntil`** — 不用 `vTaskDelay`,绝对周期不漂移
3. **同优先级不要塞两个常驻 task** — 要么拆优先级,要么确认它们有自然阻塞点
4. **数据共享走 "单写多读"** — `configUSE_MUTEXES=0`,32-bit 对齐读不会撕裂
5. **空闲优先级 (0) 只放 idle** — 业务别塞进去,`configIDLE_SHOULD_YIELD=1` 不救你
6. **长算法拆成多帧** — 别在 p2 一次占 10 ms,会卡 screen / uart

### 3. 线程清单(智能手环 3 期目标)

`configMAX_PRIORITIES = 5`,5 个等级足够用,排序如下:

| # | 线程                | 优先级 | Stack  | 周期      | 职责                         | 阻塞点 |
|---|---------------------|--------|--------|-----------|------------------------------|--------|
| 1 | `battery_monitor`   | **4**  | 256 B  | 1000 ms   | 电池电压/温度,过压欠压保护   | `vTaskDelayUntil(1000)` |
| 2 | `ble_tx`            | **3**  | 512 B  | 100 ms    | 蓝牙上行数据                 | `xQueueReceive(ble_q, 100ms)` |
| 3 | `ppg_reader`        | **3**  | 1024 B | 10 ms     | MAX30102 100 Hz 抽 FIFO,BPM/SpO2 | `vTaskDelayUntil(10)` + I²C sem |
| 4 | `env_sensors`       | **2**  | 768 B  | 1000 ms   | SHT40 + BMP280,露点/海拔     | `vTaskDelayUntil(1000)` + I²C sem |
| 5 | `screen_refresh`    | **2**  | 1024 B | 100 ms    | 更新显示帧                   | `vTaskDelayUntil(100)` + SPI sem |
| 6 | `uart_debug`        | **1**  | 512 B  | 500 ms    | 串口命令解析                 | `vTaskDelayUntil(500)` |
| 7 | `idle`              | **0**  | 128 B  | —         | FreeRTOS 内部                | 永远 ready |

**数字越低 = 越低**。p3 > p2 > p1 > p0。

#### 3.1 优先级为什么这样排

| 规则 | 解释 |
|------|------|
| `battery` 单占 p4 | 锂电池过放会失效/起火,**最先抢 CPU** |
| `ble_tx` + `ppg_reader` 共 p3 | 都要 10~100 ms 出数据,谁先 ready 谁先跑 |
| `env_sensors` + `screen_refresh` 共 p2 | 都允许百毫秒抖动,靠 `vTaskDelayUntil` 自然让出 |
| `uart_debug` 单独 p1 | 人在回路,500 ms 完全够用 |
| `idle` 单独 p0 | FreeRTOS 内部,业务不写在这里 |

#### 3.2 同优先级 (p2 = env + screen) 为什么不会饿死

两者都靠 `vTaskDelayUntil` 阻塞:
- `env_sensors` 1 s 触发一次,实际跑 ~30 ms(I²C 等中断时让出)
- `screen_refresh` 100 ms 触发一次,实际跑 ~20 ms(SPI DMA 等中断时让出)
- 100 ms 内两个 task 各自阻塞多次 → 互不饿死
- 偶发 sensor 阻塞 100 ms 时,屏幕仍能跑(同一时刻 I²C / SPI 总线只一个在用)

> 调试时如果发现 screen 卡顿,把 screen 提到 p3、env 留 p2;反之亦然。

### 4. 数据流:单写多读,无 mutex

```
                ┌────────────┐
                │ ppg_reader │──┐
                │  (写者)    │  │  共享: g_ppg_vitals
                └────────────┘  ├──────────────────┐
                                │                  ▼
┌────────────┐                  │           ┌──────────────┐
│env_sensors │──────────────────┘ 共享      │   ble_tx     │──→ 蓝牙
│  (写者)    │── g_env_data       g_*       │  screen_ref  │──→ 显示
└────────────┘                             │  (多读者)    │
                                           └──────────────┘
```

- **写者 1 个,不需要互斥**
- **读者只读**,32-bit 对齐读不会撕裂(Cortex-M33 单指令读字)
- 不靠 `volatile` 之外的锁
- 跨线程事件通知用 `xTaskNotify` / `xTaskNotifyFromISR`

### 5. 03_ 项目现状与目标差距

| 组件 | 03_ 现在 | 目标 | 改动 |
|------|---------|------|------|
| 线程数 | 2 | 6+ | 拆 ppg / env / 未来加 ble / screen / battery |
| `ppg_reader` | 在 `i2c_sensor` 里 (p1) | 拆出 p3 | 新建 `ppg_thread.c`,频率 10 ms |
| `env_sensors` | 与 ppg 混在 `i2c_sensor` (p1) | 单独 p2 | 改名 `env_thread.c`,频率 1000 ms |
| `uart_debug` | p2 | p1 | 降一级,人不急 |
| `i2c_sensor` | p1 | 拆走,删除 | 删文件 |
| `battery_monitor` | 无 | p4 | 后续加 |
| `ble_tx` | 无 | p3 | 后续加 |
| `screen_refresh` | 无 | p2 | 后续加(在 02_ 项目里已实现) |

### 6. 关键开关建议

| 开关 | 建议 | 原因 |
|------|------|------|
| `configUSE_MUTEXES` | **0 → 1** | 真要共享保护(比如后期多 task 写一个屏幕缓冲),mutex 必须开 |
| `configUSE_RECURSIVE_MUTEXES` | 0 | 避免递归锁 |
| `configCHECK_FOR_STACK_OVERFLOW` | **0 → 1** | 调试期开 `1` (方法 1) 或 `2` (方法 2),定位栈溢出 |
| `configUSE_TRACE_FACILITY` | 0 → 1 | 配 `vTaskList()` / `runtime_stats` 看 CPU 占用,非时间片调度更需要它 |
| `configGENERATE_RUN_TIME_STATS` | 0 → 1 | 配 `tracealyzer` / `uxTaskGetSystemState` 调试调度 |
| `configUSE_TICKLESS_IDLE` | 0 | 手环低功耗必须开,但需要重新校准 `SysTick` 和 `I2C` 唤醒时序 |
| `configMAX_PRIORITIES` | 5 → 7 | 给未来留 1~2 级,目前 5 也够 |

### 7. 一些容易踩的坑(非时间片专属)

| 现象 | 根因 | 解法 |
|------|------|------|
| 串口收不到命令 | `uart_debug` 同优先级比 sensor 低,sensor 的 I²C 等 ISR 时不让出 | `uart_debug` 提一级,或 sensor 拆帧 |
| 屏幕偶尔撕裂 | `screen_refresh` 和 `env_sensors` 同 p2,数据没对齐就读 | 把共享数据加 `__packed` / 临界段,或单写多读 |
| BLE 卡死 | `ble_tx` 等通知,ISR 没给 | 检查 `xTaskNotifyFromISR` 是否在 ISR 安全等级 |
| `printf` 让一切变慢 | newlib-nano 浮点格式化 + UART 阻塞 | 改 `vprintf` / 用 `xRingbuffer` |
| 任务"看起来卡了" | 同优先级独占,另一 task 在等 | 加 `taskYIELD()` 或拆优先级 |

### 8. 小结

> **非时间片抢占 = 一切调度决策都看优先级 + 阻塞点。**
> 设计 task 时,先回答 3 个问题:
> 1. **阻塞在哪?** 没有就加 `vTaskDelay(1)` 兜底
> 2. **截止时间多紧?** 紧的优先级高
> 3. **谁和我同优先级?** 确认有自然阻塞点,否则拆开

具体到本项目,优先级排布:
```
  4   battery_monitor     (安全)
  3   ble_tx, ppg_reader  (10~100 ms 实时)
  2   env_sensors, screen (百毫秒级)
  1   uart_debug          (半秒)
  0   idle                (兜底)
```

---