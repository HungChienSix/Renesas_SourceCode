# ST7735 屏幕驱动项目

## 1. 文件结构

```
src/
├── config.h              # 全局配置（开关 + 硬件宏）
├── config.c              # 总初始化函数
├── hal_entry.c           # 入口函数
├── hal_warmstart.c       # 暖启动函数
├── UART_debug/           # 串口调试模块
│   ├── uart_debug.c
│   └── uart_debug.h
├── Screen/               # 屏幕驱动模块
│   ├── screen_port.h     # 平台移植层接口定义
│   ├── screen_port.c     # Renesas 平台实现
│   ├── st7735.h          # ST7735 控制器驱动头文件
│   ├── st7735.c          # ST7735 控制器驱动实现
│   ├── screen.h          # 高级绘图API头文件
│   ├── screen.c          # 高级绘图API实现
│   ├── screen_ui.h       # UI组件头文件
│   ├── screen_ui.c       # UI组件实现
│   ├── fonts.h           # 字体头文件
│   └── fonts.c           # 字体数据
└── sys_time/             # 系统时间模块
    ├── sys_time.h
    └── sys_time.c
```

## 2. 配置管理 (config.h)

### 2.1 模块使能开关
```c
/* ========== 屏幕模块配置 ========== */
#define SCREEN_ENABLE        1    // 1: 开启, 0: 关闭

/* ========== 系统时间模块配置 ========== */
#define SYS_TIME_ENABLE      1    // 1: 开启, 0: 关闭

/* ========== 调试串口配置 ========== */
#define DEBUG_UART_ENABLE     1    // 1: 开启, 0: 关闭
```

### 2.2 硬件配置宏
```c
/* ========== 屏幕SPI硬件配置 ========== */
#define SCREEN_SPI_CTRL       g_spi0_ctrl
#define SCREEN_SPI_CFG        g_spi0_cfg

/* ========== 屏幕GPIO引脚配置 ========== */
#define TFT_RES_Pin    (BSP_IO_PORT_03_PIN_11)  // P311
#define TFT_DC_Pin     (BSP_IO_PORT_03_PIN_12)  // P312
#define TFT_CS_Pin     (BSP_IO_PORT_09_PIN_07)  // P907

/* ========== 调试串口硬件配置 ========== */
#define UART_DEBUG_CTRL       g_uart0_ctrl
#define UART_DEBUG_CFG        g_uart0_cfg
```

### 2.3 总初始化函数
```c
void Init_All(void);  // 初始化所有已开启的外设
```

## 3. 总初始化模式

外设通过 `Init_All()` 集中初始化，根据开关决定是否启用：

```c
/* config.c */
void Init_All(void)
{
#if DEBUG_UART_ENABLE
    UART_debug_Init();
#endif

#if SYS_TIME_ENABLE
    SysTime_Init();
#endif

#if SCREEN_ENABLE
    ST7735_Hardware_Init();
    ST7735_Init();
#endif
}
```

主函数调用：
```c
void hal_entry(void)
{
    Init_All();  // 自动初始化所有已开启的外设

    while(1) {
        // 主循环
    }
}
```

## 4. 模块设计规范

### 4.1 模块文件命名
- 模块名统一使用 **小写** 或 **PascalCase**
- `.c` 和 `.h` 文件名相同
- 示例：`uart_debug.c` / `uart_debug.h`

### 4.2 头文件结构
```c
/* 模块名 版本 */
#ifndef MODULE_NAME_H
#define MODULE_NAME_H

/* 1. 头文件引用 */
#include "config.h"

/* 2. 公共 API 声明（给外部调用） */
fsp_err_t Module_Init(void);
fsp_err_t Module_DeInit(void);

/* 3. 内部实现或弱函数（可选） */
__attribute__((weak)) int _isatty(int fd);

#endif /* MODULE_NAME_H */
```

## 5. 函数命名规范

### 5.1 命名格式
- 使用 **Module_Function** 格式（前缀+功能名）
- 前缀与模块名一致
- 示例：`UART_debug_Init`、`SysTime_Get_ms`、`ST7735_DrawPixel`

### 5.2 函数类型后缀
| 后缀 | 含义 | 示例 |
|------|------|------|
| `Init` | 初始化 | `UART_debug_Init` |
| `DeInit` | 反初始化 | `UART_debug_DeInit` |
| `Set` | 设置 | `UART_debug_SetCallback` |
| `Get` | 获取 | `UART_debug_GetCmdBuffer` |
| `Clear` | 清除 | `UART_debug_ClearCmdBuffer` |
| `Has` | 查询状态 | `UART_debug_HasCommand` |

### 5.3 函数分类
```c
// 初始化/反初始化
fsp_err_t UART_debug_Init();      // 打开硬件并初始化
fsp_err_t UART_debug_DeInit();     // 关闭硬件

// 轮询式 API（主循环调用）
bool UART_debug_HasCommand(void);              // 检查是否有数据
const char* UART_debug_GetCmdBuffer(void);     // 获取数据
void UART_debug_ClearCmdBuffer(void);          // 清空缓冲区
```

## 6. 变量命名规范

### 6.1 全局变量
- 小写 + 下划线分隔
- 添加模块前缀避免冲突：`uart_receive_complete_flag`

### 6.2 静态变量
- 模块内部使用 `static` 修饰
- 命名与全局变量相同规则

### 6.3 常量宏
- 全大写 + 下划线分隔
- 添加模块前缀：`CMD_BUFFER_SIZE`、`ST7735_WIDTH`

```c
#define CMD_BUFFER_SIZE 64           // 常量
static char cmd_buffer[CMD_BUFFER_SIZE];  // 静态变量
volatile bool uart_receive_complete_flag; // 全局标志
```

