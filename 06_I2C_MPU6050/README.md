# MPU6050 姿态传感器项目

## 1. 项目概述

MPU6050 是一款集成三轴陀螺仪和三轴加速度计的六轴姿态传感器。本项目使用 Renesas RA6M5 芯片通过 I2C 通信获取传感器数据，并利用卡尔曼滤波算法进行姿态解算，最终输出稳定的 Roll（横滚角）和 Pitch（俯仰角）。

### 硬件连接

| MPU6050 | RA6M5 |
|---------|-------|
| VCC | 3.3V |
| GND | GND |
| SCL | P408 (I2C0 SCL) |
| SDA | P407 (I2C0 SDA) |
| AD0 | GND (地址 0x68) |

### 传感器特性

- 陀螺仪量程：±250, ±500, ±1000, ±2000 dps
- 加速度计量程：±2g, ±4g, ±8g, ±16g
- ADC 分辨率：16 位
- I2C 地址：0x68（AD0 接地）

## 2. 文件结构

```
src/
├── hal_entry.c           # 入口函数（主循环）
├── hal_warmstart.c       # 暖启动函数
├── UART_debug/           # 串口调试模块
│   ├── uart_debug.c
│   └── uart_debug.h
├── I2C_MPU6050/          # MPU6050 传感器模块
│   ├── mpu6050.c         # MPU6050 驱动（寄存器读写）
│   ├── mpu6050.h         # MPU6050 驱动头文件
│   ├── kalman.c          # 卡尔曼滤波 + 姿态解算
│   └── kalman.h          # 卡尔曼滤波头文件
└── sys_time/             # 系统时间模块
    ├── sys_time.h
    └── sys_time.c
```

## 3. 模块设计

### 3.1 分层概述
```
┌─────────────────────────┐
│   hal_entry.c           │  应用层（主循环调用姿态API）
├─────────────────────────┤
│   kalman.c              │  姿态解算层（卡尔曼滤波 + 姿态融合）
├─────────────────────────┤
│   mpu6050.c             │  传感器驱动层（I2C通信 + 寄存器操作）
└─────────────────────────┘
```

### 3.2 各层职责

| 层级 | 文件 | 职责 |
|------|------|------|
| 应用层 | hal_entry.c | 主循环调用姿态 API，输出角度 |
| 姿态解算层 | kalman.c | 卡尔曼滤波融合陀螺仪和加速度计数据，返回稳定角度 |
| 传感器驱动层 | mpu6050.c | I2C 通信，读写 MPU6050 寄存器 |

## 4. API 参考

### 4.1 MPU6050 驱动 API

```c
// 初始化
uint8_t MPU_Init(void);                          // 初始化MPU6050

// 配置
uint8_t MPU_Set_Gyro_Fsr(uint8_t fsr);           // 设置陀螺仪量程 (0-3)
uint8_t MPU_Set_Accel_Fsr(uint8_t fsr);          // 设置加速度计量程 (0-3)
uint8_t MPU_Set_LPF(uint16_t lpf);               // 设置低通滤波器频率 (Hz)
uint8_t MPU_Set_Rate(uint16_t rate);             // 设置采样率 (4-1000 Hz)

// 数据读取
short MPU_Get_Temperature(void);                 // 获取温度（返回值扩大100倍）
uint8_t MPU_Get_Gyroscope(short *gx, short *gy, short *gz);    // 获取陀螺仪原始数据
uint8_t MPU_Get_Accelerometer(short *ax, short *ay, short *az); // 获取加速度原始数据

// 寄存器操作
uint8_t MPU_Write_Byte(uint8_t reg, uint8_t data);                          // 写单个寄存器
uint8_t MPU_Read_Byte(uint8_t reg);                                         // 读单个寄存器
uint8_t MPU_Write_Len(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf); // 连续写
uint8_t MPU_Read_Len(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf);   // 连续读
```

### 4.2 姿态解算 API

