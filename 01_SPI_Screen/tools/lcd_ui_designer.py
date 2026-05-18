#!/usr/bin/env python3
"""
Screen UI Designer - 可视化界面设计工具
用于 ST7735 128x128 LCD 的 UI 组件设计

依赖: pip install Pillow
用法: python lcd_ui_designer.py
"""

import tkinter as tk
from tkinter import ttk, colorchooser, messagebox
import json

# =============================================================================
# 组件类型定义
# =============================================================================
COMPONENT_TYPES = {
    "Button": {
        "struct": "struUI_Button_t",
        "fields": ["location", "frame", "label", "ascii_font", "hz_font", "color", "state"],
        "color_count": 3,
    },
    "Tooltip": {
        "struct": "struUI_Tooltip_t",
        "fields": ["location", "frame", "text", "ascii_font", "hz_font", "color"],
        "color_count": 2,
    },
    "ProgressBar": {
        "struct": "struUI_ProgressBar_t",
        "fields": ["location", "frame", "color", "progress"],
        "color_count": 2,
    },
    "Switch": {
        "struct": "struUI_Switch_t",
        "fields": ["location", "width", "height", "track_color", "thumb_color", "value"],
        "color_count": 2,
    },
    "Slider": {
        "struct": "struUI_Slider_t",
        "fields": ["location", "width", "height", "track_color", "thumb_color", "progress_color", "min_value", "max_value", "current_value"],
        "color_count": 3,
    },
    "ListItem": {
        "struct": "struUI_ListItem_t",
        "fields": ["location", "width", "height", "text", "font", "bg_color", "text_color", "border_color", "selected", "show_border"],
        "color_count": 3,
    },
}

# 预定义颜色
PRESET_COLORS = {
    "BLACK": 0x0000,
    "RED": 0xF800,
    "GREEN": 0x07E0,
    "BLUE": 0x001F,
    "YELLOW": 0xFFE0,
    "MAGENTA": 0xF81F,
    "CYAN": 0x07FF,
    "WHITE": 0xFFFF,
    "GRAY": 0x6E6E,
}

SCREEN_W, SCREEN_H = 128, 128
CANVAS_SCALE = 3  # 3px per LCD pixel


# =============================================================================
# 组件类
# =============================================================================
class UIComponent:
    next_id = 1

    def __init__(self, comp_type, x, y):
        self.id = UIComponent.next_id
        UIComponent.next_id += 1
        self.type = comp_type
        self.x = x
        self.y = y
        self.properties = self._default_properties(comp_type)

    def _default_properties(self, comp_type):
        if comp_type == "Button":
            return {
                "frame": [60, 24, 4],
                "label": "Button",
                "state": 0x00,
                "color": [0x07E0, 0x03E0, 0x0000],
            }
        elif comp_type == "Tooltip":
            return {
                "frame": [80, 20],
                "text": "Tooltip",
                "color": [0xFFFF, 0x0000],
            }
        elif comp_type == "ProgressBar":
            return {
                "frame": [80, 12],
                "color": [0xFFFF, 0x07E0],
                "progress": 50,
            }
        elif comp_type == "Switch":
            return {
                "width": 50,
                "height": 26,
                "track_color": 0x6E6E,
                "thumb_color": 0xFFFF,
                "value": False,
            }
        elif comp_type == "Slider":
            return {
                "width": 100,
                "height": 20,
                "track_color": 0x6E6E,
                "thumb_color": 0xFFFF,
                "progress_color": 0x07E0,
                "min_value": 0,
                "max_value": 100,
                "current_value": 50,
            }
        elif comp_type == "ListItem":
            return {
                "width": 120,
                "height": 22,
                "text": "List Item",
                "bg_color": 0x0000,
                "text_color": 0xFFFF,
                "border_color": 0x6E6E,
                "selected": False,
                "show_border": True,
            }
        return {}

    def get_bounds(self):
        """返回组件的包围盒 (x0, y0, x1, y1)"""
        p = self.properties
        if self.type == "Button":
            w, h = p["frame"][0], p["frame"][1]
        elif self.type == "Tooltip":
            w, h = p["frame"][0], p["frame"][1]
        elif self.type == "ProgressBar":
            w, h = p["frame"][0], p["frame"][1]
        elif self.type == "Switch":
            w, h = p["width"], p["height"]
        elif self.type == "Slider":
            w, h = p["width"], p["height"]
        elif self.type == "ListItem":
            w, h = p["width"], p["height"]
        else:
            w, h = 20, 20
        return (self.x - w // 2, self.y - h // 2, self.x + w // 2, self.y + h // 2)

    def contains_point(self, px, py):
        x0, y0, x1, y1 = self.get_bounds()
        return x0 <= px <= x1 and y0 <= py <= y1


# =============================================================================
# 渲染引擎 - 像素级复制 C 代码的绘制算法
# =============================================================================
SCREEN_MAX_ROUND_RADIUS = 64
SCREEN_MAX_ARC_POINTS = 128

def rgb565_to_rgb888(val):
    r = (val >> 11) & 0x1F
    g = (val >> 5) & 0x3F
    b = val & 0x1F
    return (r << 3 | r >> 2, g << 2 | g >> 4, b << 3 | b >> 2)


def rgb888_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3)


def xor_pixel(current, new_val):
    """异或模式：相同颜色则反色"""
    return current ^ new_val


