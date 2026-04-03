#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import json
import re
from pathlib import Path
import sys

PREFIX = "I18N_"  # 数字开头的 key 前缀

def normalize_key(key: str) -> str:
    """
    将 JSON key 转换为合法 C++ enum 名：
    - 非字母数字替换为下划线
    - 全部大写
    - 数字开头加前缀
    """
    norm = re.sub(r'[^a-zA-Z0-9]', '_', key).upper()
    if norm[0].isdigit():
        norm = PREFIX + norm
    return norm

def flatten_json(data: dict, prefix: str = "") -> dict:
    """
    递归扁平化嵌套 JSON，用 '.' 连接路径。
    仅叶子节点（字符串值）被保留。
    """
    result = {}
    for k, v in data.items():
        flat_key = f"{prefix}.{k}" if prefix else k
        if isinstance(v, dict):
            result.update(flatten_json(v, flat_key))
        elif isinstance(v, str):
            result[flat_key] = v
        else:
            print(f"[WARN] skip non-string value: {flat_key} = {v!r}")
    return result

def generate_header(json_file: str, output_file: str):
    json_path = Path(json_file)
    if not json_path.exists():
        print(f"Error: {json_file} 不存在")
        sys.exit(1)

    data = json.loads(json_path.read_text(encoding='utf-8'))
    flat = flatten_json(data)

    normalized_keys = {}
    for k in flat.keys():
        nk = normalize_key(k)
        if nk in normalized_keys:
            print(f"[ERROR] enum conflict: {k} and {normalized_keys[nk]} both map to {nk}")
            sys.exit(1)
        normalized_keys[nk] = k

    lines = []
    lines.append("// This file is auto-generated. Do not modify manually.\n")
    lines.append("#pragma once\n\n")

    # 生成 enum（Allman 风格，4 空格缩进）
    lines.append("enum class I18nKey : unsigned short\n")
    lines.append("{\n")
    for nk in sorted(normalized_keys.keys()):
        lines.append(f"    {nk},\n")
    lines.append("};\n\n")

    # 生成 to_string 映射函数
    lines.append("inline constexpr const char* i18n_keys_string(I18nKey key)\n")
    lines.append("{\n")
    lines.append("    switch (key)\n")
    lines.append("    {\n")
    for nk, original in sorted(normalized_keys.items()):
        lines.append(f"        case I18nKey::{nk}:\n")
        lines.append(f"            return \"{original}\";\n")
    lines.append("        default:\n")
    lines.append("            return \"\";\n")
    lines.append("    }\n")
    lines.append("}\n\n")

    # 生成 key 数量常量
    lines.append(f"inline constexpr unsigned short I18N_KEY_COUNT = {len(normalized_keys)};\n")

    # 写入文件
    output_path = Path(output_file)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text("".join(lines), encoding='utf-8')
    print(f"[OK] Header generated: {output_file}")

if __name__ == "__main__":
    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parent

    input_json = sys.argv[1] if len(sys.argv) > 1 else str(project_root / "i18n" / "en_US.json")
    output_header = sys.argv[2] if len(sys.argv) > 2 else str(project_root / "generated" / "i18n_keys.h")
    generate_header(input_json, output_header)