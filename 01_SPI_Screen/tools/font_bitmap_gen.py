#!/usr/bin/env python3
"""
Font Bitmap Generator - 字体取模工具 (替代 PCtoLCD2002)
从 TTF/OTF 字体文件生成嵌入式 LCD 用的 C 字模数组

依赖: pip install Pillow
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from PIL import Image, ImageDraw, ImageFont
import os
import sys

# ---------------------------------------------------------------------------
# 取模引擎
# ---------------------------------------------------------------------------
SCAN_ROW    = "逐行式"
SCAN_COL    = "逐列式"
SCAN_COLROW = "列行式"

BIT_LSB = "低位在前 (LSB)"
BIT_MSB = "高位在前 (MSB)"


def render_char_bitmap(font, char, width, height):
    """用 Pillow 将单个字符渲染为 1-bit 位图，返回 list[list[int]] (0/1)"""
    img = Image.new("1", (width, height), 0)
    draw = ImageDraw.Draw(img)

    bbox = draw.textbbox((0, 0), char, font=font)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    ox = (width - tw) // 2 - bbox[0]
    oy = (height - th) // 2 - bbox[1]
    draw.text((ox, oy), char, fill=1, font=font)

    raw_data = img.load()
    return [[1 if raw_data[x, y] else 0 for x in range(width)] for y in range(height)]


def bitmap_to_bytes(pixels, width, height, scan_mode, bit_order):
    """将 0/1 像素矩阵按指定取模方式转换为字节数组"""
    bytes_per_row = (width + 7) // 8
    is_lsb = (bit_order == BIT_LSB)

    if scan_mode == SCAN_ROW:
        result = []
        for y in range(height):
            row = pixels[y]
            for byte_idx in range(bytes_per_row):
                val = 0
                base = byte_idx * 8
                for bit in range(8):
                    x = base + bit
                    if x < width and row[x]:
                        val |= 1 << (bit if is_lsb else (7 - bit))
                result.append(val)
        return result

    elif scan_mode == SCAN_COL:
        bytes_per_col = (height + 7) // 8
        result = []
        for x in range(width):
            for byte_idx in range(bytes_per_col):
                val = 0
                base = byte_idx * 8
                for bit in range(8):
                    y = base + bit
                    if y < height and pixels[y][x]:
                        val |= 1 << (bit if is_lsb else (7 - bit))
                result.append(val)
        return result

    elif scan_mode == SCAN_COLROW:
        col_groups = (height + 7) // 8
        total = width * col_groups
        result = [0] * total
        for x in range(width):
            offset = x * col_groups
            for y in range(height):
                if pixels[y][x]:
                    idx = offset + (y >> 3)
                    result[idx] |= 1 << ((y & 7) if is_lsb else (7 - (y & 7)))
        return result

    return []


def generate_ascii_range(font, width, height, scan_mode, bit_order,
                         start=32, end=126):
    """生成 ASCII 字符范围的字模数据，返回 dict {char_index: bytes}"""
    data = {}
    for i in range(start, end + 1):
        ch = chr(i)
        pixels = render_char_bitmap(font, ch, width, height)
        raw = bitmap_to_bytes(pixels, width, height, scan_mode, bit_order)
        data[i - start] = {"char": ch, "pixels": pixels, "bytes": raw}
    return data


def generate_utf_chars(font, width, height, scan_mode, bit_order, chars):
    """生成 UTF-8 汉字的字模数据，返回 list[{char, utf8_bytes, pixels, data}]"""
    results = []
    seen = set()
    for ch in chars:
        if ch in seen:
            continue
        seen.add(ch)
        utf8 = ch.encode("utf-8")
        if len(utf8) > 3:
            continue
        pixels = render_char_bitmap(font, ch, width, height)
        raw = bitmap_to_bytes(pixels, width, height, scan_mode, bit_order)
        results.append({
            "char": ch,
            "utf8": utf8,
            "pixels": pixels,
            "bytes": raw,
        })
    return results


# ---------------------------------------------------------------------------
# C 代码生成
# ---------------------------------------------------------------------------
def fmt_c_array(data, items_per_line=16):
    """将字节数组格式化为 C 风格的逗号分隔 hex 字符串"""
    hexvals = [f"0x{b:02X}" for b in data]
    return ",".join(hexvals)


def generate_ascii_c_code(var_prefix, width, height, bytes_per_row, char_data,
                           font_name="font"):
    """生成 ASCII 字体的完整 C 代码（格式与 fonts.c 一致）"""
    lines = []
    total_chars = len(char_data)
    data_size = height * bytes_per_row

    lines.append(f"/* {width}x{height} {font_name}字体数据 */")
    lines.append(f"const uint8_t {var_prefix}_data[{total_chars}][{data_size}] = ")
    lines.append("{")
    for i in range(total_chars):
        if i not in char_data:
            row = ",".join(["0x00"] * data_size)
            lines.append("{" + row + "},")
            continue
        ch = char_data[i]["char"]
        raw = char_data[i]["bytes"]
        row = ",".join(f"0x{b:02X}" for b in raw)
        lines.append("{" + row + "},/*\"" + ch + "\"," + str(ord(ch)) + "*/")
        lines.append("")
    lines.append("};")
    lines.append("")
    lines.append(f"const struFont_t {var_prefix} = {{{width}, {height}, {bytes_per_row}, (uint8_t*){var_prefix}_data}};")

    return "\n".join(lines)


def generate_utf_c_code(var_prefix, width, height, bytes_per_row, utf_data):
    """生成 UTF-8 汉字字体的完整 C 代码（格式与 fonts.c 一致）"""
    if not utf_data:
        return "/* No UTF-8 characters */"

    lines = []
    data_size = height * bytes_per_row

    # 每个汉字的独立数组
    # 按 Unicode 编码排序
    sorted_data = sorted(utf_data, key=lambda x: ord(x["char"]))

    lines.append(f"/* {width}*{height} UTF-8字体数据 */")
    for item in sorted_data:
        ch = item["char"]
        utf8 = item["utf8"]
        raw = item["bytes"]
        hex_name = var_prefix + "_" + utf8.hex().upper()
        hexvals = ",".join(f"0x{b:02X}" for b in raw)
        # 按 data_size/2 分成两半
        half = data_size // 2
        vals = [f"0x{b:02X}" for b in raw]
        left = ",".join(vals[:half])
        right = ",".join(vals[half:])
        lines.append(
            f"static const uint8_t {hex_name}[{data_size}] = {{    {left},    {right}}}; // \"{ch}\""
        )

    lines.append("")

    # 编码查找表
    lines.append(f"/* {width}x{height} UTF-8编码表 */")
    lines.append(f"const struFont_UTF_data_t {var_prefix}_data[] = {{")
    lines.append("    /* UTF-8编码, 字模数据指针 */")
    for item in sorted_data:
        ch = item["char"]
        utf8 = item["utf8"]
        hex_name = var_prefix + "_" + utf8.hex().upper()
        hex_bytes = ", ".join(f"0x{b:02X}" for b in utf8)
        utf_str = " ".join(f"0x{b:02X}" for b in utf8)
        lines.append(f'    {{{{{hex_bytes}}}, {hex_name}}},   // "{ch}" UTF-8: {utf_str}')
    lines.append("    {{0x00, 0x00, 0x00}, NULL}            // 表尾标记")
    lines.append("};")
    lines.append("")

    # 结构体
    lines.append(f"const struFont_UTF_t {var_prefix} = {{{width}, {height}, {bytes_per_row}, {var_prefix}_data}};")

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# GUI 应用
# ---------------------------------------------------------------------------
class FontGenApp:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Font Bitmap Generator - 字体取模工具")
        self.root.minsize(1050, 700)

        self._current_font = None
        self._ascii_data = {}
        self._utf_data = []
        self._preview_char_index = 0
        self._preview_mode = "ascii"  # "ascii" or "utf"
        self._pixel_scale = 10

        self._build_ui()
        self._load_default_font()

    def _build_ui(self):
        # ===== 上方：设置面板 =====
        settings = ttk.LabelFrame(self.root, text="Font Settings", padding=6)
        settings.pack(fill=tk.X, padx=6, pady=(6, 2))

        # 第一行：字体文件 + 尺寸
        r = 0
        ttk.Label(settings, text="System Font:").grid(row=r, column=0, sticky="w", padx=2)
        self.sys_font_var = tk.StringVar()
        self.sys_font_combo = ttk.Combobox(settings, textvariable=self.sys_font_var,
                                           width=32, state="readonly")
        self.sys_font_combo.grid(row=r, column=1, columnspan=2, sticky="w", padx=2)
        self.sys_font_combo.bind("<<ComboboxSelected>>", self._on_sys_font_select)
        self._scan_system_fonts()

        r += 1
        ttk.Label(settings, text="Font File:").grid(row=r, column=0, sticky="w", padx=2)
        self.font_path_var = tk.StringVar()
        ttk.Entry(settings, textvariable=self.font_path_var, width=50).grid(
            row=r, column=1, columnspan=3, sticky="w", padx=2)
        ttk.Button(settings, text="Browse...", command=self._browse_font).grid(
            row=r, column=4, padx=4)
        ttk.Button(settings, text="Load", command=self._load_font).grid(
            row=r, column=5, padx=2)

        r += 1
        ttk.Label(settings, text="Size:").grid(row=r, column=0, sticky="w", padx=2)
        self.width_var = tk.IntVar(value=16)
        ttk.Spinbox(settings, from_=4, to=64, textvariable=self.width_var, width=4).grid(
            row=r, column=1, sticky="w", padx=2)
        ttk.Label(settings, text="x").grid(row=r, column=1, sticky="e")
        self.height_var = tk.IntVar(value=16)
        ttk.Spinbox(settings, from_=4, to=64, textvariable=self.height_var, width=4).grid(
            row=r, column=2, sticky="w", padx=2)

        ttk.Label(settings, text="Scan:").grid(row=r, column=3, sticky="e", padx=2)
        self.scan_var = tk.StringVar(value=SCAN_ROW)
        ttk.Combobox(settings, textvariable=self.scan_var, values=[SCAN_ROW, SCAN_COL, SCAN_COLROW],
                      width=10, state="readonly").grid(row=r, column=4, padx=2)

        ttk.Label(settings, text="Bit:").grid(row=r, column=5, sticky="e", padx=2)
        self.bit_var = tk.StringVar(value=BIT_LSB)
        ttk.Combobox(settings, textvariable=self.bit_var, values=[BIT_LSB, BIT_MSB],
                      width=14, state="readonly").grid(row=r, column=6, padx=2)

        # 第二行：字符输入
        r += 1
        ttk.Label(settings, text="ASCII Range:").grid(row=r, column=0, sticky="w", padx=2)
        self.ascii_start_var = tk.IntVar(value=32)
        ttk.Spinbox(settings, from_=0, to=127, textvariable=self.ascii_start_var, width=4).grid(
            row=r, column=1, sticky="w", padx=2)
        ttk.Label(settings, text="~").grid(row=r, column=1, sticky="e")
        self.ascii_end_var = tk.IntVar(value=126)
        ttk.Spinbox(settings, from_=0, to=127, textvariable=self.ascii_end_var, width=4).grid(
            row=r, column=2, sticky="w", padx=2)

        ttk.Label(settings, text="UTF-8 Text:").grid(row=r, column=3, sticky="e", padx=2)
        self.utf_text_var = tk.StringVar()
        ttk.Entry(settings, textvariable=self.utf_text_var, width=24).grid(
            row=r, column=4, columnspan=2, sticky="w", padx=2)

        ttk.Button(settings, text="Generate", command=self._generate).grid(
            row=r, column=6, padx=6)

        # ===== 中部：预览 + 字符列表 =====
        mid = ttk.Frame(self.root)
        mid.pack(fill=tk.BOTH, expand=True, padx=6, pady=2)

        # 左侧：字符列表
        list_frame = ttk.LabelFrame(mid, text="Characters", padding=4)
        list_frame.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 4))

        self.char_listbox = tk.Listbox(list_frame, width=14, height=20, font=("Consolas", 9))
        char_scroll = ttk.Scrollbar(list_frame, orient="vertical", command=self.char_listbox.yview)
        self.char_listbox.config(yscrollcommand=char_scroll.set)
        self.char_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        char_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.char_listbox.bind("<<ListboxSelect>>", self._on_char_select)

        # 中间：位图预览
        preview_frame = ttk.LabelFrame(mid, text="Bitmap Preview", padding=4)
        preview_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=4)

        self.preview_canvas = tk.Canvas(preview_frame, bg="#2b2b2b", width=260, height=260)
        self.preview_canvas.pack(fill=tk.BOTH, expand=True)

        info_frame = ttk.Frame(preview_frame)
        info_frame.pack(fill=tk.X, pady=(4, 0))
        self.preview_info = ttk.Label(info_frame, text="Select a character to preview")
        self.preview_info.pack(side=tk.LEFT)
        self.preview_hex = ttk.Label(info_frame, text="", font=("Consolas", 9))
        self.preview_hex.pack(side=tk.RIGHT)

        # 右侧：字节数据预览
        byte_frame = ttk.LabelFrame(mid, text="Byte Data", padding=4)
        byte_frame.pack(side=tk.RIGHT, fill=tk.Y, padx=(4, 0))

        self.byte_text = tk.Text(byte_frame, width=36, height=24, font=("Consolas", 9),
                                 bg="#1e1e1e", fg="#d4d4d4", wrap=tk.NONE)
        self.byte_text.pack(fill=tk.BOTH, expand=True)

        # ===== 下方：C 代码输出 =====
        output_frame = ttk.LabelFrame(self.root, text="Generated C Code", padding=4)
        output_frame.pack(fill=tk.BOTH, expand=True, padx=6, pady=(2, 6))

        code_container = ttk.Frame(output_frame)
        code_container.pack(fill=tk.BOTH, expand=True)

        self.code_text = tk.Text(code_container, height=14, font=("Consolas", 10),
                                 bg="#1e1e1e", fg="#d4d4d4", wrap=tk.NONE)
        code_ysb = ttk.Scrollbar(code_container, orient="vertical", command=self.code_text.yview)
        code_xsb = ttk.Scrollbar(output_frame, orient="horizontal", command=self.code_text.xview)
        self.code_text.config(yscrollcommand=code_ysb.set, xscrollcommand=code_xsb.set)
        code_ysb.pack(side=tk.RIGHT, fill=tk.Y)
        self.code_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        code_xsb.pack(fill=tk.X)

        btn_frame = ttk.Frame(output_frame)
        btn_frame.pack(fill=tk.X, pady=(4, 0))

        self.var_name_var = tk.StringVar(value="Font_Custom")
        ttk.Label(btn_frame, text="Var Name:").pack(side=tk.LEFT, padx=4)
        ttk.Entry(btn_frame, textvariable=self.var_name_var, width=20).pack(side=tk.LEFT, padx=2)

        self.gen_type_var = tk.StringVar(value="ASCII")
        ttk.Radiobutton(btn_frame, text="ASCII", variable=self.gen_type_var, value="ASCII").pack(side=tk.LEFT, padx=8)
        ttk.Radiobutton(btn_frame, text="UTF-8", variable=self.gen_type_var, value="UTF-8").pack(side=tk.LEFT, padx=4)

        ttk.Button(btn_frame, text="Generate Code", command=self._gen_code).pack(side=tk.LEFT, padx=12)

        def _copy():
            self.root.clipboard_clear()
            self.root.clipboard_append(self.code_text.get("1.0", tk.END))
            copy_btn.config(text="Copied!")
        copy_btn = ttk.Button(btn_frame, text="Copy All", command=_copy)
        copy_btn.pack(side=tk.LEFT, padx=4)

        def _save():
            path = filedialog.asksaveasfilename(defaultextension=".c",
                                                filetypes=[("C files", "*.c"), ("Header files", "*.h")])
            if path:
                with open(path, "w", encoding="utf-8") as f:
                    f.write(self.code_text.get("1.0", tk.END))
                messagebox.showinfo("Saved", f"Saved to {path}")
        ttk.Button(btn_frame, text="Save to File", command=_save).pack(side=tk.LEFT, padx=4)

    def _scan_system_fonts(self):
        """扫描系统字体目录，构建 {显示名: 文件路径} 字典"""
        self._sys_font_map = {}  # {display_name: full_path}
        font_dirs = []
        if sys.platform == "win32":
            font_dirs.append(os.path.join(os.environ.get("SystemRoot", r"C:\Windows"), "Fonts"))
            local_font = os.path.join(os.environ.get("LOCALAPPDATA", ""), "Microsoft", "Windows", "Fonts")
            if os.path.isdir(local_font):
                font_dirs.append(local_font)
        else:
            font_dirs.extend(["/usr/share/fonts", "/usr/local/share/fonts",
                              os.path.expanduser("~/.fonts"), os.path.expanduser("~/.local/share/fonts")])

        exts = {".ttf", ".ttc", ".otf"}
        for d in font_dirs:
            if not os.path.isdir(d):
                continue
            try:
                for f in os.listdir(d):
                    base, ext = os.path.splitext(f)
                    if ext.lower() in exts:
                        full = os.path.join(d, f)
                        display = f"{base} ({ext[1:].upper()})"
                        # 去重：同名保留先找到的
                        if display not in self._sys_font_map:
                            self._sys_font_map[display] = full
            except OSError:
                pass

        sorted_names = sorted(self._sys_font_map.keys())
        self.sys_font_combo["values"] = sorted_names

    def _on_sys_font_select(self, event):
        """从系统字体下拉框选择字体"""
        name = self.sys_font_var.get()
        path = self._sys_font_map.get(name)
        if path:
            self.font_path_var.set(path)
            self._load_font()

    def _load_default_font(self):
        """尝试加载系统默认字体"""
        defaults = [
            "C:/Windows/Fonts/consola.ttf",
            "C:/Windows/Fonts/simsun.ttc",
            "C:/Windows/Fonts/simhei.ttf",
            "C:/Windows/Fonts/msyh.ttc",
            "C:/Windows/Fonts/simkai.ttf",
            "C:/Windows/Fonts/msgothic.ttc",
            "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        ]
        for p in defaults:
            if os.path.isfile(p):
                self.font_path_var.set(p)
                self._load_font()
                return

    def _browse_font(self):
        path = filedialog.askopenfilename(
            filetypes=[("Font files", "*.ttf *.otf *.ttc"), ("All files", "*.*")])
        if path:
            self.font_path_var.set(path)
            self._load_font()

    def _load_font(self):
        path = self.font_path_var.get()
        if not path or not os.path.isfile(path):
            messagebox.showwarning("Font", "Please select a valid font file.")
            return
        try:
            size = self.height_var.get()
            self._current_font = ImageFont.truetype(path, size)
            self._set_status(f"Loaded: {os.path.basename(path)}")
        except Exception as e:
            messagebox.showerror("Font Error", str(e))

    def _set_status(self, text):
        self.preview_info.config(text=text)

    def _generate(self):
        if not self._current_font:
            self._load_font()
            if not self._current_font:
                return

        width = self.width_var.get()
        height = self.height_var.get()
        scan = self.scan_var.get()
        bit = self.bit_var.get()

        # 重新加载字体（尺寸可能变了）
        try:
            path = self.font_path_var.get()
            self._current_font = ImageFont.truetype(path, height)
        except Exception:
            pass

        # ASCII
        start = self.ascii_start_var.get()
        end = self.ascii_end_var.get()
        if start > end:
            start, end = end, start
        self._ascii_data = generate_ascii_range(self._current_font, width, height, scan, bit, start, end)

        # UTF-8
        utf_text = self.utf_text_var.get()
        if utf_text.strip():
            self._utf_data = generate_utf_chars(self._current_font, width, height, scan, bit, utf_text)
        else:
            self._utf_data = []

        # 填充字符列表
        self.char_listbox.delete(0, tk.END)
        for i in sorted(self._ascii_data.keys()):
            ch = self._ascii_data[i]["char"]
            label = f"{i:3d}  {ch!r}"
            self.char_listbox.insert(tk.END, label)
        for idx, item in enumerate(self._utf_data):
            self.char_listbox.insert(tk.END, f"UTF  {item['char']}")

        count = len(self._ascii_data) + len(self._utf_data)
        self._set_status(f"Generated {count} chars ({width}x{height}, {scan}, {bit})")

        # 自动选中第一个
        if self.char_listbox.size() > 0:
            self.char_listbox.selection_set(0)
            self._preview_index(0)

    def _on_char_select(self, event):
        sel = self.char_listbox.curselection()
        if sel:
            self._preview_index(sel[0])

    def _preview_index(self, idx):
        ascii_count = len(self._ascii_data)
        if idx < ascii_count:
            self._preview_mode = "ascii"
            self._preview_char_index = idx
            item = self._ascii_data[idx]
        else:
            self._preview_mode = "utf"
            utf_idx = idx - ascii_count
            if utf_idx >= len(self._utf_data):
                return
            self._preview_char_index = utf_idx
            item = self._utf_data[utf_idx]

        pixels = item["pixels"]
        raw = item["bytes"]
        width = len(pixels[0]) if pixels else 0
        height = len(pixels)
        char_display = item["char"]

        self._draw_preview(pixels, width, height)
        self.preview_info.config(text=f"'{char_display}' ({width}x{height}, {len(raw)} bytes)")

        # 显示字节数据
        self.byte_text.delete("1.0", tk.END)
        for i, b in enumerate(raw):
            if i > 0 and i % width == 0:
                self.byte_text.insert(tk.END, "\n")
            self.byte_text.insert(tk.END, f"0x{b:02X} ")

    def _draw_preview(self, pixels, width, height):
        self.preview_canvas.delete("all")
        canvas_w = self.preview_canvas.winfo_width() or 260
        canvas_h = self.preview_canvas.winfo_height() or 260
        scale = min((canvas_w - 20) // max(width, 1), (canvas_h - 20) // max(height, 1), 20)
        scale = max(scale, 3)
        self._pixel_scale = scale

        ox = (canvas_w - width * scale) // 2
        oy = (canvas_h - height * scale) // 2

        # 背景
        self.preview_canvas.create_rectangle(ox, oy, ox + width * scale, oy + height * scale,
                                             fill="#000000", outline="#555")

        # 像素
        for y in range(height):
            for x in range(width):
                x1 = ox + x * scale
                y1 = oy + y * scale
                color = "#00FF00" if pixels[y][x] else "#1a1a1a"
                self.preview_canvas.create_rectangle(x1, y1, x1 + scale, y1 + scale,
                                                     fill=color, outline="#333", width=1)

        # 尺寸标注
        self.preview_canvas.create_text(ox, oy - 6, text=f"0", fill="#888",
                                        anchor="sw", font=("", 7))
        self.preview_canvas.create_text(ox + width * scale, oy - 6,
                                        text=f"{width - 1}", fill="#888",
                                        anchor="se", font=("", 7))

    def _gen_code(self):
        if not self._ascii_data and not self._utf_data:
            messagebox.showinfo("Code", "Generate font data first.")
            return

        width = self.width_var.get()
        height = self.height_var.get()
        var = self.var_name_var.get()
        gen_type = self.gen_type_var.get()

        code_parts = []
        scan_desc = self.scan_var.get()
        bit_desc = "低位在前" if self.bit_var.get() == BIT_LSB else "高位在前"
        code_parts.append(f"/* 取模{bit_desc} {scan_desc} 阴码 */")
        code_parts.append("")

        if gen_type == "ASCII" and self._ascii_data:
            bytes_per_row = (width + 7) // 8
            code = generate_ascii_c_code(var, width, height, bytes_per_row, self._ascii_data)
            code_parts.append(code)
        elif gen_type == "UTF-8" and self._utf_data:
            bytes_per_row = (width + 7) // 8
            code = generate_utf_c_code(var, width, height, bytes_per_row, self._utf_data)
            code_parts.append(code)
        else:
            messagebox.showinfo("Code", f"No {gen_type} data to generate.")
            return

        self.code_text.delete("1.0", tk.END)
        self.code_text.insert("1.0", "\n".join(code_parts))

    def run(self):
        self.root.mainloop()


# ---------------------------------------------------------------------------
if __name__ == "__main__":
    app = FontGenApp()
    app.run()
