#!/usr/bin/env python3
"""ST7735 LCD UI Designer - 可视化拖拽设计128x128 LCD界面，导出C代码"""

import tkinter as tk
from tkinter import messagebox
import math

# ---------------------------------------------------------------------------
# RGB565 颜色工具
# ---------------------------------------------------------------------------
NAMED_COLORS = {
    "SCREEN_BLACK":   0x0000,
    "SCREEN_RED":     0xF800,
    "SCREEN_GREEN":   0x07E0,
    "SCREEN_BLUE":    0x001F,
    "SCREEN_YELLOW":  0xFFE0,
    "SCREEN_MAGENTA": 0xF81F,
    "SCREEN_CYAN":    0x07FF,
    "SCREEN_WHITE":   0xFFFF,
}

SHADOW_RGB565 = 0x4208


def rgb565_to_hex(val):
    r = ((val >> 11) & 0x1F) * 255 // 31
    g = ((val >> 5) & 0x3F) * 255 // 63
    b = (val & 0x1F) * 255 // 31
    return f"#{r:02x}{g:02x}{b:02x}"


def rgb888_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3)


def color_name(val):
    for name, v in NAMED_COLORS.items():
        if v == val:
            return name
    return f"0x{val:04X}"


# ---------------------------------------------------------------------------
# 字体 & 组件类型定义
# ---------------------------------------------------------------------------
ASCII_FONTS = ["Font_8x16_consola", "Font_8x12_consola",
               "Font_8x16_times", "Font_8x12_times"]
UTF_FONTS = ["NULL", "Font_UTF_16x16_YuMincho", "Font_UTF_16x12_YuMincho"]

FONT_W = {
    "Font_8x16_consola": 8, "Font_8x12_consola": 8,
    "Font_8x16_times": 8, "Font_8x12_times": 8,
    "Font_UTF_16x16_YuMincho": 16, "Font_UTF_16x12_YuMincho": 16,
    "NULL": 0,
}
FONT_H = {
    "Font_8x16_consola": 16, "Font_8x12_consola": 12,
    "Font_8x16_times": 16, "Font_8x12_times": 12,
    "Font_UTF_16x16_YuMincho": 16, "Font_UTF_16x12_YuMincho": 12,
    "NULL": 0,
}

QUADRANT_OPTIONS = [
    ("Full Circle", 0x0F),
    ("Top-Right (Q1)", 0x01), ("Top-Left (Q2)", 0x02),
    ("Bottom-Left (Q3)", 0x04), ("Bottom-Right (Q4)", 0x08),
    ("Upper Half", 0x03), ("Lower Half", 0x0C),
    ("Left Half", 0x06), ("Right Half", 0x09),
]

# ---------------------------------------------------------------------------
# UID 计数器
# ---------------------------------------------------------------------------
_uid_counter = 0


def next_uid():
    global _uid_counter
    _uid_counter += 1
    return _uid_counter