class ScreenRenderer:
    """复制 C 代码 screen.c/st7735.c 的所有绘制逻辑"""

    def __init__(self):
        self.width = SCREEN_W
        self.height = SCREEN_H
        # 帧缓冲区：每个像素存 RGB565 值
        self.fb = [[0x0000] * SCREEN_W for _ in range(SCREEN_H)]
        # 静态缓存：圆弧点（Bresenham算法预计算）
        self._arc_cache = {}  # {r: [(x,y), ...]}

    def _in_bounds(self, x, y):
        return 0 <= x < self.width and 0 <= y < self.height

    # ---- 底层像素操作（复制 st7735.c） ----
    def draw_pixel(self, x, y, color, mode="Nor"):
        if not self._in_bounds(x, y):
            return
        if mode == "Xor" and self.fb[y][x] == color:
            self.fb[y][x] = xor_pixel(self.fb[y][x], color)
        else:
            self.fb[y][x] = color

    def draw_hor_line(self, x0, x1, y, color, mode="Nor"):
        """水平线 - 复制 st7735.c ST7735_DrawHorLine"""
        if y < 0 or y >= self.height:
            return
        x_start, x_end = (x0, x1) if x0 < x1 else (x1, x0)
        x_start = max(0, x_start)
        x_end = min(self.width - 1, x_end)
        if x_start > x_end:
            return
        for x in range(x_start, x_end + 1):
            val = xor_pixel(self.fb[y][x], color) if mode == "Xor" else color
            if self.fb[y][x] != val:
                self.fb[y][x] = val

    def draw_ver_line(self, x, y0, y1, color, mode="Nor"):
        """垂直线 - 复制 st7735.c ST7735_DrawVerLine"""
        if x < 0 or x >= self.width:
            return
        y_start, y_end = (y0, y1) if y0 < y1 else (y1, y0)
        y_start = max(0, y_start)
        y_end = min(self.height - 1, y_end)
        if y_start > y_end:
            return
        for y in range(y_start, y_end + 1):
            val = xor_pixel(self.fb[y][x], color) if mode == "Xor" else color
            if self.fb[y][x] != val:
                self.fb[y][x] = val

    def draw_rect_solid(self, x0, x1, y0, y1, color, mode="Nor"):
        """实心矩形 - 复制 screen.c SCREEN_DrawRectSolid"""
        x_start = min(x0, x1)
        x_end = max(x0, x1)
        y_start = min(y0, y1)
        y_end = max(y0, y1)
        for y in range(y_start, y_end + 1):
            self.draw_hor_line(x_start, x_end, y, color, mode)

    def draw_rect_hollow(self, x0, x1, y0, y1, color, mode="Nor"):
        """空心矩形 - 复制 screen.c SCREEN_DrawRectHollow"""
        self.draw_hor_line(x0, x1, y0, color, mode)
        self.draw_hor_line(x0, x1, y1, color, mode)
        self.draw_ver_line(x0, y0 + 1, y1 - 1, color, mode)
        self.draw_ver_line(x1, y0 + 1, y1 - 1, color, mode)

    # ---- 圆弧预计算（Bresenham算法，复制 screen.c SCREEN_DrawQuarArc） ----
    def _get_arc_points(self, r):
        """使用 Bresenham 算法预计算1/8圆弧点，复制 screen.c 静态缓存逻辑"""
        if r == 0:
            return []
        if r in self._arc_cache:
            return self._arc_cache[r]

        if r > SCREEN_MAX_ROUND_RADIUS:
            r = SCREEN_MAX_ROUND_RADIUS

        points = []
        x = 0
        y = r
        d = 3 - 2 * r

        while x <= y:
            points.append((x, y))
            if x < y:
                points.append((y, x))
            if d < 0:
                d = d + 4 * x + 6
            else:
                d = d + 4 * (x - y) + 10
                y -= 1
            x += 1

        self._arc_cache[r] = points
        return points

    def draw_quar_arc(self, cx, cy, r, quadrant_mask, color, mode="Nor"):
        """四分之一圆弧 - 复制 screen.c SCREEN_DrawQuarArc"""
        if r == 0:
            return
        r = min(r, SCREEN_MAX_ROUND_RADIUS)
        arc_pts = self._get_arc_points(r)

        for (x, y) in arc_pts:
            if quadrant_mask & 0x01:  # Q1: +x, -y
                self.draw_pixel(cx + x, cy - y, color, mode)
            if quadrant_mask & 0x02:  # Q2: -x, -y
                self.draw_pixel(cx - x, cy - y, color, mode)
            if quadrant_mask & 0x04:  # Q3: -x, +y
                self.draw_pixel(cx - x, cy + y, color, mode)
            if quadrant_mask & 0x08:  # Q4: +x, +y
                self.draw_pixel(cx + x, cy + y, color, mode)

    # ---- 圆角矩形（复制 screen.c 完整逻辑） ----
    def draw_round_rect_solid(self, x0, x1, y0, y1, radius, color, mode="Nor"):
        """实心圆角矩形 - 像素级复制 screen.c SCREEN_DrawRoundRectSolid"""
        x_start = min(x0, x1)
        x_end = max(x0, x1)
        y_start = min(y0, y1)
        y_end = max(y0, y1)
        w = x_end - x_start
        h = y_end - y_start

        if w <= 0 or h <= 0:
            return

        radius = min(radius, w // 2, h // 2)

        if radius == 0:
            self.draw_rect_solid(x_start, x_end, y_start, y_end, color, mode)
            return

        # 预计算圆角边界
        left_bound = {}
        arc_pts = self._get_arc_points(radius)
        for (x, y) in arc_pts:
            left_bound[y] = x
            left_bound[x] = y

        # 逐行绘制
        for row in range(y_start, y_end + 1):
            line_start = x_start
            line_end = x_end

            if row < y_start + radius:
                dy = row - (y_start + radius)
                offset = left_bound.get(abs(dy), 0)
                line_start = x_start + (radius - offset)
                line_end = x_end - (radius - offset)
            elif row > y_end - radius:
                dy = row - (y_end - radius)
                offset = left_bound.get(abs(dy), 0)
                line_start = x_start + (radius - offset)
                line_end = x_end - (radius - offset)

            if line_start <= line_end:
                self.draw_hor_line(line_start, line_end, row, color, mode)

    def draw_round_rect_hollow(self, x0, x1, y0, y1, radius, color, mode="Nor"):
        """空心圆角矩形 - 像素级复制 screen.c SCREEN_DrawRoundRectHollow"""
        x_start = min(x0, x1)
        x_end = max(x0, x1)
        y_start = min(y0, y1)
        y_end = max(y0, y1)
        w = x_end - x_start
        h = y_end - y_start

        if w <= 0 or h <= 0:
            return

        radius = min(radius, w // 2, h // 2)

        if radius == 0:
            self.draw_rect_hollow(x_start, x_end, y_start, y_end, color, mode)
            return

        # 四个圆心
        tl_cx, tl_cy = x_start + radius, y_start + radius
        tr_cx, tr_cy = x_end - radius, y_start + radius
        bl_cx, bl_cy = x_start + radius, y_end - radius
        br_cx, br_cy = x_end - radius, y_end - radius

        # 四条边（圆弧之间）
        if w > 2 * radius:
            self.draw_hor_line(x_start + radius + 1, x_end - radius - 1, y_start, color, mode)
            self.draw_hor_line(x_start + radius + 1, x_end - radius - 1, y_end, color, mode)
        if h > 2 * radius:
            self.draw_ver_line(x_start, y_start + radius + 1, y_end - radius - 1, color, mode)
            self.draw_ver_line(x_end, y_start + radius + 1, y_end - radius - 1, color, mode)

        # 四个圆角
        self.draw_quar_arc(tl_cx, tl_cy, radius, 0x02, color, mode)  # 左上
        self.draw_quar_arc(tr_cx, tr_cy, radius, 0x01, color, mode)  # 右上
        self.draw_quar_arc(bl_cx, bl_cy, radius, 0x04, color, mode)  # 左下
        self.draw_quar_arc(br_cx, br_cy, radius, 0x08, color, mode)  # 右下

    def to_image(self):
        """将帧缓冲区转换为 PIL RGB Image"""
        from PIL import Image
        img = Image.new("RGB", (SCREEN_W, SCREEN_H))
        for y in range(SCREEN_H):
            for x in range(SCREEN_W):
                r, g, b = rgb565_to_rgb888(self.fb[y][x])
                img.putpixel((x, y), (r, g, b))
        return img


def draw_component(renderer, comp, selected=False):
    """将单个组件绘制到渲染器 - 像素级复制 screen_ui.c 的绘制逻辑"""
    p = comp.properties
    x0, y0, x1, y1 = comp.get_bounds()

    # ---- Button ----
    if comp.type == "Button":
        border_c = p["color"][0]
        fill_c = p["color"][1]
        text_c = p["color"][2]

        if p["state"] == 0x00:
            # 未按下 - 空心圆角矩形
            renderer.draw_round_rect_hollow(x0, x1, y0, y1, p["frame"][2], border_c)
        else:
            # 按下 - 实心圆角矩形
            renderer.draw_round_rect_solid(x0, x1, y0, y1, p["frame"][2], fill_c)
        # 文字占位（简化显示）
        for dx in range(-len(p["label"]) * 3, len(p["label"]) * 3):
            renderer.draw_pixel(comp.x + dx, comp.y, text_c)

    # ---- Tooltip ----
    elif comp.type == "Tooltip":
        bg_c = p["color"][0]
        text_c = p["color"][1]
        # 阴影 (+2偏移)
        renderer.draw_rect_solid(x0 + 2, x1 + 2, y0 + 2, y1 + 2, 0x4208)
        # 背景
        renderer.draw_rect_solid(x0, x1, y0, y1, bg_c)

    # ---- ProgressBar ----
    elif comp.type == "ProgressBar":
        w, h = p["frame"]
        border_c = p["color"][0]
        fill_c = p["color"][1]
        radius = h // 2
        # 边框
        renderer.draw_round_rect_hollow(x0, x1, y0, y1, radius, border_c)
        # 进度填充
        progress = max(0, min(100, p["progress"]))
        fill_w = max(0, (w - 2) * progress // 100 - 1)
        if fill_w > 0 and radius > 0:
            renderer.draw_round_rect_solid(x0 + 1, x0 + 1 + fill_w, y0 + 1, y1 - 1,
                                            max(0, radius - 1), fill_c)

    # ---- Switch ----
    elif comp.type == "Switch":
        w, h = p["width"], p["height"]
        radius = h // 2
        track_c = 0x07E0 if p["value"] else p["track_color"]
        thumb_c = p["thumb_color"]
        # 轨道
        renderer.draw_round_rect_solid(x0, x1, y0, y1, radius, track_c)
        # 圆形滑块
        offset = (w // 2 - radius) if p["value"] else -(w // 2 - radius)
        tx, ty = comp.x + offset, comp.y
        tr = radius - 2
        for dy in range(-tr, tr + 1):
            for dx in range(-tr, tr + 1):
                if dx * dx + dy * dy <= tr * tr:
                    renderer.draw_pixel(tx + dx, ty + dy, thumb_c)

    # ---- Slider ----
    elif comp.type == "Slider":
        w, h = p["width"], p["height"]
        radius = h // 2
        # 轨道
        renderer.draw_round_rect_solid(x0, x1, y0, y1, radius, p["track_color"])
        # 进度填充
        val_range = max(1, p["max_value"] - p["min_value"])
        progress = (p["current_value"] - p["min_value"]) * 100 // val_range
        progress = max(0, min(100, progress))
        fill_w = (w - 2) * progress // 100
        if fill_w > 0:
            renderer.draw_round_rect_solid(x0 + 1, x0 + 1 + fill_w, y0 + 1, y1 - 1,
                                            max(0, radius - 1), p["progress_color"])
        # 滑块（矩形）
        thumb_w = h
        thumb_h = h - 2
        thumb_radius = thumb_h // 2
        tx0, tx1 = comp.x - thumb_w // 2, comp.x + thumb_w // 2
        ty0, ty1 = comp.y - thumb_h // 2, comp.y + thumb_h // 2
        renderer.draw_round_rect_solid(tx0, tx1, ty0, ty1, thumb_radius, p["thumb_color"])

    # ---- ListItem ----
    elif comp.type == "ListItem":
        # 背景
        renderer.draw_rect_solid(x0, x1, y0, y1, p["bg_color"])
        # 边框
        if p["show_border"]:
            renderer.draw_rect_hollow(x0, x1, y0, y1, p["border_color"])
        # 选中指示条
        if p["selected"]:
            renderer.draw_rect_solid(x0, x0 + 3, y0, y1, p["text_color"])

    # ---- 选中高亮 ----
    if selected:
        highlight = 0xFFFF
        renderer.draw_rect_hollow(x0 - 1, x1 + 1, y0 - 1, y1 + 1, highlight)


# =============================================================================
# 代码生成器
# =============================================================================
def generate_c_code(components):
    """根据组件列表生成 C 代码"""
    lines = []
    lines.append("/* ========== UI 组件定义 ========== */")
    lines.append("")

    # 按类型分组声明
    decl_lines = []
    init_lines = []

    for i, comp in enumerate(components):
        var_name = f"ui_{comp.type.lower()}_{comp.id}"
        p = comp.properties

        if comp.type == "Button":
            decl_lines.append(f"static struUI_Button_t {var_name};")
            init_lines.append(f"    {var_name}.location[0] = {comp.x};")
            init_lines.append(f"    {var_name}.location[1] = {comp.y};")
            init_lines.append(f"    {var_name}.frame[0] = {p['frame'][0]};")
            init_lines.append(f"    {var_name}.frame[1] = {p['frame'][1]};")
            init_lines.append(f"    {var_name}.frame[2] = {p['frame'][2]};")
            init_lines.append(f"    strcpy({var_name}.label, \"{p['label']}\");")
            init_lines.append(f"    {var_name}.ascii_font = (struFont_t *)&Font_8x12_consola;")
            init_lines.append(f"    {var_name}.hz_font = NULL;")
            init_lines.append(f"    {var_name}.color[0] = 0x{p['color'][0]:04X};")
            init_lines.append(f"    {var_name}.color[1] = 0x{p['color'][1]:04X};")
            init_lines.append(f"    {var_name}.color[2] = 0x{p['color'][2]:04X};")
            init_lines.append(f"    {var_name}.state = {p['state']:#04x};")
            init_lines.append(f"    SCREEN_DrawButton(&{var_name});")

        elif comp.type == "Tooltip":
            decl_lines.append(f"static struUI_Tooltip_t {var_name};")
            init_lines.append(f"    {var_name}.location[0] = {comp.x};")
            init_lines.append(f"    {var_name}.location[1] = {comp.y};")
            init_lines.append(f"    {var_name}.frame[0] = {p['frame'][0]};")
            init_lines.append(f"    {var_name}.frame[1] = {p['frame'][1]};")
            init_lines.append(f"    strcpy({var_name}.text, \"{p['text']}\");")
            init_lines.append(f"    {var_name}.ascii_font = (struFont_t *)&Font_8x12_consola;")
            init_lines.append(f"    {var_name}.hz_font = NULL;")
            init_lines.append(f"    {var_name}.color[0] = 0x{p['color'][0]:04X};")
            init_lines.append(f"    {var_name}.color[1] = 0x{p['color'][1]:04X};")
            init_lines.append(f"    SCREEN_DrawTooltip(&{var_name});")

        elif comp.type == "ProgressBar":
            decl_lines.append(f"static struUI_ProgressBar_t {var_name};")
            init_lines.append(f"    {var_name}.location[0] = {comp.x};")
            init_lines.append(f"    {var_name}.location[1] = {comp.y};")
            init_lines.append(f"    {var_name}.frame[0] = {p['frame'][0]};")
            init_lines.append(f"    {var_name}.frame[1] = {p['frame'][1]};")
            init_lines.append(f"    {var_name}.color[0] = 0x{p['color'][0]:04X};")
            init_lines.append(f"    {var_name}.color[1] = 0x{p['color'][1]:04X};")
            init_lines.append(f"    {var_name}.progress = {p['progress']};")
            init_lines.append(f"    SCREEN_DrawProgressBar(&{var_name});")

        elif comp.type == "Switch":
            decl_lines.append(f"static struUI_Switch_t {var_name};")
            init_lines.append(f"    {var_name}.location[0] = {comp.x};")
            init_lines.append(f"    {var_name}.location[1] = {comp.y};")
            init_lines.append(f"    {var_name}.width = {p['width']};")
            init_lines.append(f"    {var_name}.height = {p['height']};")
            init_lines.append(f"    {var_name}.track_color = 0x{p['track_color']:04X};")
            init_lines.append(f"    {var_name}.thumb_color = 0x{p['thumb_color']:04X};")
            init_lines.append(f"    {var_name}.value = {'true' if p['value'] else 'false'};")
            init_lines.append(f"    SCREEN_DrawSwitch(&{var_name});")

        elif comp.type == "Slider":
            decl_lines.append(f"static struUI_Slider_t {var_name};")
            init_lines.append(f"    {var_name}.location[0] = {comp.x};")
            init_lines.append(f"    {var_name}.location[1] = {comp.y};")
            init_lines.append(f"    {var_name}.width = {p['width']};")
            init_lines.append(f"    {var_name}.height = {p['height']};")
            init_lines.append(f"    {var_name}.track_color = 0x{p['track_color']:04X};")
            init_lines.append(f"    {var_name}.thumb_color = 0x{p['thumb_color']:04X};")
            init_lines.append(f"    {var_name}.progress_color = 0x{p['progress_color']:04X};")
            init_lines.append(f"    {var_name}.min_value = {p['min_value']};")
            init_lines.append(f"    {var_name}.max_value = {p['max_value']};")
            init_lines.append(f"    {var_name}.current_value = {p['current_value']};")
            init_lines.append(f"    SCREEN_DrawSlider(&{var_name});")

        elif comp.type == "ListItem":
            decl_lines.append(f"static struUI_ListItem_t {var_name};")
            init_lines.append(f"    {var_name}.location[0] = {comp.x};")
            init_lines.append(f"    {var_name}.location[1] = {comp.y};")
            init_lines.append(f"    {var_name}.width = {p['width']};")
            init_lines.append(f"    {var_name}.height = {p['height']};")
            init_lines.append(f"    strcpy({var_name}.text, \"{p['text']}\");")
            init_lines.append(f"    {var_name}.font = &Font_8x16_consola;")
            init_lines.append(f"    {var_name}.bg_color = 0x{p['bg_color']:04X};")
            init_lines.append(f"    {var_name}.text_color = 0x{p['text_color']:04X};")
            init_lines.append(f"    {var_name}.border_color = 0x{p['border_color']:04X};")
            init_lines.append(f"    {var_name}.selected = {'true' if p['selected'] else 'false'};")
            init_lines.append(f"    {var_name}.show_border = {'true' if p['show_border'] else 'false'};")
            init_lines.append(f"    SCREEN_DrawListItem(&{var_name});")

        init_lines.append("")

    lines.extend(decl_lines)
    lines.append("")
    lines.append("/* ========== UI 组件初始化 ========== */")
    lines.append("void UI_Init(void) {")
    lines.extend(init_lines)
    lines.append("}")
    lines.append("")

    return "\n".join(lines)


# =============================================================================
# 应用程序
# =============================================================================
class LCDUIApp:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("LCD UI Designer - ST7735 128x128")
        self.root.minsize(900, 600)

        self.components = []
        self.selected_comp = None
        self.dragging = False
        self.drag_offset_x = 0
        self.drag_offset_y = 0
        self.panning = False
        self.pan_start_x = 0
        self.pan_start_y = 0

        # 画布缩放偏移
        self.canvas_offset_x = 0
        self.canvas_offset_y = 0

        self._build_ui()
        self._render()

    def _build_ui(self):
        # ---- 顶部工具栏 ----
        toolbar = ttk.Frame(self.root)
        toolbar.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(toolbar, text="ST7735 128x128 UI Designer", font=("", 12, "bold")).pack(side=tk.LEFT, padx=8)

        ttk.Button(toolbar, text="Clear All", command=self._clear_all).pack(side=tk.RIGHT, padx=2)
        ttk.Button(toolbar, text="Load", command=self._load_project).pack(side=tk.RIGHT, padx=2)
        ttk.Button(toolbar, text="Save", command=self._save_project).pack(side=tk.RIGHT, padx=2)

        # ---- 主体：左侧组件面板 + 中间画布 + 右侧属性 ----
        main = ttk.Frame(self.root)
        main.pack(fill=tk.BOTH, expand=True, padx=4, pady=2)

        # 左侧：组件面板
        left_panel = ttk.LabelFrame(main, text="Components", padding=4)
        left_panel.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 4))

        for comp_type in COMPONENT_TYPES.keys():
            btn = tk.Button(left_panel, text=comp_type, font=("Consolas", 10),
                            width=12, bg="#2d2d2d", fg="#d4d4d4", relief=tk.RAISED,
                            command=lambda t=comp_type: self._add_component(t))
            btn.pack(pady=2)

        ttk.Separator(left_panel, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=8)

        ttk.Label(left_panel, text="Presets:", font=("", 9, "bold")).pack()
        preset_frame = ttk.Frame(left_panel)
        preset_frame.pack()
        for name, color_val in PRESET_COLORS.items():
            hex_c = f"#{rgb565_to_rgb888(color_val)[0]:02X}{rgb565_to_rgb888(color_val)[1]:02X}{rgb565_to_rgb888(color_val)[2]:02X}"
            lbl = tk.Label(preset_frame, text=name[:3], bg=hex_c,
                           font=("", 7), width=6, relief=tk.RIDGE)
            lbl.pack(pady=1)

        ttk.Separator(left_panel, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=8)

        self.info_label = ttk.Label(left_panel, text="", wraplength=120, font=("", 8))
        self.info_label.pack(pady=4)

        # 中间：画布区域
        canvas_frame = ttk.Frame(main)
        canvas_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        self.canvas = tk.Canvas(canvas_frame, bg="#1a1a1a", width=SCREEN_W * CANVAS_SCALE + 40,
                                height=SCREEN_H * CANVAS_SCALE + 40)
        self.canvas.pack()

        # 鼠标事件
        self.canvas.bind("<Button-1>", self._on_canvas_click)
        self.canvas.bind("<B1-Motion>", self._on_canvas_drag)
        self.canvas.bind("<ButtonRelease-1>", self._on_canvas_release)
        self.canvas.bind("<Button-3>", self._on_canvas_right_click)
        self.canvas.bind("<MouseWheel>", self._on_mousewheel)

        self.canvas.bind("<Motion>", self._on_canvas_motion)

        # 右侧：属性面板
        right_panel = ttk.LabelFrame(main, text="Properties", padding=4)
        right_panel.pack(side=tk.RIGHT, fill=tk.Y, padx=(4, 0))

        self.prop_frame = ttk.Frame(right_panel)
        self.prop_frame.pack(fill=tk.BOTH, expand=True)

        self.prop_widgets = {}
        self._build_prop_panel()

        # ---- 底部：代码预览 ----
        bottom = ttk.LabelFrame(self.root, text="Generated C Code", padding=4)
        bottom.pack(fill=tk.BOTH, expand=True, padx=4, pady=2)

        code_frame = ttk.Frame(bottom)
        code_frame.pack(fill=tk.BOTH, expand=True)

        self.code_text = tk.Text(code_frame, height=10, font=("Consolas", 9),
                                  bg="#1e1e1e", fg="#d4d4d4")
        code_scroll = ttk.Scrollbar(code_frame, orient="vertical", command=self.code_text.yview)
        self.code_text.config(yscrollcommand=code_scroll.set)
        code_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.code_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        btn_row = ttk.Frame(bottom)
        btn_row.pack(fill=tk.X, pady=2)

        ttk.Button(btn_row, text="Generate Code", command=self._generate_code).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_row, text="Copy to Clipboard", command=self._copy_code).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_row, text="Delete Selected", command=self._delete_selected).pack(side=tk.RIGHT, padx=2)

        self._update_info()

    def _build_prop_panel(self):
        for w in self.prop_frame.winfo_children():
            w.destroy()
        self.prop_widgets.clear()

        if self.selected_comp is None:
            ttk.Label(self.prop_frame, text="No component selected", foreground="gray").pack()
            return

        comp = self.selected_comp
        p = comp.properties

        def add_row(label, widget):
            row = ttk.Frame(self.prop_frame)
            row.pack(fill=tk.X, pady=2)
            ttk.Label(row, text=label, width=14).pack(side=tk.LEFT)
            widget.pack(side=tk.LEFT, fill=tk.X, expand=True)
            return widget

        # 位置
        add_row("Type:", ttk.Label(self.prop_frame, text=comp.type, font=("", 9, "bold")))
        add_row("X:", ttk.Label(self.prop_frame, text=str(comp.x)))
        add_row("Y:", ttk.Label(self.prop_frame, text=str(comp.y)))

        ttk.Separator(self.prop_frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=6)

        # 根据类型添加属性控件
        if comp.type == "Button":
            self._add_entry_row("Label:", p["label"], "label", str)
            self._add_spinner_row("Width:", p["frame"][0], "frame_0", int)
            self._add_spinner_row("Height:", p["frame"][1], "frame_1", int)
            self._add_spinner_row("Radius:", p["frame"][2], "frame_2", int)
            self._add_color_row("Border:", p["color"][0], "color_0")
            self._add_color_row("Fill:", p["color"][1], "color_1")
            self._add_color_row("Text:", p["color"][2], "color_2")
            self._add_check_row("Pressed:", p["state"] == 0xFF, "state_pressed")

        elif comp.type == "Tooltip":
            self._add_entry_row("Text:", p["text"], "text", str)
            self._add_spinner_row("Width:", p["frame"][0], "frame_0", int)
            self._add_spinner_row("Height:", p["frame"][1], "frame_1", int)
            self._add_color_row("Background:", p["color"][0], "color_0")
            self._add_color_row("Text Color:", p["color"][1], "color_1")

        elif comp.type == "ProgressBar":
            self._add_spinner_row("Width:", p["frame"][0], "frame_0", int)
            self._add_spinner_row("Height:", p["frame"][1], "frame_1", int)
            self._add_color_row("Border:", p["color"][0], "color_0")
            self._add_color_row("Fill:", p["color"][1], "color_1")
            self._add_spinner_row("Progress:", p["progress"], "progress", int)

        elif comp.type == "Switch":
            self._add_spinner_row("Width:", p["width"], "width", int)
            self._add_spinner_row("Height:", p["height"], "height", int)
            self._add_color_row("Track:", p["track_color"], "track_color")
            self._add_color_row("Thumb:", p["thumb_color"], "thumb_color")
            self._add_check_row("ON:", p["value"], "value_bool")

        elif comp.type == "Slider":
            self._add_spinner_row("Width:", p["width"], "width", int)
            self._add_spinner_row("Height:", p["height"], "height", int)
            self._add_color_row("Track:", p["track_color"], "track_color")
            self._add_color_row("Thumb:", p["thumb_color"], "thumb_color")
            self._add_color_row("Progress:", p["progress_color"], "progress_color")
            self._add_spinner_row("Min:", p["min_value"], "min_value", int)
            self._add_spinner_row("Max:", p["max_value"], "max_value", int)
            self._add_spinner_row("Value:", p["current_value"], "current_value", int)

        elif comp.type == "ListItem":
            self._add_entry_row("Text:", p["text"], "text", str)
            self._add_spinner_row("Width:", p["width"], "width", int)
            self._add_spinner_row("Height:", p["height"], "height", int)
            self._add_color_row("Background:", p["bg_color"], "bg_color")
            self._add_color_row("Text:", p["text_color"], "text_color")
            self._add_color_row("Border:", p["border_color"], "border_color")
            self._add_check_row("Selected:", p["selected"], "selected")
            self._add_check_row("Show Border:", p["show_border"], "show_border")

    def _add_entry_row(self, label, value, key, dtype):
        row = ttk.Frame(self.prop_frame)
        row.pack(fill=tk.X, pady=2)
        ttk.Label(row, text=label, width=14).pack(side=tk.LEFT)
        var = tk.StringVar(value=str(value))
        ent = ttk.Entry(row, textvariable=var)
        ent.pack(side=tk.LEFT, fill=tk.X, expand=True)
        ent.bind("<FocusOut>", lambda e: self._on_prop_change(key, dtype(var.get())))
        ent.bind("<Return>", lambda e: self._on_prop_change(key, dtype(var.get())))
        self.prop_widgets[key] = var

    def _add_spinner_row(self, label, value, key, dtype):
        row = ttk.Frame(self.prop_frame)
        row.pack(fill=tk.X, pady=2)
        ttk.Label(row, text=label, width=14).pack(side=tk.LEFT)
        var = tk.IntVar(value=int(value))
        sp = ttk.Spinbox(row, from_=0, to=999, textvariable=var, width=8)
        sp.pack(side=tk.LEFT)
        sp.bind("<FocusOut>", lambda e: self._on_prop_change(key, dtype(var.get())))
        sp.bind("<Return>", lambda e: self._on_prop_change(key, dtype(var.get())))
        self.prop_widgets[key] = var

    def _add_color_row(self, label, value, key):
        row = ttk.Frame(self.prop_frame)
        row.pack(fill=tk.X, pady=2)
        ttk.Label(row, text=label, width=14).pack(side=tk.LEFT)
        var = tk.IntVar(value=int(value))
        hex_str = f"#{rgb565_to_rgb888(value)[0]:02X}{rgb565_to_rgb888(value)[1]:02X}{rgb565_to_rgb888(value)[2]:02X}"
        color_lbl = tk.Label(row, text=hex_str, bg=hex_str, relief=tk.SUNKEN, font=("Consolas", 9), width=10)
        color_lbl.pack(side=tk.LEFT, padx=2)

        def pick():
            color = colorchooser.askcolor(title=f"Pick {label}")
            if color[0]:
                rgb565 = rgb888_to_rgb565(int(color[0][0]), int(color[0][1]), int(color[0][2]))
                var.set(rgb565)
                new_hex = f"#{int(color[0][0]):02X}{int(color[0][1]):02X}{int(color[0][2]):02X}"
                color_lbl.config(bg=new_hex, text=new_hex)
                self._on_prop_change(key, rgb565)

        ttk.Button(row, text="Pick", command=pick, width=6).pack(side=tk.LEFT)
        self.prop_widgets[key] = var

    def _add_check_row(self, label, value, key):
        row = ttk.Frame(self.prop_frame)
        row.pack(fill=tk.X, pady=2)
        ttk.Label(row, text=label, width=14).pack(side=tk.LEFT)
        var = tk.BooleanVar(value=bool(value))
        ttk.Checkbutton(row, variable=var).pack(side=tk.LEFT)
        var.trace_add("write", lambda *_: self._on_prop_change(key, var.get()))
        self.prop_widgets[key] = var

    def _on_prop_change(self, key, value):
        if self.selected_comp is None:
            return
        p = self.selected_comp.properties

        if key in ("label", "text"):
            p[key] = value
        elif key.startswith("frame_"):
            idx = int(key.split("_")[1])
            p["frame"][idx] = value
        elif key.startswith("color_"):
            idx = int(key.split("_")[1])
            p["color"][idx] = value
        elif key in ("width", "height", "progress", "min_value", "max_value", "current_value"):
            p[key] = value
        elif key in ("track_color", "thumb_color", "progress_color", "bg_color", "text_color", "border_color"):
            p[key] = value
        elif key == "value_bool":
            p["value"] = value
        elif key == "state_pressed":
            p["state"] = 0xFF if value else 0x00
        elif key in ("selected", "show_border"):
            p[key] = value

        self._render()

    def _add_component(self, comp_type):
        comp = UIComponent(comp_type, SCREEN_W // 2, SCREEN_H // 2)
        self.components.append(comp)
        self.selected_comp = comp
        self._build_prop_panel()
        self._render()
        self._update_info()

    def _delete_selected(self):
        if self.selected_comp:
            self.components.remove(self.selected_comp)
            self.selected_comp = None
            self._build_prop_panel()
            self._render()
            self._update_info()

    def _clear_all(self):
        if messagebox.askyesno("Clear", "Delete all components?"):
            self.components.clear()
            self.selected_comp = None
            self._build_prop_panel()
            self._render()
            self._update_info()

    def _render(self):
        self.canvas.delete("all")

        # 计算画布偏移（居中）
        cw = self.canvas.winfo_width() or (SCREEN_W * CANVAS_SCALE + 40)
        ch = self.canvas.winfo_height() or (SCREEN_H * CANVAS_SCALE + 40)
        ox = (cw - SCREEN_W * CANVAS_SCALE) // 2
        oy = (ch - SCREEN_H * CANVAS_SCALE) // 2
        self.canvas_offset_x = ox
        self.canvas_offset_y = oy

        # 屏幕背景
        self.canvas.create_rectangle(ox, oy, ox + SCREEN_W * CANVAS_SCALE, oy + SCREEN_H * CANVAS_SCALE,
                                      fill="#111111", outline="#444444", width=2)

        # 网格线
        for i in range(0, SCREEN_W + 1, 16):
            x = ox + i * CANVAS_SCALE
            self.canvas.create_line(x, oy, x, oy + SCREEN_H * CANVAS_SCALE, fill="#222222")
        for i in range(0, SCREEN_H + 1, 16):
            y = oy + i * CANVAS_SCALE
            self.canvas.create_line(ox, y, ox + SCREEN_W * CANVAS_SCALE, y, fill="#222222")

        # 渲染组件到帧缓冲
        renderer = ScreenRenderer()
        for comp in self.components:
            draw_component(renderer, comp, selected=(comp == self.selected_comp))
        img = renderer.to_image()

        # 缩放显示
        for y in range(SCREEN_H):
            for x in range(SCREEN_W):
                r, g, b = img.getpixel((x, y))
                color = f"#{r:02X}{g:02X}{b:02X}"
                x1 = ox + x * CANVAS_SCALE
                y1 = oy + y * CANVAS_SCALE
                self.canvas.create_rectangle(x1, y1, x1 + CANVAS_SCALE, y1 + CANVAS_SCALE,
                                              fill=color, outline="")

    def _get_canvas_lcd_coords(self, event):
        x = (event.x - self.canvas_offset_x) // CANVAS_SCALE
        y = (event.y - self.canvas_offset_y) // CANVAS_SCALE
        return x, y

    def _on_canvas_click(self, event):
        x, y = self._get_canvas_lcd_coords(event)
        # 查找点击的组件
        for comp in reversed(self.components):
            if comp.contains_point(x, y):
                self.selected_comp = comp
                self.dragging = True
                self.drag_offset_x = x - comp.x
                self.drag_offset_y = y - comp.y
                self._build_prop_panel()
                self._render()
                return
        # 点击空白区域
        self.selected_comp = None
        self._build_prop_panel()
        self._render()

    def _on_canvas_drag(self, event):
        if self.dragging and self.selected_comp:
            x, y = self._get_canvas_lcd_coords(event)
            x = max(0, min(SCREEN_W - 1, x - self.drag_offset_x))
            y = max(0, min(SCREEN_H - 1, y - self.drag_offset_y))
            self.selected_comp.x = x
            self.selected_comp.y = y
            self._render()
            self._build_prop_panel()

    def _on_canvas_release(self, event):
        self.dragging = False

    def _on_canvas_right_click(self, event):
        x, y = self._get_canvas_lcd_coords(event)
        for comp in reversed(self.components):
            if comp.contains_point(x, y):
                if messagebox.askyesno("Delete", f"Delete {comp.type} #{comp.id}?"):
                    self.components.remove(comp)
                    if self.selected_comp == comp:
                        self.selected_comp = None
                        self._build_prop_panel()
                    self._render()
                return

    def _on_mousewheel(self, event):
        pass

    def _on_canvas_motion(self, event):
        x, y = self._get_canvas_lcd_coords(event)
        self.info_label.config(text=f"Cursor: ({x}, {y})")

    def _generate_code(self):
        code = generate_c_code(self.components)
        self.code_text.delete("1.0", tk.END)
        self.code_text.insert("1.0", code)

    def _copy_code(self):
        code = self.code_text.get("1.0", tk.END)
        self.root.clipboard_clear()
        self.root.clipboard_append(code)
        messagebox.showinfo("Copied", "Code copied to clipboard!")

    def _update_info(self):
        self.info_label.config(text=f"Components: {len(self.components)}\nSelected: {self.selected_comp.type if self.selected_comp else 'None'}")

    def _save_project(self):
        data = []
        for comp in self.components:
            item = {
                "id": comp.id,
                "type": comp.type,
                "x": comp.x,
                "y": comp.y,
                "properties": comp.properties,
            }
            data.append(item)

        json_str = json.dumps(data, indent=2, ensure_ascii=False)
        self.code_text.delete("1.0", tk.END)
        self.code_text.insert("1.0", json_str)
        messagebox.showinfo("Save", "JSON data is in the code area. Copy it to a file manually.")

    def _load_project(self):
        json_str = self.code_text.get("1.0", tk.END).strip()
        if not json_str:
            messagebox.showwarning("Load", "Paste JSON data in the code area first.")
            return
        try:
            data = json.loads(json_str)
        except json.JSONDecodeError:
            messagebox.showerror("Error", "Invalid JSON data.")
            return

        self.components.clear()
        UIComponent.next_id = 1
        for item in data:
            comp = UIComponent(item["type"], item["x"], item["y"])
            comp.properties = item["properties"]
            UIComponent.next_id = max(UIComponent.next_id, item["id"] + 1)
            self.components.append(comp)

        self.selected_comp = None
        self._build_prop_panel()
        self._render()
        self._update_info()
        messagebox.showinfo("Load", f"Loaded {len(data)} components.")

    def run(self):
        self.root.mainloop()


# =============================================================================
if __name__ == "__main__":
    app = LCDUIApp()
    app.run()