# Screen 绘图库 API 使用指南

> 适用于 ST7735 128x128 SPI LCD，基于 Renesas RA 系列 MCU

---

## 目录

1. [基本概念](#1-基本概念)
2. [初始化](#2-初始化)
3. [颜色系统](#3-颜色系统)
4. [基础绘图](#4-基础绘图)
5. [文本绘制](#5-文本绘制)
6. [图片绘制](#6-图片绘制)
7. [屏幕管理](#7-屏幕管理)
8. [UI 组件](#8-ui-组件)
9. [字体列表](#9-字体列表)
10. [返回值说明](#10-返回值说明)
11. [完整示例](#11-完整示例)

---

## 1. 基本概念

### 坐标系

```
(0,0) ─────────────────── (127,0)
  │                           │
  │        128 x 128          │
  │        LCD 画布            │
  │                           │
(0,127) ────────────────── (127,127)
```

- 原点 (0,0) 在左上角，X 向右增大，Y 向下增大
- 有效范围：X: 0~127，Y: 0~127

### 帧缓冲架构

所有绘图操作先写入内存中的帧缓冲区 (`display_ram[128][128]`)，不会立即显示到屏幕。需要调用 `SCREEN_RefreshScreen()` 将变化的数据通过 SPI 发送到 LCD。

### 绘制模式

```c
typedef enum SCREEN_Mode {
    SCREEN_Nor = 0x00,  // 正常模式：新颜色覆盖旧颜色
    SCREEN_Xor,         // 异或模式：目标像素与绘制颜色相同时反色
} SCREEN_Mode_t;
```

`SCREEN_Xor` 常用于高亮/取消高亮：用相同颜色再画一次即可恢复原色。

### 头文件引用

```c
#include "st7735.h"      // ST7735 控制器驱动
#include "screen.h"      // 绘图 API
#include "screen_ui.h"   // UI 组件 API
#include "fonts.h"       // 字体定义和资源声明
```

---

## 2. 初始化

```c
fsp_err_t ST7735_Hardware_Init(void);  // SPI硬件初始化
fsp_err_t ST7735_Init(void);           // ST7735控制器初始化
void ST7735_Reset(void);               // 复位ST7735
```

**初始化顺序：**
1. `ST7735_Hardware_Init()` - 初始化 SPI 接口
2. `ST7735_Init()` - 复位 ST7735、配置寄存器、清屏

**必须在所有绘图函数之前调用。**

```c
ST7735_Hardware_Init();
ST7735_Init();
// 此后可以开始绘图
```

---

## 3. 颜色系统

### RGB565 格式

16 位颜色，R 占 5 位、G 占 6 位、B 占 5 位：

```
 Bit: 15 14 13 12 11 | 10  9  8  7  6  5 |  4  3  2  1  0
      R4 R3 R2 R1 R0 | G5 G4 G3 G2 G1 G0 | B4 B3 B2 B1 B0
```

### 预定义颜色

| 宏名 | 值 | 颜色 |
|------|-----|------|
| `SCREEN_BLACK` | `0x0000` | 黑色 |
| `SCREEN_RED` | `0xF800` | 红色 |
| `SCREEN_GREEN` | `0x07E0` | 绿色 |
| `SCREEN_BLUE` | `0x001F` | 蓝色 |
| `SCREEN_YELLOW` | `0xFFE0` | 黄色 |
| `SCREEN_MAGENTA` | `0xF81F` | 品红 |
| `SCREEN_CYAN` | `0x07FF` | 青色 |
| `SCREEN_WHITE` | `0xFFFF` | 白色 |

### 自定义颜色

```c
// 从 RGB888 (0-255) 转换为 RGB565
ST7735_Pixel_t orange = SCREEN_COLOR565(255, 165, 0);   // 橙色
ST7735_Pixel_t gray   = SCREEN_COLOR565(128, 128, 128); // 灰色
```

宏定义：
```c
#define SCREEN_COLOR565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))
```

---

## 4. 基础绘图

### 4.1 画点

```c
SCREEN_Event_t SCREEN_DrawPixel(int16_t x, int16_t y, ST7735_Pixel_t color, SCREEN_Mode_t mode);
```

| 参数 | 说明 |
|------|------|
| `x, y` | 坐标 (0~127) |
| `color` | RGB565 颜色 |
| `mode` | `SCREEN_Nor` 或 `SCREEN_Xor` |

返回值：`SCREEN_OK` / `SCREEN_OUT`（超出边界）

### 4.2 画直线

```c
SCREEN_Event_t SCREEN_DrawLine(int16_t x0, int16_t x1, int16_t y0, int16_t y1,
                                ST7735_Pixel_t color, SCREEN_Mode_t mode);
```

使用 Bresenham 算法，支持任意方向。参数为两个端点坐标 `(x0,y0)` 和 `(x1,y1)`。

> **注意**：参数名 `x0,x1` 是水平坐标，`y0,y1` 是垂直坐标，不要混淆。

```c
// 画一条对角线
SCREEN_DrawLine(0, 127, 0, 127, SCREEN_RED, SCREEN_Nor);
```

### 4.3 画矩形

```c
// 实心矩形
SCREEN_Event_t SCREEN_DrawRectSolid(int16_t x0, int16_t x1, int16_t y0, int16_t y1,
                                     ST7735_Pixel_t color, SCREEN_Mode_t mode);
// 空心矩形
SCREEN_Event_t SCREEN_DrawRectHollow(int16_t x0, int16_t x1, int16_t y0, int16_t y1,
                                      ST7735_Pixel_t color, SCREEN_Mode_t mode);
```

| 参数 | 说明 |
|------|------|
| `x0, x1` | 水平起止坐标（自动排序，无需管大小） |
| `y0, y1` | 垂直起止坐标（自动排序） |

```c
// 蓝色实心矩形
SCREEN_DrawRectSolid(10, 50, 20, 60, SCREEN_BLUE, SCREEN_Nor);
// 白色空心矩形边框
SCREEN_DrawRectHollow(10, 50, 20, 60, SCREEN_WHITE, SCREEN_Nor);
```

### 4.4 画圆角矩形

```c
// 实心圆角矩形
SCREEN_Event_t SCREEN_DrawRoundRectSolid(int16_t x0, int16_t x1, int16_t y0, int16_t y1,
                                          uint8_t radius, ST7735_Pixel_t color, SCREEN_Mode_t mode);
// 空心圆角矩形
SCREEN_Event_t SCREEN_DrawRoundRectHollow(int16_t x0, int16_t x1, int16_t y0, int16_t y1,
                                           uint8_t radius, ST7735_Pixel_t color, SCREEN_Mode_t mode);
```

| 参数 | 说明 |
|------|------|
| `radius` | 圆角半径，最大 64 (`SCREEN_MAX_ROUND_RADIUS`)，超出自动裁剪 |
| 其余 | 同普通矩形 |

```c
// 绿色实心圆角矩形，圆角半径5
SCREEN_DrawRoundRectSolid(10, 70, 30, 50, 5, SCREEN_GREEN, SCREEN_Nor);
```

### 4.5 画圆弧和扇形

```c
// 圆弧（轮廓线）
SCREEN_Event_t SCREEN_DrawQuarArc(int16_t cx, int16_t cy, uint16_t r,
                                   uint8_t quadrant_mask, ST7735_Pixel_t color, SCREEN_Mode_t mode);
// 扇形（填充）
SCREEN_Event_t SCREEN_DrawQuarSector(int16_t cx, int16_t cy, uint16_t r,
                                      uint8_t quadrant_mask, ST7735_Pixel_t color, SCREEN_Mode_t mode);
```

| 参数 | 说明 |
|------|------|
| `cx, cy` | 圆心坐标 |
| `r` | 半径 |
| `quadrant_mask` | 象限掩码，可组合 |

**象限定义：**

| 宏名 | 值 | 方向 |
|------|-----|------|
| `SCREEN_Quarter1` | `0x01` | 右上 |
| `SCREEN_Quarter2` | `0x02` | 左上 |
| `SCREEN_Quarter3` | `0x04` | 左下 |
| `SCREEN_Quarter4` | `0x08` | 右下 |

```c
// 画一个完整的圆（四个象限组合）
SCREEN_DrawQuarArc(64, 64, 30, 0x01|0x02|0x04|0x08, SCREEN_WHITE, SCREEN_Nor);

// 画上半圆扇形
SCREEN_DrawQuarSector(64, 64, 30, 0x01|0x02, SCREEN_CYAN, SCREEN_Nor);

// 画右上四分之一圆弧
SCREEN_DrawQuarArc(64, 64, 20, SCREEN_Quarter1, SCREEN_YELLOW, SCREEN_Nor);
```

### 4.6 绘制函数原理与流程

#### 4.6.1 像素绘制流程

所有绘图函数遵循相同的底层流程：

```
用户调用绘图函数
       │
       ▼
┌──────────────────┐
│ 参数校验          │ ← 检查坐标、指针等是否有效
│ (x, y 在有效范围?) │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ 写入帧缓冲区      │ ← 写入 display_ram[y][x]
│ (根据 mode 计算)   │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ 标记脏区域        │ ← 记录变化的行范围
│ (row_min ~ row_max)│
└────────┬─────────┘
         │
         ▼
      返回结果
```

#### 4.6.2 绘制模式详解

```c
typedef enum SCREEN_Mode {
    SCREEN_Nor = 0x00,  // 正常模式：直接写入新颜色
    SCREEN_Xor,         // 异或模式：新颜色与原有颜色异或
} SCREEN_Mode_t;
```

**Normal 模式（正常覆盖）：**
- 直接将 `color` 写入像素位置
- `pixel = color`

**Xor 模式（异或模式）：**
- 将 `color` 与原有像素进行异或运算
- `pixel = pixel ^ color`
- 用于高亮/反显：相同颜色绘制两次会恢复原色

```c
// 高亮文字示例
SCREEN_DrawString(10, 10, "Hello", font, SCREEN_WHITE, SCREEN_Nor);   // 正常绘制
SCREEN_DrawString(10, 10, "Hello", font, SCREEN_WHITE, SCREEN_Xor);  // 相同位置异或 = 消除
```

#### 4.6.3 直线算法（Bresenham）

`SCREEN_DrawLine` 使用 Bresenham 算法绘制直线，避免浮点运算：

```
核心思想：每一步选择最近的像素点

起点 (x0, y0) → 终点 (x1, y1)

计算决策参数：
  d = (y1 - y0) * 2 - (x1 - x0)

每一步：
  if (d > 0) {
      画 (x, y)
      y++          // 向上移动
      d += 2 * (dy - dx)
  } else {
      x++          // 向右移动
      d += 2 * dy
  }
```

#### 4.6.4 圆弧算法（Bresenham）

`SCREEN_DrawQuarArc` 使用八分法圆弧：

```
1. 先画第一象限的 1/8 圆弧（Bresenham）
2. 利用对称性映射到其他 7 个 1/8 弧

对称关系：
  (x, y) → (-x, y) → (x, -y) → (-x, -y)
         → (y, x) → (-y, x) → (y, -x) → (-y, -x)
```

象限掩码控制只绘制特定部分：
- `0x01` (0001): 绘制象限1（右上）
- `0x02` (0010): 绘制象限2（左上）
- `0x04` (0100): 绘制象限3（左下）
- `0x08` (1000): 绘制象限4（右下）

#### 4.6.5 圆角矩形原理

圆角矩形 = 矩形主体 + 四个圆角

```
┌────────────────────────────┐
│    ┌──┐          ┌──┐    │  ← 顶部圆角
│    │  │          │  │    │
├────┘  └──────────┘  └────┤  ← 中间矩形
│    └──┐          ┌──┘    │  ← 底部圆角
│       └──────────┘       │
└────────────────────────────┘
```

**绘制步骤：**
1. 填充矩形主体（排除四个角区域）
2. 在四个角绘制 1/4 圆弧扇形

#### 4.6.6 字符绘制原理

ASCII 字符绘制：
```
1. 从字体数据中定位字符（char_index = ch - ' '）
2. 定位字符数据位置：
   font_data += char_index * font->height * font->bytes_per_row
3. 逐行扫描字体位图
4. 每位为 1 时绘制对应颜色像素
```

汉字绘制类似，但使用 3 字节 UTF-8 编码索引汉字表。

#### 4.6.7 分区刷新原理

```
脏标记跟踪：
┌─────────────────────────────┐
│ row_min = 10                │
│ row_max = 50                │
│ col_min = 5                 │
│ col_max = 80                │
└─────────────────────────────┘

刷新时：
1. 设置列地址范围：col_min → col_max
2. 设置行地址范围：row_min → row_max
3. 只发送脏区域数据，而非全屏 32KB
4. 发送完毕后清除脏标记
```

---

## 5. 文本绘制

### 5.1 ASCII 字符/字符串

```c
SCREEN_Event_t SCREEN_DrawChar(int16_t x, int16_t y, char ch,
                                const struFont_t *font,
                                ST7735_Pixel_t color, SCREEN_Mode_t mode);

SCREEN_Event_t SCREEN_DrawString(int16_t x, int16_t y, const char *str,
                                  const struFont_t *font,
                                  ST7735_Pixel_t color, SCREEN_Mode_t mode);
```

| 参数 | 说明 |
|------|------|
| `x, y` | 字符左上角坐标 |
| `ch` | ASCII 字符 (32~126) |
| `str` | ASCII 字符串 |
| `font` | 字体指针 |

```c
// 显示字符串
SCREEN_DrawString(10, 20, "Hello!", &Font_8x12_consola, SCREEN_WHITE, SCREEN_Nor);

// 显示单个字符
SCREEN_DrawChar(0, 0, 'A', &Font_8x16_consola, SCREEN_YELLOW, SCREEN_Nor);
```

> **注意**：`SCREEN_DrawString` 超出屏幕宽度时截断并返回 `SCREEN_OUT`。

### 5.2 UTF-8 汉字

```c
SCREEN_Event_t SCREEN_DrawUTFChar(int16_t x, int16_t y, const char *utf8_char,
                                   const struFont_UTF_t *hz_font,
                                   ST7735_Pixel_t color, SCREEN_Mode_t mode);
```

`utf8_char` 指向 3 字节 UTF-8 编码的汉字。

```c
// 显示一个汉字
SCREEN_DrawUTFChar(10, 40, "\xe4\xbd\xa0", &Font_UTF_16x16_YuMincho, SCREEN_RED, SCREEN_Nor);
```

### 5.3 混合 ASCII 和汉字

```c
SCREEN_Event_t SCREEN_DrawUTFString(int16_t x, int16_t y, const char *utf8_str,
                                     const struFont_t *ascii_font,
                                     const struFont_UTF_t *hz_font,
                                     ST7735_Pixel_t color, SCREEN_Mode_t mode);
```

自动识别 ASCII（1字节）和 UTF-8 汉字（3字节），混合绘制。

```c
// 混合显示
SCREEN_DrawUTFString(0, 0, "Hello\xe4\xb8\x96\xe7\x95\x8c",
                      &Font_8x12_consola, &Font_UTF_16x16_YuMincho,
                      SCREEN_WHITE, SCREEN_Nor);
```

> 不在字体表中的汉字会被跳过，返回 `SCREEN_CHAR_EXCEED`。

---

## 6. 图片绘制

### 6.1 单色位图

```c
SCREEN_Event_t SCREEN_DrawImage(int16_t x, int16_t y, uint16_t width, uint16_t height,
                                 const uint8_t *image,
                                 ST7735_Pixel_t color, SCREEN_Mode_t mode);
```

- 1 位/像素，MSB 在前
- 每行字节数 = `(width + 7) / 8`
- 总大小 = `每行字节数 * height`

```c
// 显示 32x32 单色苹果图标（128字节）
SCREEN_DrawImage(48, 48, 32, 32, gImage_apple, SCREEN_GREEN, SCREEN_Nor);
```

### 6.2 RGB565 彩色图

```c
SCREEN_Event_t SCREEN_DrawRGBImage(int16_t x, int16_t y, uint16_t width, uint16_t height,
                                    const uint8_t *image);
```

- 每像素 2 字节（RGB565 小端序：低字节在前）
- 总大小 = `width * height * 2`

```c
// 显示 32x32 RGB 彩色图标（2048字节）
SCREEN_DrawRGBImage(0, 0, 32, 32, gImage_RGB_163music);
```

---

## 7. 屏幕管理

### 7.1 填充屏幕

```c
void SCREEN_FillScreen(ST7735_Pixel_t color);
```

用指定颜色填满 128x128 帧缓冲区。在分区刷新模式下，只标记实际变化的像素为脏。

### 7.2 刷新屏幕

```c
// 分区刷新（推荐）：只发送变化的像素，返回耗时(ms)
uint32_t SCREEN_RefreshScreen(void);

// 强制全量刷新：发送全部 32KB 数据
void SCREEN_RefreshScreen_Force(void);
```

**典型工作流：**

```c
// 1. 绘制内容到帧缓冲区
SCREEN_FillScreen(SCREEN_BLACK);
SCREEN_DrawString(10, 10, "Hello", &Font_8x12_consola, SCREEN_WHITE, SCREEN_Nor);

// 2. 刷新到 LCD
uint32_t ms = SCREEN_RefreshScreen();
printf("Refresh: %lu ms\n", ms);
```

### 7.3 分区刷新原理

启用 `ST7735_PARTIAL_REFRESH` 宏后（默认启用）：

1. 每次绘图操作自动标记变化的行和列范围
2. `SCREEN_RefreshScreen()` 只发送脏区域的数据
3. 发送后自动清除脏标记
4. 对于局部更新（如按钮状态切换），刷新速度可快 10 倍以上

---

## 8. UI 组件

### 8.1 按钮 (Button)

```c
typedef struct {
    int8_t       location[2];    // 中心坐标 {x, y}
    uint8_t      frame[3];       // {宽度, 高度, 圆角半径}
    char         label[32];      // 按钮文字（最多31字符）
    struFont_t  *ascii_font;     // ASCII 字体指针
    struFont_UTF_t *hz_font;     // 汉字字体指针（NULL 则不显示汉字）
    ST7735_Pixel_t color[3];     // {边框色, 填充色, 文字色}
    uint8_t      state;          // 0x00=未按下(空心)  0xFF=按下(实心)
} struUI_Button_t;

SCREEN_Event_t SCREEN_DrawButton(struUI_Button_t *button);
```

```c
struUI_Button_t btn = {
    .location   = {64, 100},
    .frame      = {40, 16, 3},
    .label      = "OK",
    .ascii_font = (struFont_t *)&Font_8x12_consola,
    .hz_font    = NULL,
    .color      = {SCREEN_GREEN, SCREEN_BLACK, SCREEN_YELLOW},
    .state      = 0x00,
};
SCREEN_DrawButton(&btn);
```

**视觉效果：**
- `state == 0x00`：空心圆角矩形边框 + 文字
- `state == 0xFF`：实心圆角矩形 + 文字

### 8.2 提示框 (Tooltip)

```c
typedef struct {
    int8_t       location[2];    // 中心坐标 {x, y}
    uint8_t      frame[2];       // {宽度, 高度}
    char         text[64];       // 提示文字（最多63字符）
    struFont_t  *ascii_font;     // ASCII 字体指针
    struFont_UTF_t *hz_font;     // 汉字字体指针
    ST7735_Pixel_t color[2];     // {背景色, 文字色}
} struUI_Tooltip_t;

SCREEN_Event_t SCREEN_DrawTooltip(struUI_Tooltip_t *tooltip);
```

```c
struUI_Tooltip_t tip = {
    .location   = {64, 60},
    .frame      = {60, 16},
    .text       = "Loading...",
    .ascii_font = (struFont_t *)&Font_8x12_consola,
    .hz_font    = NULL,
    .color      = {SCREEN_BLACK, SCREEN_WHITE},
};
SCREEN_DrawTooltip(&tip);
```

**视觉效果：** 带有 2px 深灰色投影阴影的矩形，文字垂直居中、左留 4px 边距。

### 8.3 进度条 (ProgressBar)

```c
typedef struct {
    int8_t       location[2];    // 中心坐标 {x, y}
    uint8_t      frame[2];       // {宽度, 高度}
    uint8_t      color[2];       // {边框色, 填充色}
    uint8_t      progress;       // 进度 0~100
} struUI_ProgressBar_t;

SCREEN_Event_t SCREEN_DrawProgressBar(struUI_ProgressBar_t *bar);
```

```c
struUI_ProgressBar_t bar = {
    .location = {64, 110},
    .frame    = {80, 10},
    .color    = {SCREEN_WHITE, SCREEN_GREEN},
    .progress = 65,
};
SCREEN_DrawProgressBar(&bar);
```

**视觉效果：** 圆角矩形边框（半径 = 高度/2），内部按百分比填充，留 1px 内边距。

### 8.4 开关 (Switch)

```c
typedef struct {
    int8_t       location[2];    // 中心坐标 {x, y}
    uint8_t      width;          // 开关宽度
    uint8_t      height;         // 开关高度（通常为宽度的一半）
    ST7735_Pixel_t track_color; // 轨道颜色（关闭状态）
    ST7735_Pixel_t thumb_color; // 滑块颜色
    bool         value;          // 当前状态 false=关 true=开
} struUI_Switch_t;

SCREEN_Event_t SCREEN_DrawSwitch(struUI_Switch_t *sw);
```

```c
struUI_Switch_t sw = {
    .location    = {64, 30},
    .width       = 50,
    .height      = 26,
    .track_color = 0x6E6E,      // 关闭时灰色
    .thumb_color = SCREEN_WHITE,
    .value       = false,
};
SCREEN_DrawSwitch(&sw);
```

**视觉效果：**
- `value == false`：灰色轨道（`track_color`），左侧白色圆形滑块
- `value == true`：绿色轨道，右侧白色圆形滑块

### 8.5 滑动条 (Slider)

```c
typedef struct {
    int8_t       location[2];    // 中心坐标 {x, y}
    uint8_t      width;          // 滑动条宽度
    uint8_t      height;         // 滑动条高度
    ST7735_Pixel_t track_color;   // 轨道颜色
    ST7735_Pixel_t thumb_color;    // 滑块颜色
    ST7735_Pixel_t progress_color;  // 已填充进度颜色
    int16_t      min_value;      // 最小值
    int16_t      max_value;      // 最大值
    int16_t      current_value;  // 当前值
} struUI_Slider_t;

SCREEN_Event_t SCREEN_DrawSlider(struUI_Slider_t *slider);
```

```c
struUI_Slider_t slider = {
    .location       = {64, 30},
    .width          = 100,
    .height         = 20,
    .track_color    = 0x6E6E,      // 轨道灰色
    .thumb_color    = SCREEN_WHITE, // 滑块白色
    .progress_color = SCREEN_GREEN, // 进度绿色
    .min_value      = 0,
    .max_value      = 100,
    .current_value  = 60,
};
SCREEN_DrawSlider(&slider);
```

**视觉效果：** 圆角矩形轨道 + 按比例填充的进度条 + 中央矩形滑块。

### 8.6 列表项 (ListItem)

```c
typedef struct {
    int8_t       location[2];    // 左上角坐标 {x, y}
    uint8_t      width;          // 列表项宽度
    uint8_t      height;          // 列表项高度
    char         text[64];       // 列表项文本
    struFont_t  *font;           // 字体指针
    ST7735_Pixel_t bg_color;     // 背景颜色
    ST7735_Pixel_t text_color;   // 文本颜色
    ST7735_Pixel_t border_color; // 边框颜色（可为透明）
    bool         selected;       // 是否被选中
    bool         show_border;    // 是否显示边框
} struUI_ListItem_t;

SCREEN_Event_t SCREEN_DrawListItem(struUI_ListItem_t *item);
```

```c
struUI_ListItem_t item = {
    .location    = {0, 10},
    .width       = 120,
    .height      = 22,
    .text        = "List Item 1",
    .font        = &Font_8x16_consola,
    .bg_color    = SCREEN_BLACK,
    .text_color  = SCREEN_WHITE,
    .border_color = 0x6E6E,
    .selected    = false,
    .show_border = true,
};
SCREEN_DrawListItem(&item);
```

**视觉效果：**
- 正常状态：实心背景 + 边框 + 文本
- 选中状态：左侧3像素指示条 + 文本反色（Xor模式）

### 8.7 UI组件测试

```c
SCREEN_Event_t SCREEN_DrawUITest(uint8_t index);
```

根据 index 测试不同组件：

| index | 组件 | 测试内容 |
|-------|------|---------|
| 0 | Button | 两个按钮（未按/按下状态） |
| 1 | Tooltip | 带阴影的文本提示框 |
| 2 | ProgressBar | 两个不同进度的进度条 |
| 3 | Switch | 两个开关（关闭/开启状态） |
| 4 | Slider | 两个不同值的滑块 |
| 5 | ListItem | 三个列表项（含选中状态） |

```c
SCREEN_DrawUITest(0);  // 测试Button
SCREEN_DrawUITest(5);  // 测试ListItem
```

---

## 9. 字体列表

### ASCII 字体

| 变量名 | 尺寸 | 说明 |
|--------|------|------|
| `Font_8x16_consola` | 8 x 16 | 等宽字体，适合代码/数据 |
| `Font_8x12_consola` | 8 x 12 | 等宽字体，紧凑 |
| `Font_8x16_times` | 8 x 16 | 衬线字体 |
| `Font_8x12_times` | 8 x 12 | 衬线字体，紧凑 |

### UTF-8 汉字字体

| 变量名 | 尺寸 | 说明 |
|--------|------|------|
| `Font_UTF_16x16_YuMincho` | 16 x 16 | 明朝体汉字 |
| `Font_UTF_16x12_YuMincho` | 16 x 12 | 明朝体汉字，紧凑 |

### 字符宽度计算

```c
// ASCII 字符宽度 = font.width（8px）
// 汉字字符宽度 = 16px
// 字符串总宽度 = ASCII数 * 8 + 汉字数 * 16
```

---

## 10. 返回值说明

```c
typedef enum SCREEN_Event {
    SCREEN_OK          = 0,   // 成功
    SCREEN_OUT,               // 坐标超出屏幕范围
    SCREEN_PARAM_ERROR,       // 参数错误（如 NULL 指针）
    SCREEN_CHAR_EXCEED,       // 字符超出范围（无效字符/汉字未收录）
} SCREEN_Event_t;
```

所有绘图函数均返回 `SCREEN_Event_t`。建议在关键位置检查返回值。

---

## 11. 完整示例

```c
#include "screen.h"
#include "screen_ui.h"

void hal_entry(void) {
    /* 初始化 */
    ST7735_Hardware_Init();
    ST7735_Init();

    /* 黑色背景 */
    SCREEN_FillScreen(SCREEN_BLACK);

    /* 标题文字 */
    SCREEN_DrawString(8, 4, "LCD Demo", &Font_8x12_consola, SCREEN_CYAN, SCREEN_Nor);

    /* 红色对角线 */
    SCREEN_DrawLine(0, 127, 0, 127, SCREEN_RED, SCREEN_Nor);

    /* 绿色实心矩形 */
    SCREEN_DrawRectSolid(5, 40, 40, 70, SCREEN_GREEN, SCREEN_Nor);

    /* 白色空心圆角矩形 */
    SCREEN_DrawRoundRectHollow(50, 120, 20, 50, 8, SCREEN_WHITE, SCREEN_Nor);

    /* 圆形 */
    SCREEN_DrawQuarArc(90, 90, 20,
                       SCREEN_Quarter1 | SCREEN_Quarter2 | SCREEN_Quarter3 | SCREEN_Quarter4,
                       SCREEN_YELLOW, SCREEN_Nor);

    /* 按钮 */
    struUI_Button_t btn = {
        .location   = {64, 90},
        .frame      = {50, 14, 4},
        .label      = "Press",
        .ascii_font = (struFont_t *)&Font_8x12_consola,
        .hz_font    = NULL,
        .color      = {SCREEN_WHITE, SCREEN_BLUE, SCREEN_WHITE},
        .state      = 0x00,
    };
    SCREEN_DrawButton(&btn);

    /* 进度条 */
    struUI_ProgressBar_t bar = {
        .location = {64, 115},
        .frame    = {100, 8},
        .color    = {SCREEN_WHITE, SCREEN_GREEN},
        .progress = 60,
    };
    SCREEN_DrawProgressBar(&bar);

    /* 刷新到屏幕 */
    uint32_t ms = SCREEN_RefreshScreen();
    printf("Refresh: %lu ms\n", ms);

    while (1) {
        /* 主循环中按需更新 UI */
    }
}
```

---

## 附：函数速查表

| 类别 | 函数 | 说明 |
|------|------|------|
| 初始化 | `ST7735_Hardware_Init()` / `ST7735_Init()` | 初始化屏幕 |
| 画点 | `SCREEN_DrawPixel()` | 绘制单个像素 |
| 画线 | `SCREEN_DrawLine()` | Bresenham 直线 |
| 实心矩形 | `SCREEN_DrawRectSolid()` | 填充矩形 |
| 空心矩形 | `SCREEN_DrawRectHollow()` | 矩形边框 |
| 实心圆角矩形 | `SCREEN_DrawRoundRectSolid()` | 圆角填充矩形 |
| 空心圆角矩形 | `SCREEN_DrawRoundRectHollow()` | 圆角矩形边框 |
| 圆弧 | `SCREEN_DrawQuarArc()` | 四分之一圆弧轮廓 |
| 扇形 | `SCREEN_DrawQuarSector()` | 四分之一扇形填充 |
| ASCII字符 | `SCREEN_DrawChar()` | 绘制单个 ASCII 字符 |
| ASCII字符串 | `SCREEN_DrawString()` | 绘制 ASCII 字符串 |
| 汉字 | `SCREEN_DrawUTFChar()` | 绘制单个 UTF-8 汉字 |
| 混合字符串 | `SCREEN_DrawUTFString()` | ASCII + 汉字混合字符串 |
| 单色图 | `SCREEN_DrawImage()` | 1位/像素位图 |
| 彩色图 | `SCREEN_DrawRGBImage()` | RGB565 彩色图 |
| 填充 | `SCREEN_FillScreen()` | 填满整个屏幕 |
| 刷新 | `SCREEN_RefreshScreen()` | 分区刷新（推荐） |
| 全刷 | `SCREEN_RefreshScreen_Force()` | 强制全量刷新 |
| 开关 | `SCREEN_DrawSwitch()` | UI 开关 |
| 滑动条 | `SCREEN_DrawSlider()` | UI 滑动条 |
| 列表项 | `SCREEN_DrawListItem()` | UI 列表项 |

---

## 附：底层 ST7735 函数（通常不直接调用）

| 类别 | 函数 | 说明 |
|------|------|------|
| 像素 | `ST7735_DrawPixel()` | 底层像素绘制（不推荐直接使用） |
| 水平线 | `ST7735_DrawHorLine()` | 底层水平线绘制 |
| 垂直线 | `ST7735_DrawVerLine()` | 底层垂直线绘制 |
| 读像素 | `ST7735_ReadPixel()` | 读取帧缓冲像素值 |
| 命令 | `ST7735_WriteCmd()` | 发送 ST7735 命令 |
| 数据 | `ST7735_WriteByte()` / `ST7735_WriteData()` | 发送数据 |
| 地址 | `ST7735_SetAddressWindow()` | 设置 RAM 读写窗口 |