# ---------------------------------------------------------------------------
# UI 组件数据模型
# ---------------------------------------------------------------------------
class UIComponent:
    LCD_W = 128
    LCD_H = 128

    def __init__(self, comp_type, cx, cy):
        self.uid = next_uid()
        self.comp_type = comp_type
        # ---------- UI 组件 ----------
        if comp_type == "button":
            self.props = {
                "location": [cx, cy], "frame": [30, 14, 3],
                "label": "OK", "ascii_font": "Font_8x12_consola",
                "hz_font": "NULL", "color": [0x07E0, 0x0000, 0xFFFF],
                "state": 0x00,
            }
        elif comp_type == "tooltip":
            self.props = {
                "location": [cx, cy], "frame": [48, 12],
                "text": "Tip", "ascii_font": "Font_8x12_consola",
                "hz_font": "NULL", "color": [0xF800, 0xFFFF],
            }
        elif comp_type == "progressbar":
            self.props = {
                "location": [cx, cy], "frame": [40, 8],
                "color": [0xF800, 0x07E0], "progress": 75,
            }
        elif comp_type == "switch":
            self.props = {
                "location": [cx, cy], "width": 50, "height": 26,
                "track_color": 0x6E6E, "thumb_color": 0xFFFF, "value": False,
            }
        elif comp_type == "slider":
            self.props = {
                "location": [cx, cy], "width": 100, "height": 20,
                "track_color": 0x6E6E, "thumb_color": 0xFFFF,
                "progress_color": 0x07E0, "min_value": 0,
                "max_value": 100, "current_value": 50,
            }
        elif comp_type == "listitem":
            self.props = {
                "location": [cx, cy], "width": 120, "height": 22,
                "text": "List Item", "font": "Font_8x16_consola",
                "bg_color": 0x0000, "text_color": 0xFFFF,
                "border_color": 0x6E6E, "selected": False, "show_border": True,
            }
        # ---------- 基本图形 ----------
        elif comp_type == "rect_solid":
            self.props = {
                "location": [cx, cy], "frame": [40, 30],
                "color": [0x07E0],
            }
        elif comp_type == "rect_hollow":
            self.props = {
                "location": [cx, cy], "frame": [40, 30],
                "color": [0xFFFF],
            }
        elif comp_type == "rrect_solid":
            self.props = {
                "location": [cx, cy], "frame": [40, 24, 5],
                "color": [0x001F],
            }
        elif comp_type == "rrect_hollow":
            self.props = {
                "location": [cx, cy], "frame": [40, 24, 5],
                "color": [0xFFFF],
            }
        elif comp_type == "line":
            self.props = {
                "location": [max(0, cx - 20), max(0, cy - 15)],
                "end": [min(self.LCD_W - 1, cx + 20), min(self.LCD_H - 1, cy + 15)],
                "color": [0xF800],
            }
        elif comp_type == "circle":
            self.props = {
                "location": [cx, cy], "radius": 20,
                "quadrant": 0x0F, "color": [0xFFE0],
            }
        elif comp_type == "sector":
            self.props = {
                "location": [cx, cy], "radius": 20,
                "quadrant": 0x0F, "color": [0x07FF],
            }
        elif comp_type == "label":
            self.props = {
                "location": [cx, cy], "text": "Text",
                "ascii_font": "Font_8x12_consola", "color": [0xFFFF],
            }

    # ---------- 边界计算 ----------
    def get_bounds(self):
        p = self.props
        loc = p["location"]
        t = self.comp_type

        if t in ("button", "progressbar"):
            w, h = p["frame"][0], p["frame"][1]
            return (loc[0] - w // 2, loc[1] - h // 2,
                    loc[0] + w // 2, loc[1] + h // 2)
        elif t in ("switch", "slider"):
            w, h = p["width"], p["height"]
            return (loc[0] - w // 2, loc[1] - h // 2,
                    loc[0] + w // 2, loc[1] + h // 2)
        elif t == "listitem":
            w, h = p["width"], p["height"]
            return (loc[0], loc[1], loc[0] + w, loc[1] + h)
        elif t in ("rect_solid", "rect_hollow"):
            w, h = p["frame"][0], p["frame"][1]
            return (loc[0] - w // 2, loc[1] - h // 2,
                    loc[0] + w // 2, loc[1] + h // 2)
        elif t in ("rrect_solid", "rrect_hollow"):
            w, h = p["frame"][0], p["frame"][1]
            return (loc[0] - w // 2, loc[1] - h // 2,
                    loc[0] + w // 2, loc[1] + h // 2)
        elif t == "tooltip":
            w, h = p["frame"][0], p["frame"][1]
            return (loc[0] - w // 2, loc[1] - h // 2,
                    loc[0] + w // 2 + 2, loc[1] + h // 2 + 2)
        elif t == "line":
            x0, y0 = p["location"]
            x1, y1 = p["end"]
            return (min(x0, x1), min(y0, y1), max(x0, x1), max(y0, y1))
        elif t in ("circle", "sector"):
            r = p["radius"]
            return (loc[0] - r, loc[1] - r, loc[0] + r, loc[1] + r)
        elif t == "label":
            fw = FONT_W.get(p["ascii_font"], 8)
            fh = FONT_H.get(p["ascii_font"], 12)
            tw = len(p["text"]) * fw
            return (loc[0], loc[1], loc[0] + tw, loc[1] + fh)
        return (0, 0, 0, 0)

    def contains(self, px, py):
        x0, y0, x1, y1 = self.get_bounds()
        pad = 4
        return (x0 - pad <= px <= x1 + pad and y0 - pad <= py <= y1 + pad)

    def display_name(self):
        p = self.props
        t = self.comp_type
        NAMES = {
            "button":       lambda: f'Button "{p["label"]}"',
            "tooltip":      lambda: f'Tooltip "{p["text"]}"',
            "progressbar":  lambda: f'ProgressBar {p["progress"]}%',
            "switch":       lambda: f'Switch {"ON" if p["value"] else "OFF"}',
            "slider":       lambda: f'Slider {p["current_value"]}',
            "listitem":     lambda: f'ListItem "{p["text"]}"',
            "rect_solid":   lambda: f'RectFill {p["frame"][0]}x{p["frame"][1]}',
            "rect_hollow":  lambda: f'RectEdge {p["frame"][0]}x{p["frame"][1]}',
            "rrect_solid":  lambda: f'RRectFill {p["frame"][0]}x{p["frame"][1]}',
            "rrect_hollow": lambda: f'RRectEdge {p["frame"][0]}x{p["frame"][1]}',
            "line":         lambda: f'Line',
            "circle":       lambda: f'Circle r={p["radius"]}',
            "sector":       lambda: f'Sector r={p["radius"]}',
            "label":        lambda: f'Label "{p["text"]}"',
        }
        return NAMES.get(t, lambda: t)()


# ---------------------------------------------------------------------------
# 圆角矩形辅助
# ---------------------------------------------------------------------------
def rounded_rect_points(x0, y0, x1, y1, r):
    r = min(r, (x1 - x0) // 2, (y1 - y0) // 2)
    r = max(r, 0)
    pts = []
    steps = max(4, r)
    corners = [
        (x0 + r, y0 + r, math.pi, 1.5 * math.pi),
        (x1 - r, y0 + r, 1.5 * math.pi, 2 * math.pi),
        (x1 - r, y1 - r, 0, 0.5 * math.pi),
        (x0 + r, y1 - r, 0.5 * math.pi, math.pi),
    ]
    for cx, cy, a0, a1 in corners:
        for i in range(steps + 1):
            a = a0 + (a1 - a0) * i / steps
            pts.append(cx + r * math.cos(a))
            pts.append(cy + r * math.sin(a))
    return pts


# ---------------------------------------------------------------------------
# LCD 画布
# ---------------------------------------------------------------------------
class LCDCanvas:
    SCALE = 3

    def __init__(self, parent, app, lcd_w=128, lcd_h=128):
        self.app = app
        self.LCD_W = lcd_w
        self.LCD_H = lcd_h
        self.components = []
        self.selected = None
        self._placing_type = None
        self._drag_offset = None

        frame = tk.Frame(parent)
        frame.pack(fill=tk.BOTH, expand=True)

        self.canvas = tk.Canvas(frame,
                                width=self.LCD_W * self.SCALE,
                                height=self.LCD_H * self.SCALE,
                                bg="#1a1a2e", highlightthickness=1,
                                highlightbackground="#444")
        self.canvas.pack(padx=4, pady=4)

        self.coord_label = tk.Label(frame, text="Coordinate: (-, -)", anchor="w")
        self.coord_label.pack(fill=tk.X, padx=4)

        self.canvas.bind("<Button-1>", self._on_click)
        self.canvas.bind("<B1-Motion>", self._on_drag)
        self.canvas.bind("<ButtonRelease-1>", self._on_release)
        self.canvas.bind("<Motion>", self._on_motion)
        self._draw_background()

    def _draw_background(self):
        s = self.SCALE
        w, h = self.LCD_W * s, self.LCD_H * s
        self.canvas.create_rectangle(0, 0, w, h, fill="#000000", outline="#333", tags="bg")
        for x in range(0, w + 1, 8 * s):
            self.canvas.create_line(x, 0, x, h, fill="#111", tags="bg")
        for y in range(0, h + 1, 8 * s):
            self.canvas.create_line(0, y, w, y, fill="#111", tags="bg")

    def set_placing_mode(self, comp_type):
        self._placing_type = comp_type
        self.canvas.config(cursor="crosshair")
        self.app.set_status(f"Click to place {comp_type}...")

    def _lcd_xy(self, event):
        return event.x // self.SCALE, event.y // self.SCALE

    def _on_click(self, event):
        lx, ly = self._lcd_xy(event)
        if self._placing_type:
            comp = UIComponent(self._placing_type, lx, ly)
            self.components.append(comp)
            self.selected = comp
            self._placing_type = None
            self.canvas.config(cursor="")
            self.app.set_status("Select mode")
            self.redraw()
            self.app.show_properties(comp)
            return

        hit = None
        for comp in reversed(self.components):
            if comp.contains(lx, ly):
                hit = comp
                break
        self.selected = hit
        if hit:
            self._drag_offset = (lx - hit.props["location"][0],
                                 ly - hit.props["location"][1])
        self.redraw()
        self.app.show_properties(hit)

    def _on_drag(self, event):
        if not self.selected or not self._drag_offset:
            return
        lx, ly = self._lcd_xy(event)
        dx, dy = self._drag_offset
        nx = max(0, min(self.LCD_W - 1, lx - dx))
        ny = max(0, min(self.LCD_H - 1, ly - dy))
        old = self.selected.props["location"]
        delta_x = nx - old[0]
        delta_y = ny - old[1]
        self.selected.props["location"] = [nx, ny]
        # 直线需要同步移动终点
        if "end" in self.selected.props:
            ex, ey = self.selected.props["end"]
            self.selected.props["end"] = [ex + delta_x, ey + delta_y]
        self.redraw()
        self.app.refresh_properties()

    def _on_release(self, event):
        self._drag_offset = None

    def _on_motion(self, event):
        lx, ly = self._lcd_xy(event)
        if 0 <= lx < self.LCD_W and 0 <= ly < self.LCD_H:
            self.coord_label.config(text=f"Coordinate: ({lx}, {ly})")
        else:
            self.coord_label.config(text="Coordinate: (-, -)")

    def delete_selected(self):
        if self.selected and self.selected in self.components:
            self.components.remove(self.selected)
            self.selected = None
            self.redraw()
            self.app.show_properties(None)

    def redraw(self):
        self.canvas.delete("comp")
        self.canvas.delete("sel")
        for comp in self.components:
            self._draw_component(comp)
        if self.selected:
            self._draw_selection(self.selected)

    def _draw_component(self, comp):
        s = self.SCALE
        t = comp.comp_type
        p = comp.props
        loc = p["location"]
        sx, sy = loc[0] * s, loc[1] * s
        lw = max(1, s // 2)

        # --- 矩形类 (location = center) ---
        if t in ("rect_solid", "rect_hollow", "rrect_solid", "rrect_hollow", "button", "progressbar", "tooltip"):
            x0, y0, x1, y1 = comp.get_bounds()
            if t in ("rect_solid", "rect_hollow", "rrect_solid", "rrect_hollow"):
                color = rgb565_to_hex(p["color"][0])
            # tooltip 阴影偏移修正
            if t == "tooltip":
                x1 -= 2
                y1 -= 2
            sx0, sy0 = x0 * s, y0 * s
            sx1, sy1 = x1 * s, y1 * s

            if t in ("rrect_solid", "rrect_hollow"):
                r = p["frame"][2] * s
                pts = rounded_rect_points(sx0, sy0, sx1, sy1, r)
                if len(pts) >= 6:
                    if t == "rrect_solid":
                        self.canvas.create_polygon(pts, fill=color, outline="", tags="comp")
                    else:
                        self.canvas.create_polygon(pts, fill="", outline=color, width=lw, tags="comp")

            elif t == "rect_solid":
                self.canvas.create_rectangle(sx0, sy0, sx1, sy1,
                                             fill=color, outline="", tags="comp")
            elif t == "rect_hollow":
                self.canvas.create_rectangle(sx0, sy0, sx1, sy1,
                                             fill="", outline=color, width=lw, tags="comp")

            elif t == "button":
                r = p["frame"][2] * s
                border_c = rgb565_to_hex(p["color"][0])
                fill_c = rgb565_to_hex(p["color"][1])
                text_c = rgb565_to_hex(p["color"][2])
                pts = rounded_rect_points(sx0, sy0, sx1, sy1, r)
                if len(pts) >= 6:
                    if p["state"] == 0x00:
                        self.canvas.create_polygon(pts, fill="", outline=border_c, width=lw, tags="comp")
                    else:
                        self.canvas.create_polygon(pts, fill=fill_c, outline=border_c, width=lw, tags="comp")
                fn = p["ascii_font"]
                fs = max(7, FONT_H.get(fn, 12) * s // 2)
                self.canvas.create_text(sx0 + 2 * s, (sy0 + sy1) / 2,
                                        text=p["label"], fill=text_c, anchor="w",
                                        font=("Consolas", fs), tags="comp")

            elif t == "tooltip":
                bg_c = rgb565_to_hex(p["color"][0])
                text_c = rgb565_to_hex(p["color"][1])
                shadow_c = rgb565_to_hex(SHADOW_RGB565)
                so = 2 * s
                self.canvas.create_rectangle(sx0 + so, sy0 + so, sx1 + so, sy1 + so,
                                             fill=shadow_c, outline="", tags="comp")
                self.canvas.create_rectangle(sx0, sy0, sx1, sy1,
                                             fill=bg_c, outline="", tags="comp")
                fn = p["ascii_font"]
                fs = max(7, FONT_H.get(fn, 12) * s // 2)
                self.canvas.create_text(sx0 + 4 * s, (sy0 + sy1) / 2,
                                        text=p["text"], fill=text_c, anchor="w",
                                        font=("Consolas", fs), tags="comp")

            elif t == "progressbar":
                border_c = rgb565_to_hex(p["color"][0])
                fill_c = rgb565_to_hex(p["color"][1])
                w, h = p["frame"][0], p["frame"][1]
                r = (h // 2) * s
                pts = rounded_rect_points(sx0, sy0, sx1, sy1, r)
                if len(pts) >= 6:
                    self.canvas.create_polygon(pts, fill="", outline=border_c, width=lw, tags="comp")
                fill_w = (w - 2) * p["progress"] // 100
                if fill_w > 0:
                    fx0, fy0 = sx0 + s, sy0 + s
                    fx1, fy1 = fx0 + fill_w * s, sy1 - s
                    fpts = rounded_rect_points(fx0, fy0, fx1, fy1, max(0, r - s))
                    if len(fpts) >= 6:
                        self.canvas.create_polygon(fpts, fill=fill_c, outline="", tags="comp")

            # --- Switch ---
            elif t == "switch":
                sw = p["width"] * s
                sh = p["height"] * s
                rx = sx - sw // 2
                ry = sy - sh // 2
                r = sh // 2
                track_c = rgb565_to_hex(0x07E0 if p["value"] else 0x6E6E)
                thumb_c = rgb565_to_hex(p["thumb_color"])
                pts = rounded_rect_points(rx, ry, rx + sw, ry + sh, r)
                if len(pts) >= 6:
                    self.canvas.create_polygon(pts, fill=track_c, outline="", tags="comp")
                tr = r - 2 * s
                offset = (sw // 2 - r) * (1 if p["value"] else -1)
                tx = sx + offset
                ty = sy
                self.canvas.create_arc(tx - tr, ty - tr, tx + tr, ty + tr,
                                        start=0, extent=360, style="pieslice",
                                        fill=thumb_c, outline="", tags="comp")

            # --- Slider ---
            elif t == "slider":
                sl = p["width"] * s
                sh = p["height"] * s
                rx = sx - sl // 2
                ry = sy - sh // 2
                r = sh // 2
                track_c = rgb565_to_hex(p["track_color"])
                prog_c = rgb565_to_hex(p["progress_color"])
                thumb_c = rgb565_to_hex(p["thumb_color"])
                pts = rounded_rect_points(rx, ry, rx + sl, ry + sh, r)
                if len(pts) >= 6:
                    self.canvas.create_polygon(pts, fill=track_c, outline="", tags="comp")
                rng = p["max_value"] - p["min_value"]
                if rng <= 0: rng = 1
                prog = (p["current_value"] - p["min_value"]) * (sl - 2 * s) // rng
                if prog > 0:
                    fx0, fy0 = rx + s, ry + s
                    fx1, fy1 = fx0 + prog, ry + sh - s
                    fpts = rounded_rect_points(fx0, fy0, fx1, fy1, max(0, r - s))
                    if len(fpts) >= 6:
                        self.canvas.create_polygon(fpts, fill=prog_c, outline="", tags="comp")
                tw = sh
                th = sh - 2 * s
                tr = th // 2
                tx0 = sx - tw // 2
                ty0 = sy - th // 2
                tpts = rounded_rect_points(tx0, ty0, tx0 + tw, ty0 + th, tr)
                if len(tpts) >= 6:
                    self.canvas.create_polygon(tpts, fill=thumb_c, outline="", tags="comp")

            # --- ListItem ---
            elif t == "listitem":
                x0, y0 = loc[0] * s, loc[1] * s
                w, h = p["width"] * s, p["height"] * s
                bg_c = rgb565_to_hex(p["bg_color"])
                bd_c = rgb565_to_hex(p["border_color"])
                self.canvas.create_rectangle(x0, y0, x0 + w, y0 + h,
                                             fill=bg_c, outline=bd_c, width=lw, tags="comp")
                fn = p["font"]
                fs = max(7, FONT_H.get(fn, 12) * s // 2)
                text_c = rgb565_to_hex(p["text_color"])
                self.canvas.create_text(x0 + 4 * s, y0 + h // 2,
                                        text=p["text"], fill=text_c, anchor="w",
                                        font=("Consolas", fs), tags="comp")
                if p["selected"]:
                    self.canvas.create_rectangle(x0, y0, x0 + 3 * s, y0 + h,
                                                  fill=text_c, outline="", tags="comp")

        # --- 直线 ---
        elif t == "line":
            color = rgb565_to_hex(p["color"][0])
            ex, ey = p["end"]
            self.canvas.create_line(sx, sy, ex * s, ey * s,
                                    fill=color, width=lw, tags="comp")

        # --- 圆弧 ---
        elif t == "circle":
            color = rgb565_to_hex(p["color"][0])
            r = p["radius"] * s
            mask = p["quadrant"]
            for q, (qx, qy, sa, ea) in [
                (0x01, (sx, sy, -90, 0)), (0x02, (sx, sy, -180, -90)),
                (0x04, (sx, sy, 90, 180)), (0x08, (sx, sy, 0, 90)),
            ]:
                if mask & q:
                    self.canvas.create_arc(qx - r, qy - r, qx + r, qy + r,
                                           start=sa, extent=ea, style="arc",
                                           outline=color, width=lw, tags="comp")

        # --- 扇形 ---
        elif t == "sector":
            color = rgb565_to_hex(p["color"][0])
            r = p["radius"] * s
            mask = p["quadrant"]
            for q, (qx, qy, sa, ea) in [
                (0x01, (sx, sy, -90, 0)), (0x02, (sx, sy, -180, -90)),
                (0x04, (sx, sy, 90, 180)), (0x08, (sx, sy, 0, 90)),
            ]:
                if mask & q:
                    self.canvas.create_arc(qx - r, qy - r, qx + r, qy + r,
                                           start=sa, extent=ea, style="pieslice",
                                           fill=color, outline="", tags="comp")

        # --- 文本标签 ---
        elif t == "label":
            color = rgb565_to_hex(p["color"][0])
            fn = p["ascii_font"]
            fs = max(7, FONT_H.get(fn, 12) * s // 2)
            self.canvas.create_text(sx, sy, text=p["text"], fill=color,
                                    anchor="nw", font=("Consolas", fs), tags="comp")

    def _draw_selection(self, comp):
        s = self.SCALE
        x0, y0, x1, y1 = comp.get_bounds()
        pad = 3 * s
        self.canvas.create_rectangle(x0 * s - pad, y0 * s - pad,
                                     x1 * s + pad, y1 * s + pad,
                                     outline="#00BFFF", width=2,
                                     dash=(6, 3), tags="sel")


# ---------------------------------------------------------------------------
# 属性面板
# ---------------------------------------------------------------------------
class PropertyPanel:
    def __init__(self, parent, app):
        self.app = app
        self.comp = None
        self._widgets = {}

        container = tk.Frame(parent)
        container.pack(fill=tk.BOTH, expand=True)
        tk.Label(container, text="Properties", font=("", 11, "bold")).pack(anchor="w", padx=4, pady=2)

        self.scroll_canvas = tk.Canvas(container, highlightthickness=0)
        scrollbar = tk.Scrollbar(container, orient="vertical", command=self.scroll_canvas.yview)
        self.inner = tk.Frame(self.scroll_canvas)
        self.inner.bind("<Configure>",
                        lambda e: self.scroll_canvas.configure(scrollregion=self.scroll_canvas.bbox("all")))
        self.scroll_canvas.create_window((0, 0), window=self.inner, anchor="nw")
        self.scroll_canvas.configure(yscrollcommand=scrollbar.set)
        self.scroll_canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self._show_placeholder()

    def _show_placeholder(self):
        tk.Label(self.inner, text="No component selected", fg="gray").pack(padx=8, pady=20)

    def show(self, comp):
        for w in self.inner.winfo_children():
            w.destroy()
        self._widgets.clear()
        self.comp = comp
        if comp is None:
            self._show_placeholder()
            return

        row = 0
        tk.Label(self.inner, text=comp.display_name(), font=("", 10, "bold")).grid(
            row=row, column=0, columnspan=3, sticky="w", padx=4, pady=(4, 2))
        row += 1
        row = self._label(row, f"Type: {comp.comp_type}")

        t = comp.comp_type
        p = comp.props

        # ===== UI 组件 =====
        if t == "button":
            row = self._spin(row, "Center X", "location", 0)
            row = self._spin(row, "Center Y", "location", 1)
            row = self._spin(row, "Width", "frame", 0, 1, 255)
            row = self._spin(row, "Height", "frame", 1, 1, 255)
            row = self._spin(row, "Radius", "frame", 2, 0, 64)
            row = self._entry(row, "Label", "label")
            row = self._opt(row, "ASCII Font", "ascii_font", ASCII_FONTS)
            row = self._opt(row, "UTF Font", "hz_font", UTF_FONTS)
            row = self._color(row, "Border Color", "color", 0)
            row = self._color(row, "Fill Color", "color", 1)
            row = self._color(row, "Text Color", "color", 2)
            row = self._opt(row, "State", "state",
                            [("0x00 Normal", 0x00), ("0xFF Pressed", 0xFF)])

        elif t == "tooltip":
            row = self._spin(row, "Center X", "location", 0)
            row = self._spin(row, "Center Y", "location", 1)
            row = self._spin(row, "Width", "frame", 0, 1, 255)
            row = self._spin(row, "Height", "frame", 1, 1, 255)
            row = self._entry(row, "Text", "text")
            row = self._opt(row, "ASCII Font", "ascii_font", ASCII_FONTS)
            row = self._opt(row, "UTF Font", "hz_font", UTF_FONTS)
            row = self._color(row, "BG Color", "color", 0)
            row = self._color(row, "Text Color", "color", 1)

        elif t == "progressbar":
            row = self._spin(row, "Center X", "location", 0)
            row = self._spin(row, "Center Y", "location", 1)
            row = self._spin(row, "Width", "frame", 0, 1, 255)
            row = self._spin(row, "Height", "frame", 1, 1, 255)
            row = self._color(row, "Border Color", "color", 0)
            row = self._color(row, "Fill Color", "color", 1)
            row = self._scale(row, "Progress", "progress", 0, 100)

        elif t == "switch":
            row = self._spin(row, "Center X", "location", 0)
            row = self._spin(row, "Center Y", "location", 1)
            row = self._spin(row, "Width", "width", None, 1, 255)
            row = self._spin(row, "Height", "height", None, 1, 255)
            row = self._color(row, "Track Color", "track_color", None)
            row = self._color(row, "Thumb Color", "thumb_color", None)
            row = self._opt(row, "Value", "value",
                            [("OFF (false)", False), ("ON (true)", True)])

        elif t == "slider":
            row = self._spin(row, "Center X", "location", 0)
            row = self._spin(row, "Center Y", "location", 1)
            row = self._spin(row, "Width", "width", None, 1, 255)
            row = self._spin(row, "Height", "height", None, 1, 255)
            row = self._color(row, "Track Color", "track_color", None)
            row = self._color(row, "Thumb Color", "thumb_color", None)
            row = self._color(row, "Progress Color", "progress_color", None)
            row = self._spin(row, "Min Value", "min_value", None, -32768, 32767)
            row = self._spin(row, "Max Value", "max_value", None, -32768, 32767)
            row = self._spin(row, "Current Value", "current_value", None, -32768, 32767)

        elif t == "listitem":
            row = self._spin(row, "X", "location", 0)
            row = self._spin(row, "Y", "location", 1)
            row = self._spin(row, "Width", "width", None, 1, 255)
            row = self._spin(row, "Height", "height", None, 1, 255)
            row = self._entry(row, "Text", "text")
            row = self._opt(row, "Font", "font", ASCII_FONTS)
            row = self._color(row, "BG Color", "bg_color", None)
            row = self._color(row, "Text Color", "text_color", None)
            row = self._color(row, "Border Color", "border_color", None)
            row = self._opt(row, "Selected", "selected",
                            [("No", False), ("Yes", True)])
            row = self._opt(row, "Show Border", "show_border",
                            [("No", False), ("Yes", True)])

        # ===== 基本图形 =====
        elif t in ("rect_solid", "rect_hollow"):
            row = self._spin(row, "Center X", "location", 0)
            row = self._spin(row, "Center Y", "location", 1)
            row = self._spin(row, "Width", "frame", 0, 1, 255)
            row = self._spin(row, "Height", "frame", 1, 1, 255)
            row = self._color(row, "Color", "color", 0)

        elif t in ("rrect_solid", "rrect_hollow"):
            row = self._spin(row, "Center X", "location", 0)
            row = self._spin(row, "Center Y", "location", 1)
            row = self._spin(row, "Width", "frame", 0, 1, 255)
            row = self._spin(row, "Height", "frame", 1, 1, 255)
            row = self._spin(row, "Radius", "frame", 2, 0, 64)
            row = self._color(row, "Color", "color", 0)

        elif t == "line":
            row = self._spin(row, "Start X", "location", 0)
            row = self._spin(row, "Start Y", "location", 1)
            row = self._spin(row, "End X", "end", 0)
            row = self._spin(row, "End Y", "end", 1)
            row = self._color(row, "Color", "color", 0)

        elif t in ("circle", "sector"):
            row = self._spin(row, "Center X", "location", 0)
            row = self._spin(row, "Center Y", "location", 1)
            row = self._spin(row, "Radius", "radius", None, 1, 64)
            row = self._opt(row, "Quadrant", "quadrant", QUADRANT_OPTIONS)
            row = self._color(row, "Color", "color", 0)

        elif t == "label":
            row = self._spin(row, "X", "location", 0)
            row = self._spin(row, "Y", "location", 1)
            row = self._entry(row, "Text", "text")
            row = self._opt(row, "Font", "ascii_font", ASCII_FONTS)
            row = self._color(row, "Color", "color", 0)

    def refresh(self):
        if not self.comp:
            return
        for key, (widget, prop_key, idx) in self._widgets.items():
            try:
                if isinstance(widget, tk.Spinbox):
                    widget.delete(0, tk.END)
                    val = self.comp.props[prop_key]
                    widget.insert(0, str(val if idx is None else val[idx]))
                elif isinstance(widget, tk.Entry):
                    widget.delete(0, tk.END)
                    widget.insert(0, str(self.comp.props[prop_key]))
                elif isinstance(widget, tk.Scale):
                    widget.set(self.comp.props[prop_key])
            except Exception:
                pass

    # ---------- 辅助构建方法 ----------
    def _label(self, row, text):
        tk.Label(self.inner, text=text, fg="gray").grid(
            row=row, column=0, columnspan=3, sticky="w", padx=4, pady=1)
        return row + 1

    def _spin(self, row, label, prop_key, idx, lo=-128, hi=127):
        tk.Label(self.inner, text=label).grid(row=row, column=0, sticky="w", padx=4, pady=1)
        val = self.comp.props[prop_key]
        var = tk.StringVar(value=str(val if idx is None else val[idx]))
        sb = tk.Spinbox(self.inner, from_=lo, to=hi, textvariable=var, width=6,
                        command=lambda: self._apply_spin(prop_key, idx, var))
        sb.bind("<Return>", lambda e: self._apply_spin(prop_key, idx, var))
        sb.bind("<FocusOut>", lambda e: self._apply_spin(prop_key, idx, var))
        sb.grid(row=row, column=1, columnspan=2, sticky="w", padx=4, pady=1)
        self._widgets[f"{prop_key}_{idx}"] = (sb, prop_key, idx)
        return row + 1

    def _entry(self, row, label, prop_key):
        tk.Label(self.inner, text=label).grid(row=row, column=0, sticky="w", padx=4, pady=1)
        var = tk.StringVar(value=str(self.comp.props[prop_key]))
        e = tk.Entry(self.inner, textvariable=var, width=16)
        e.bind("<Return>", lambda ev: self._apply_text(prop_key, var))
        e.bind("<FocusOut>", lambda ev: self._apply_text(prop_key, var))
        e.grid(row=row, column=1, columnspan=2, sticky="w", padx=4, pady=1)
        self._widgets[prop_key] = (e, prop_key, 0)
        return row + 1

    def _opt(self, row, label, prop_key, options):
        tk.Label(self.inner, text=label).grid(row=row, column=0, sticky="w", padx=4, pady=1)
        var = tk.StringVar()

        if isinstance(options[0], tuple):
            display = [o[0] for o in options]
            values = [o[1] for o in options]
            cur = self.comp.props[prop_key]
            for i, v in enumerate(values):
                if v == cur:
                    var.set(display[i])
                    break
            else:
                var.set(display[0])
            om = tk.OptionMenu(self.inner, var, *display)
            om.config(width=16)
            om.grid(row=row, column=1, columnspan=2, sticky="w", padx=4, pady=1)

            def _cb(dl=display, vl=values, v=var, pk=prop_key):
                idx = dl.index(v.get())
                self.comp.props[pk] = vl[idx]
                self.app.canvas.redraw()
            var.trace_add("write", lambda *a: _cb())
        else:
            var.set(self.comp.props[prop_key])
            om = tk.OptionMenu(self.inner, var, *options)
            om.config(width=16)
            om.grid(row=row, column=1, columnspan=2, sticky="w", padx=4, pady=1)

            def _cb_str(v=var, pk=prop_key):
                self.comp.props[pk] = v.get()
                self.app.canvas.redraw()
            var.trace_add("write", lambda *a: _cb_str())

        self._widgets[label] = (om, prop_key, 0)
        return row + 1

    def _color(self, row, label, prop_key, idx):
        tk.Label(self.inner, text=label).grid(row=row, column=0, sticky="w", padx=4, pady=1)
        if idx is None:
            val = self.comp.props[prop_key]
        else:
            val = self.comp.props[prop_key][idx]
        btn = tk.Button(self.inner, bg=rgb565_to_hex(val), width=3, relief="solid",
                        command=lambda: self._pick_color(prop_key, idx))
        btn.grid(row=row, column=1, sticky="w", padx=2, pady=1)
        lbl = tk.Label(self.inner, text=color_name(val), width=10, anchor="w")
        lbl.grid(row=row, column=2, sticky="w", padx=2, pady=1)
        self._widgets[f"{prop_key}_{idx}"] = (btn, prop_key, idx)
        return row + 1

    def _scale(self, row, label, prop_key, lo, hi):
        tk.Label(self.inner, text=label).grid(row=row, column=0, sticky="w", padx=4, pady=1)
        var = tk.IntVar(value=self.comp.props[prop_key])
        sc = tk.Scale(self.inner, from_=lo, to=hi, orient=tk.HORIZONTAL, variable=var,
                      length=140, command=lambda v: self._apply_scale(prop_key, int(v)))
        sc.grid(row=row, column=1, columnspan=2, sticky="w", padx=4, pady=1)
        self._widgets[prop_key] = (sc, prop_key, 0)
        return row + 1

    # ---------- 值应用 ----------
    def _apply_spin(self, prop_key, idx, var):
        try:
            val = int(var.get())
            if idx is not None:
                self.comp.props[prop_key][idx] = val
            else:
                self.comp.props[prop_key] = val
            self.app.canvas.redraw()
        except ValueError:
            pass

    def _apply_text(self, prop_key, var):
        self.comp.props[prop_key] = var.get()
        self.app.canvas.redraw()

    def _apply_scale(self, prop_key, val):
        self.comp.props[prop_key] = val
        self.app.canvas.redraw()

    # ---------- 颜色选择器 ----------
    def _pick_color(self, prop_key, idx):
        if idx is None:
            current = self.comp.props[prop_key]
        else:
            current = self.comp.props[prop_key][idx]
        top = tk.Toplevel(self.app.root)
        top.title("Pick Color")
        top.resizable(False, False)
        top.grab_set()
        chosen = [current]

        row = 0
        tk.Label(top, text="Named Colors", font=("", 10, "bold")).grid(
            row=row, column=0, columnspan=4, sticky="w", padx=8, pady=(8, 4))
        row += 1

        for name, val in NAMED_COLORS.items():
            hex_c = rgb565_to_hex(val)
            btn = tk.Button(top, bg=hex_c, width=20, height=1, relief="solid",
                            text=name.replace("SCREEN_", ""), font=("", 7),
                            fg="white" if val < 0x8000 else "black")
            btn.grid(row=row, column=0, columnspan=4, sticky="w", padx=8, pady=1)

            def _pick(v=val, t=top):
                chosen[0] = v
                t.destroy()
            btn.config(command=_pick)
            row += 1

        row += 1
        tk.Label(top, text="Custom RGB (0-255)", font=("", 9, "bold")).grid(
            row=row, column=0, columnspan=4, sticky="w", padx=8, pady=(8, 2))
        row += 1

        r0 = ((current >> 11) & 0x1F) * 255 // 31
        g0 = ((current >> 5) & 0x3F) * 255 // 63
        b0 = (current & 0x1F) * 255 // 31
        r_var = tk.StringVar(value=str(r0))
        g_var = tk.StringVar(value=str(g0))
        b_var = tk.StringVar(value=str(b0))

        preview = tk.Label(top, text="  Preview  ", bg=rgb565_to_hex(current),
                           relief="solid", width=10)
        preview.grid(row=row, column=0, columnspan=4, padx=8, pady=4)
        row += 1

        for i, (lbl, var) in enumerate([("R", r_var), ("G", g_var), ("B", b_var)]):
            tk.Label(top, text=lbl).grid(row=row, column=i, padx=2)
            tk.Entry(top, textvariable=var, width=5).grid(row=row + 1, column=i, padx=2)
        row += 2

        def _apply_custom():
            try:
                r = max(0, min(255, int(r_var.get())))
                g = max(0, min(255, int(g_var.get())))
                b = max(0, min(255, int(b_var.get())))
                val = rgb888_to_rgb565(r, g, b)
                preview.config(bg=rgb565_to_hex(val))
                chosen[0] = val
            except ValueError:
                pass

        tk.Button(top, text="Preview", command=_apply_custom).grid(
            row=row, column=0, columnspan=2, padx=4, pady=4)
        row += 1

        def _on_close():
            if idx is None:
                self.comp.props[prop_key] = chosen[0]
            else:
                self.comp.props[prop_key][idx] = chosen[0]
            self.app.canvas.redraw()
            self.show(self.comp)
            top.destroy()

        tk.Button(top, text="OK", command=_on_close).grid(
            row=row, column=0, columnspan=4, padx=8, pady=(4, 8))
        top.protocol("WM_DELETE_WINDOW", _on_close)


# ---------------------------------------------------------------------------
# 主应用
# ---------------------------------------------------------------------------
SHAPE_TYPES = [
    ("rect_solid",   "Rect Fill"),
    ("rect_hollow",  "Rect Edge"),
    ("rrect_solid",  "RRect Fill"),
    ("rrect_hollow", "RRect Edge"),
    ("line",         "Line"),
    ("circle",       "Circle"),
    ("sector",       "Sector"),
    ("label",        "Text"),
]

UI_TYPES = [
    ("button",       "Button"),
    ("tooltip",      "Tooltip"),
    ("progressbar",  "ProgBar"),
    ("switch",       "Switch"),
    ("slider",       "Slider"),
    ("listitem",     "ListItem"),
]


class LCDUIDesignerApp:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("LCD UI Designer")
        self.root.minsize(920, 520)

        # 默认分辨率
        self.lcd_w_var = tk.IntVar(value=128)
        self.lcd_h_var = tk.IntVar(value=128)

        # --- 左侧面板 ---
        left = tk.Frame(self.root, width=130, relief="ridge", bd=1)
        left.pack(side=tk.LEFT, fill=tk.Y, padx=2, pady=2)
        left.pack_propagate(False)

        # 分辨率设置
        res_frame = tk.Frame(left)
        res_frame.pack(fill=tk.X, padx=4, pady=(6, 2))
        tk.Label(res_frame, text="LCD Size:", font=("", 9, "bold")).pack(anchor="w")
        rf = tk.Frame(res_frame)
        rf.pack(fill=tk.X)
        tk.Spinbox(rf, from_=16, to=320, textvariable=self.lcd_w_var, width=4).pack(side=tk.LEFT)
        tk.Label(rf, text="x").pack(side=tk.LEFT)
        tk.Spinbox(rf, from_=16, to=320, textvariable=self.lcd_h_var, width=4).pack(side=tk.LEFT)
        tk.Button(res_frame, text="Apply", width=8, command=self._apply_lcd_size).pack(pady=2)

        tk.Frame(left, height=2, relief="sunken", bd=1).pack(fill=tk.X, padx=8, pady=4)

        # 基本图形区
        tk.Label(left, text="Shapes", font=("", 10, "bold")).pack(pady=(2, 4))
        for comp_type, icon in SHAPE_TYPES:
            tk.Button(left, text=f"+ {icon}", width=14,
                      command=lambda t=comp_type: self.canvas.set_placing_mode(t)).pack(pady=1, padx=4)

        tk.Frame(left, height=2, relief="sunken", bd=1).pack(fill=tk.X, padx=8, pady=6)

        # UI 组件区
        tk.Label(left, text="UI Components", font=("", 10, "bold")).pack(pady=(2, 4))
        for comp_type, icon in UI_TYPES:
            tk.Button(left, text=f"+ {icon}", width=14,
                      command=lambda t=comp_type: self.canvas.set_placing_mode(t)).pack(pady=1, padx=4)

        tk.Frame(left, height=2, relief="sunken", bd=1).pack(fill=tk.X, padx=8, pady=6)

        # 操作按钮
        tk.Button(left, text="Export C Code", width=14, command=self.export_c_code).pack(pady=2, padx=4)
        tk.Button(left, text="Delete", width=14,
                  command=lambda: self.canvas.delete_selected()).pack(pady=1, padx=4)
        tk.Button(left, text="Clear All", width=14, command=self.clear_all).pack(pady=1, padx=4)

        tk.Frame(left, height=2, relief="sunken", bd=1).pack(fill=tk.X, padx=8, pady=6)

        self.status_label = tk.Label(left, text="Select mode", fg="blue", wraplength=120)
        self.status_label.pack(pady=2, padx=4)

        tk.Label(left, text="Objects:", font=("", 9, "bold")).pack(anchor="w", padx=4, pady=(2, 1))
        self.comp_listbox = tk.Listbox(left, width=16, height=8)
        self.comp_listbox.pack(fill=tk.BOTH, expand=True, padx=4, pady=2)
        self.comp_listbox.bind("<<ListboxSelect>>", self._on_list_select)

        # --- 中央画布 ---
        self.center_frame = tk.Frame(self.root)
        self.center_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=2, pady=2)
        self.canvas = LCDCanvas(self.center_frame, self,
                                lcd_w=self.lcd_w_var.get(),
                                lcd_h=self.lcd_h_var.get())

        # --- 右侧属性 ---
        right = tk.Frame(self.root, width=260, relief="ridge", bd=1)
        right.pack(side=tk.RIGHT, fill=tk.Y, padx=2, pady=2)
        right.pack_propagate(False)
        self.prop_panel = PropertyPanel(right, self)

        # 快捷键
        self.root.bind("<Delete>", lambda e: self.canvas.delete_selected())
        self.root.bind("<BackSpace>", lambda e: self.canvas.delete_selected())
        self.root.bind("<Escape>", lambda e: self._cancel_placing())

    def _apply_lcd_size(self):
        w = self.lcd_w_var.get()
        h = self.lcd_h_var.get()
        UIComponent.LCD_W = w
        UIComponent.LCD_H = h
        self.canvas.canvas.destroy()
        self.canvas.coord_label.destroy()
        self.canvas.LCD_W = w
        self.canvas.LCD_H = h
        self.canvas.canvas = tk.Canvas(self.center_frame,
                                        width=w * LCDCanvas.SCALE,
                                        height=h * LCDCanvas.SCALE,
                                        bg="#1a1a2e", highlightthickness=1,
                                        highlightbackground="#444")
        self.canvas.canvas.pack(padx=4, pady=4)
        self.canvas.coord_label = tk.Label(self.center_frame, text="Coordinate: (-, -)", anchor="w")
        self.canvas.coord_label.pack(fill=tk.X, padx=4)
        self.canvas.canvas.bind("<Button-1>", self.canvas._on_click)
        self.canvas.canvas.bind("<B1-Motion>", self.canvas._on_drag)
        self.canvas.canvas.bind("<ButtonRelease-1>", self.canvas._on_release)
        self.canvas.canvas.bind("<Motion>", self.canvas._on_motion)
        self.canvas._draw_background()
        self.canvas.redraw()
        self.root.title(f"LCD UI Designer ({w}x{h})")

    def set_status(self, text):
        self.status_label.config(text=text)

    def show_properties(self, comp):
        self.prop_panel.show(comp)
        self._refresh_list()

    def refresh_properties(self):
        self.prop_panel.refresh()
        self._refresh_list()

    def _refresh_list(self):
        sel_uid = self.canvas.selected.uid if self.canvas.selected else None
        self.comp_listbox.delete(0, tk.END)
        sel_idx = None
        for i, comp in enumerate(self.canvas.components):
            self.comp_listbox.insert(tk.END, comp.display_name())
            if comp.uid == sel_uid:
                sel_idx = i
        if sel_idx is not None:
            self.comp_listbox.selection_set(sel_idx)
            self.comp_listbox.see(sel_idx)

    def _on_list_select(self, event):
        sel = self.comp_listbox.curselection()
        if not sel:
            return
        idx = sel[0]
        if idx < len(self.canvas.components):
            comp = self.canvas.components[idx]
            self.canvas.selected = comp
            self.canvas.redraw()
            self.prop_panel.show(comp)

    def _cancel_placing(self):
        self.canvas._placing_type = None
        self.canvas.canvas.config(cursor="")
        self.set_status("Select mode")

    def clear_all(self):
        if not self.canvas.components:
            return
        if messagebox.askyesno("Clear All", "Remove all objects?"):
            self.canvas.components.clear()
            self.canvas.selected = None
            self.canvas.redraw()
            self.show_properties(None)

    # -----------------------------------------------------------------------
    # 导出 C 代码
    # -----------------------------------------------------------------------
    def export_c_code(self):
        components = self.canvas.components
        if not components:
            messagebox.showinfo("Export", "No objects to export.")
            return

        lines = [
            "/* Generated by LCD UI Designer */",
            '#include "screen.h"',
            '#include "screen_ui.h"',
            "",
        ]

        counters = {}
        for comp in components:
            p = comp.props
            loc = p["location"]
            t = comp.comp_type
            c = color_name(p["color"][0])
            counters[t] = counters.get(t, 0) + 1
            n = counters[t]

            if t == "button":
                hz = "NULL" if p["hz_font"] == "NULL" else p["hz_font"]
                hz_line = ".hz_font = NULL," if hz == "NULL" else f".hz_font = (struFont_UTF_t *)&{hz},"
                lines.append(f"/* Button: {p['label']} */")
                lines.append(f"struUI_Button_t ui_btn_{n} = {{")
                lines.append(f"    .location = {{{loc[0]}, {loc[1]}}},")
                lines.append(f"    .frame = {{{p['frame'][0]}, {p['frame'][1]}, {p['frame'][2]}}},")
                lines.append(f"    .label = \"{p['label']}\",")
                lines.append(f"    .ascii_font = (struFont_t *)&{p['ascii_font']},")
                lines.append(f"    {hz_line}")
                lines.append(f"    .color = {{{color_name(p['color'][0])}, {color_name(p['color'][1])}, {color_name(p['color'][2])}}},")
                lines.append(f"    .state = 0x{p['state']:02X}")
                lines.append("};")
                lines.append(f"SCREEN_DrawButton(&ui_btn_{n});")

            elif t == "tooltip":
                hz = "NULL" if p["hz_font"] == "NULL" else p["hz_font"]
                hz_line = ".hz_font = NULL," if hz == "NULL" else f".hz_font = (struFont_UTF_t *)&{hz},"
                lines.append(f"/* Tooltip: {p['text']} */")
                lines.append(f"struUI_Tooltip_t ui_tip_{n} = {{")
                lines.append(f"    .location = {{{loc[0]}, {loc[1]}}},")
                lines.append(f"    .frame = {{{p['frame'][0]}, {p['frame'][1]}}},")
                lines.append(f"    .text = \"{p['text']}\",")
                lines.append(f"    .ascii_font = (struFont_t *)&{p['ascii_font']},")
                lines.append(f"    {hz_line}")
                lines.append(f"    .color = {{{color_name(p['color'][0])}, {color_name(p['color'][1])}}}")
                lines.append("};")
                lines.append(f"SCREEN_DrawTooltip(&ui_tip_{n});")

            elif t == "progressbar":
                lines.append(f"/* ProgressBar: {p['progress']}% */")
                lines.append(f"struUI_ProgressBar_t ui_bar_{n} = {{")
                lines.append(f"    .location = {{{loc[0]}, {loc[1]}}},")
                lines.append(f"    .frame = {{{p['frame'][0]}, {p['frame'][1]}}},")
                lines.append(f"    .color = {{{color_name(p['color'][0])}, {color_name(p['color'][1])}}},")
                lines.append(f"    .progress = {p['progress']}")
                lines.append("};")
                lines.append(f"SCREEN_DrawProgressBar(&ui_bar_{n});")

            elif t == "switch":
                lines.append(f"/* Switch: {'ON' if p['value'] else 'OFF'} */")
                lines.append(f"struUI_Switch_t ui_sw_{n} = {{")
                lines.append(f"    .location = {{{loc[0]}, {loc[1]}}},")
                lines.append(f"    .width = {p['width']},")
                lines.append(f"    .height = {p['height']},")
                lines.append(f"    .track_color = {color_name(p['track_color'])},")
                lines.append(f"    .thumb_color = {color_name(p['thumb_color'])},")
                lines.append(f"    .value = {'true' if p['value'] else 'false'}")
                lines.append("};")
                lines.append(f"SCREEN_DrawSwitch(&ui_sw_{n});")

            elif t == "slider":
                lines.append(f"/* Slider: {p['current_value']} */")
                lines.append(f"struUI_Slider_t ui_sl_{n} = {{")
                lines.append(f"    .location = {{{loc[0]}, {loc[1]}}},")
                lines.append(f"    .width = {p['width']},")
                lines.append(f"    .height = {p['height']},")
                lines.append(f"    .track_color = {color_name(p['track_color'])},")
                lines.append(f"    .thumb_color = {color_name(p['thumb_color'])},")
                lines.append(f"    .progress_color = {color_name(p['progress_color'])},")
                lines.append(f"    .min_value = {p['min_value']},")
                lines.append(f"    .max_value = {p['max_value']},")
                lines.append(f"    .current_value = {p['current_value']}")
                lines.append("};")
                lines.append(f"SCREEN_DrawSlider(&ui_sl_{n});")

            elif t == "listitem":
                lines.append(f"/* ListItem: {p['text']} */")
                lines.append(f"struUI_ListItem_t ui_item_{n} = {{")
                lines.append(f"    .location = {{{loc[0]}, {loc[1]}}},")
                lines.append(f"    .width = {p['width']},")
                lines.append(f"    .height = {p['height']},")
                lines.append(f"    .text = \"{p['text']}\",")
                lines.append(f"    .font = &{p['font']},")
                lines.append(f"    .bg_color = {color_name(p['bg_color'])},")
                lines.append(f"    .text_color = {color_name(p['text_color'])},")
                lines.append(f"    .border_color = {color_name(p['border_color'])},")
                lines.append(f"    .selected = {'true' if p['selected'] else 'false'},")
                lines.append(f"    .show_border = {'true' if p['show_border'] else 'false'}")
                lines.append("};")
                lines.append(f"SCREEN_DrawListItem(&ui_item_{n});")

            elif t == "rect_solid":
                w, h = p["frame"][0], p["frame"][1]
                x0, y0 = loc[0] - w // 2, loc[1] - h // 2
                x1, y1 = loc[0] + w // 2, loc[1] + h // 2
                lines.append(f"SCREEN_DrawRectSolid({x0}, {x1}, {y0}, {y1}, {c}, SCREEN_Nor);")

            elif t == "rect_hollow":
                w, h = p["frame"][0], p["frame"][1]
                x0, y0 = loc[0] - w // 2, loc[1] - h // 2
                x1, y1 = loc[0] + w // 2, loc[1] + h // 2
                lines.append(f"SCREEN_DrawRectHollow({x0}, {x1}, {y0}, {y1}, {c}, SCREEN_Nor);")

            elif t == "rrect_solid":
                w, h, r = p["frame"][0], p["frame"][1], p["frame"][2]
                x0, y0 = loc[0] - w // 2, loc[1] - h // 2
                x1, y1 = loc[0] + w // 2, loc[1] + h // 2
                lines.append(f"SCREEN_DrawRoundRectSolid({x0}, {x1}, {y0}, {y1}, {r}, {c}, SCREEN_Nor);")

            elif t == "rrect_hollow":
                w, h, r = p["frame"][0], p["frame"][1], p["frame"][2]
                x0, y0 = loc[0] - w // 2, loc[1] - h // 2
                x1, y1 = loc[0] + w // 2, loc[1] + h // 2
                lines.append(f"SCREEN_DrawRoundRectHollow({x0}, {x1}, {y0}, {y1}, {r}, {c}, SCREEN_Nor);")

            elif t == "line":
                ex, ey = p["end"]
                lines.append(f"SCREEN_DrawLine({loc[0]}, {ex}, {loc[1]}, {ey}, {c}, SCREEN_Nor);")

            elif t == "circle":
                lines.append(f"SCREEN_DrawQuarArc({loc[0]}, {loc[1]}, {p['radius']}, 0x{p['quadrant']:02X}, {c}, SCREEN_Nor);")

            elif t == "sector":
                lines.append(f"SCREEN_DrawQuarSector({loc[0]}, {loc[1]}, {p['radius']}, 0x{p['quadrant']:02X}, {c}, SCREEN_Nor);")

            elif t == "label":
                lines.append(f"SCREEN_DrawString({loc[0]}, {loc[1]}, \"{p['text']}\", &{p['ascii_font']}, {c}, SCREEN_Nor);")

            lines.append("")

        code = "\n".join(lines)
        win = tk.Toplevel(self.root)
        win.title("Exported C Code")
        win.geometry("580x500")

        txt = tk.Text(win, wrap=tk.NONE, font=("Consolas", 10))
        vsb = tk.Scrollbar(txt, orient="vertical", command=txt.yview)
        hsb = tk.Scrollbar(win, orient="horizontal", command=txt.xview)
        txt.config(yscrollcommand=vsb.set, xscrollcommand=hsb.set)
        vsb.pack(side=tk.RIGHT, fill=tk.Y)
        txt.pack(fill=tk.BOTH, expand=True, padx=4, pady=(4, 0))
        hsb.pack(fill=tk.X, padx=4)
        txt.insert("1.0", code)

        btn_frame = tk.Frame(win)
        btn_frame.pack(fill=tk.X, padx=4, pady=4)

        def _copy():
            self.root.clipboard_clear()
            self.root.clipboard_append(code)
            copy_btn.config(text="Copied!")

        copy_btn = tk.Button(btn_frame, text="Copy to Clipboard", command=_copy)
        copy_btn.pack(side=tk.LEFT, padx=4)

        def _save():
            from tkinter import filedialog
            path = filedialog.asksaveasfilename(defaultextension=".c",
                                                filetypes=[("C files", "*.c")])
            if path:
                with open(path, "w", encoding="utf-8") as f:
                    f.write(code)
                messagebox.showinfo("Saved", f"Saved to {path}")

        tk.Button(btn_frame, text="Save to File", command=_save).pack(side=tk.LEFT, padx=4)

    def run(self):
        self.root.mainloop()


# ---------------------------------------------------------------------------
if __name__ == "__main__":
    app = LCDUIDesignerApp()
    app.run()