```c
// 初始化（内部调用 MPU_Init 和卡尔曼滤波初始化）
uint8_t Attitude_Init(void);

// 姿态更新（主循环定时调用）
void Attitude_Update(float dt);  // dt: 时间间隔（秒），通常 0.005 (5ms)

// 获取姿态角
void Attitude_Get(float *roll, float *pitch);  // 输出弧度，需乘57.29578转为角度
```

## 5. 使用示例

### 5.1 基础用法
```c
void hal_entry(void)
{
    UART_debug_Init();
    Attitude_Init();  // 初始化传感器

    const float dt = 0.005f;  // 5ms 周期

    while (1)
    {
        Attitude_Update(dt);

        float roll, pitch;
        Attitude_Get(&roll, &pitch);

        // roll 和 pitch 是弧度，转角度：
        // float roll_deg = roll * 57.29578f;
        // float pitch_deg = pitch * 57.29578f;

        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    }
}
```

### 5.2 获取原始数据
```c
short ax, ay, az;  // 加速度
short gx, gy, gz;  // 陀螺仪

MPU_Get_Accelerometer(&ax, &ay, &az);
MPU_Get_Gyroscope(&gx, &gy, &gz);
short temp = MPU_Get_Temperature();  // 温度（已扩大100倍）
```

## 6. 命名规范

### 6.1 文件命名
- 模块名统一使用 **小写** 或 **PascalCase**
- `.c` 和 `.h` 文件名相同
- 示例：`uart_debug.c` / `uart_debug.h`、`mpu6050.c` / `mpu6050.h`

### 6.2 函数命名
- 使用 **Module_Function** 格式（前缀+功能名）
- 前缀与模块名一致
- 示例：`UART_debug_Init`、`MPU_Get_Gyroscope`、`Attitude_Update`

### 6.3 函数后缀
| 后缀 | 含义 | 示例 |
|------|------|------|
| `Init` | 初始化 | `Attitude_Init` |
| `Set` | 设置 | `MPU_Set_Gyro_Fsr` |
| `Get` | 获取 | `MPU_Get_Gyroscope` |
| `Update` | 更新 | `Attitude_Update` |

### 6.4 变量命名
- 全局变量：小写 + 下划线分隔
- 静态变量：同全局变量规则，添加 `static` 修饰
- 常量宏：全大写 + 下划线分隔

```c
#define MPU_ADDR                      0X68           // 常量
volatile bool i2c_receive_complete_flag;            // 全局标志
static kalman_filter_t kf_roll;                      // 静态变量
```

## 7. 代码注释规范

### 7.1 函数说明注释
```c
/**********************************************
函数名称：MPU_Init
函数功能：初始化MPU6050
函数参数：无
函数返回值：0,初始化成功  其他,初始化失败
**********************************************/
```

### 7.2 代码行注释
```c
if (ch == ':')  // 命令开始，清空缓冲区
{
    ...
}
```

## 8. 错误处理

- FSP API 返回 `fsp_err_t` 类型
- 初始化函数返回错误码：`0` 成功，`1` 失败

## 9. 寄存器定义 (mpu6050.h)

| 寄存器 | 地址 | 说明 |
|--------|------|------|
| MPU_PWR_MGMT1_REG | 0x6B | 电源管理 |
| MPU_GYRO_CFG_REG | 0x1B | 陀螺仪配置 |
| MPU_ACCEL_CFG_REG | 0x1C | 加速度计配置 |
| MPU_SAMPLE_RATE_REG | 0x19 | 采样率 |
| MPU_CFG_REG | 0x1A | 配置（低通滤波器） |
| MPU_ACCEL_XOUTH_REG | 0x3B | 加速度X轴高字节 |
| MPU_GYRO_XOUTH_REG | 0x43 | 陀螺仪X轴高字节 |
| MPU_TEMP_OUTH_REG | 0x41 | 温度高字节 |
| MPU_DEVICE_ID_REG | 0x75 | 设备ID |

## 10. 快速参考

```c
// 初始化
Attitude_Init();

// 主循环更新
Attitude_Update(0.005f);
float roll, pitch;
Attitude_Get(&roll, &pitch);
int roll_deg = (int)(roll * 57.29578f);
int pitch_deg = (int)(pitch * 57.29578f);
```