## 7. 屏幕模块分层设计

### 7.1 分层概述
```
┌─────────────────┐
│   screen_ui.c   │  UI组件层（按钮、进度条、开关等）
├─────────────────┤
│   screen.c      │  绘图API层（直线、矩形、文字等）
├─────────────────┤
│   st7735.c      │  控制器驱动层（ST7735初始化、像素操作）
├─────────────────┤
│   screen_port.c │  平台移植层（SPI传输、GPIO操作、延时）
└─────────────────┘
```

### 7.2 平台移植接口 (screen_port.h)
```c
typedef struct screen_port {
    screen_spi_transfer_t  spi_transfer;    // SPI传输函数
    screen_gpio_write_t    gpio_write;      // GPIO写函数
    screen_delay_t         delay_ms;        // 毫秒延时
    screen_delay_t         delay_us;        // 微秒延时
} screen_port_t;

const screen_port_t * screen_port_get(void);
int screen_port_init(void);
```

### 7.3 移植到新平台
1. 实现 `screen_port.h` 中的接口函数
2. 在 `screen_port.c` 中填充 `screen_port_t` 结构体
3. `st7735.c` 及以上层无需修改

## 8. 命令协议

### 8.1 格式
```
:[命令内容]\
```

### 8.2 命令处理
```c
void Get_Command(void) {
    if (UART_debug_HasCommand()) {
        const char *cmd = UART_debug_GetCmdBuffer();

        /* 跳过 ':' 字符，比较命令内容 */
        if (memcmp(cmd + 1, "screen_printf_disable", 22) == 0) {
            printf_screen = false;
        }
        else if (memcmp(cmd + 1, "screen_printf_able", 19) == 0) {
            printf_screen = true;
        }

        UART_debug_ClearCmdBuffer();
    }
}
```

## 9. 代码注释规范

### 9.1 文件头注释
```c
/* V1.0 Module_Name */
```

### 9.2 函数说明注释
```c
/* 获取命令缓冲区 */
const char* UART_debug_GetCmdBuffer(void);
```

### 9.3 代码行注释
```c
if (ch == ':')  // 命令开始，清空缓冲区
{
    ...
}
```

## 10. 错误处理

- FSP API 返回 `fsp_err_t` 类型
- 初始化函数返回错误码
- 示例：`fsp_err_t UART_debug_Init()`

## 11. 轮询 vs 回调

| 模式 | 适用场景 | API 设计 |
|------|----------|----------|
| 轮询 | 主循环架构，简单场景 | `Has*()` 查询状态 |
| 回调 | 实时响应，RTOS | `Set*Callback()` 注册 |

本项目采用**轮询模式**：
```c
while(1){
    Get_Command();
}
```

## 12. 快速参考

### 屏幕初始化
```c
Init_All();  // 总初始化，包含屏幕
```

### 屏幕操作
```c
ST7735_FillScreen(SCREEN_BLACK);           // 清屏
ST7735_DrawPixel(x, y, color);            // 画点
SCREEN_DrawString(x, y, str, font, color, type);  // 绘制字符串
ST7735_RefreshScreen();                    // 刷新屏幕
```

### UI组件测试
```c
SCREEN_DrawUITest(0);  // 测试Button
SCREEN_DrawUITest(1);  // 测试Tooltip
SCREEN_DrawUITest(2);  // 测试ProgressBar
SCREEN_DrawUITest(3);  // 测试Switch
SCREEN_DrawUITest(4);  // 测试Slider
SCREEN_DrawUITest(5);  // 测试ListItem
```

### 时间获取
```c
SysTime_Get_ms();    // 获取毫秒
SysTime_Get_us();    // 获取微秒
SysTime_Elapsed_us(start, end);  // 计算时间差
```

## 13. UI组件

### 13.1 组件列表

| 组件 | 结构体 | 说明 |
|------|--------|------|
| Button | `struUI_Button_t` | 带边框/填充/文本的圆角矩形按钮 |
| Tooltip | `struUI_Tooltip_t` | 带阴影的文本提示框 |
| ProgressBar | `struUI_ProgressBar_t` | 进度条（0-100%） |
| Switch | `struUI_Switch_t` | 拨动开关（开启/关闭） |
| Slider | `struUI_Slider_t` | 滑动条（显示比例） |
| ListItem | `struUI_ListItem_t` | 列表项（支持选中状态） |

### 13.2 使用示例

```c
/* Button示例 */
struUI_Button_t btn;
btn.location[0] = 64;
btn.location[1] = 30;
btn.frame[0] = 60;   // 宽度
btn.frame[1] = 24;  // 高度
btn.frame[2] = 4;    // 圆角半径
strcpy(btn.label, "OK");
btn.ascii_font = &Font_8x16_consola;
btn.hz_font = &Font_UTF_16x16_YuMincho;
btn.color[0] = 0x07E0;  // 边框绿色
btn.color[1] = 0x03E0;  // 填充深绿
btn.color[2] = 0x0000;  // 文本黑色
btn.state = 0x00;
SCREEN_DrawButton(&btn);

/* ProgressBar示例 */
struUI_ProgressBar_t bar;
bar.location[0] = 64;
bar.location[1] = 30;
bar.frame[0] = 100;
bar.frame[1] = 16;
bar.color[0] = 0xFFFF;  // 边框白色
bar.color[1] = 0x07E0;  // 填充绿色
bar.progress = 66;
SCREEN_DrawProgressBar(&bar);

/* Switch示例 */
struUI_Switch_t sw;
sw.location[0] = 64;
sw.location[1] = 30;
sw.width = 50;
sw.height = 26;
sw.track_color = 0x6E6E;
sw.thumb_color = 0xFFFF;
sw.value = true;
SCREEN_DrawSwitch(&sw);
```