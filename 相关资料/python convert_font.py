#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import re

def chinese_to_utf8_hex(chinese_char):
    """将中文字符转换为UTF-8十六进制编码"""
    utf8_bytes = chinese_char.encode('utf-8')
    hex_list = [f"0x{b:02X}" for b in utf8_bytes]
    return hex_list

def parse_mod_output(file_path):
    """解析取模软件输出格式（支持多行字模数据）"""
    with open(file_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    # 解析所有数据行（包括带注释和不带注释的）
    # 格式1: {0x10,0x01,...},
    # 格式2: {0x89,0x14,...},/*"你",0*/
    # 先找出所有带注释的行
    result = []
    for i, line in enumerate(lines):
        line = line.strip()
        # 匹配带注释的行: {...},/*"字符",索引*/
        comment_pattern = r'\{([^}]+)\},/\*"([^"]+)",(\d+)\*/'
        comment_match = re.search(comment_pattern, line)
        if comment_match:
            second_half = comment_match.group(1).replace(' ', '').split(',')
            char = comment_match.group(2)
            idx = comment_match.group(3)

            # 检查前一行是否也是数据行（第一行）
            if i > 0:
                prev_line = lines[i-1].strip()
                data_pattern = r'\{([^}]+)\},'
                data_match = re.search(data_pattern, prev_line)
                if data_match:
                    first_half = data_match.group(1).replace(' ', '').split(',')
                    combined_data = first_half + second_half
                    result.append({
                        'char': char,
                        'index': int(idx),
                        'data': combined_data
                    })

    # 按字符Unicode值排序
    result.sort(key=lambda x: ord(x['char']))

    return result

def generate_c_code(parsed_data, font_name="YuMincho", font_size="16x16"):
    """生成C语言格式代码"""
    c_code = []

    # 获取字模数据大小（从第一个字获取）
    if parsed_data:
        data_size = len(parsed_data[0]['data'])
        # 计算宽和高（从font_size解析）
        try:
            width, height = map(int, font_size.split('x'))
            comment_size = f"{width}*{height}"
        except:
            width = height = int((data_size * 2) ** 0.5) if data_size > 0 else 16
            comment_size = f"{width}*{height}"
        c_code.append(f"// {comment_size} UTF-8字体数据\n")
    else:
        data_size = 32
        c_code.append(f"// UTF-8字体数据\n")

    # 生成字模数据数组
    array_names = []
    utf8_list = []

    for item in parsed_data:
        char = item['char']
        utf8_hex = chinese_to_utf8_hex(char)
        utf8_str = ''.join([h.replace('0x', '') for h in utf8_hex])
        array_name = f"UTF_{font_size}_{font_name}_{utf8_str}"
        array_names.append(array_name)
        utf8_list.append({
            'char': char,
            'utf8_hex': utf8_hex,
            'array_name': array_name
        })

        # 生成字模数组，每行16个字节
        c_code.append(f"static const uint8_t {array_name}[{data_size}] = {{")
        for i in range(0, data_size, 16):
            chunk = item['data'][i:i+16]
            if i == 0:
                c_code.append(f"    {','.join(chunk)},")
            elif i + 16 >= data_size:
                c_code.append(f"    {','.join(chunk)}")
            else:
                c_code.append(f"    {','.join(chunk)},")
        c_code.append(f"}}; // \"{char}\"\n")

    # 生成UTF-8编码表
    c_code.append(f"// {font_size} UTF-8编码表\n")
    c_code.append(f"const struFont_UTF_data_t Font_UTF_{font_size}_{font_name}_data[] = {{\n")
    c_code.append("\t// UTF-8编码, 字模数据指针\n")

    for utf8_info in utf8_list:
        utf8_comment = ' '.join(utf8_info['utf8_hex'])
        utf8_code = ', '.join(utf8_info['utf8_hex'])
        c_code.append(f"\t{{{{{utf8_code}}}, {utf8_info['array_name']}}},   // \"{utf8_info['char']}\" UTF-8: {utf8_comment}\n")

    # 表尾标记
    c_code.append("\t{{0x00, 0x00, 0x00}, NULL}            // 表尾标记\n")
    c_code.append("};\n")

    return ''.join(c_code)

def main():
    # 用户输入参数
    input_file = input("请输入字模文件名（默认UTF.txt）: ").strip() or "UTF.txt"
    font_name = input("请输入字体名字（默认YuMincho）: ").strip() or "YuMincho"
    font_size = input("请输入字体大小（默认16x16）: ").strip() or "16x16"

    print(f"\n解析取模软件输出文件 {input_file}...")
    parsed_data = parse_mod_output(input_file)
    print(f"解析到 {len(parsed_data)} 个汉字字模")

    # 显示字模数据大小
    if parsed_data:
        data_size = len(parsed_data[0]['data'])
        print(f"字模数组大小: {data_size} 字节\n")

    print("生成C语言代码...")
    c_code = generate_c_code(parsed_data, font_name, font_size)

    print("=" * 60)
    print(c_code)
    print("=" * 60)

    # 保存到文件
    output_file = "font_output.h"
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(c_code)
    print(f"\n已保存到 {output_file}")

if __name__ == "__main__":
    main()
