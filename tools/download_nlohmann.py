#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
下载 nlohmann/json 单文件 header。
优先从 Gitee 镜像下载，失败后回退到 GitHub。

用法: python download_nlohmann.py <output_dir> [version]
  output_dir  — 输出目录，json.hpp 将放在 <output_dir>/nlohmann/json.hpp
  version     — 版本号，默认 v3.11.3
"""

import sys
import os
import hashlib
from pathlib import Path
from urllib.request import urlopen, Request
from urllib.error import URLError

DEFAULT_VERSION = "v3.11.3"

EXPECTED_SHA256 = {
    "v3.11.3": "853b2eb3bf7632c949f4a2dd6fb8a629c5ff25cc0059d29c7d21915dc719487f",
}

SOURCES = [
    ("Gitee",  "https://gitee.com/nicethink/json/raw/{ver}/single_include/nlohmann/json.hpp"),
    ("GitHub", "https://raw.githubusercontent.com/nlohmann/json/{ver}/single_include/nlohmann/json.hpp"),
]

TIMEOUT = 30


def download(output_dir: str, version: str):
    dest = Path(output_dir) / "nlohmann" / "json.hpp"
    if dest.exists():
        print(f"[SKIP] {dest} already exists")
        return

    dest.parent.mkdir(parents=True, exist_ok=True)

    for name, url_tpl in SOURCES:
        url = url_tpl.format(ver=version)
        print(f"[INFO] Trying {name}: {url}")
        try:
            req = Request(url, headers={"User-Agent": "i18n_core_trs/cmake"})
            with urlopen(req, timeout=TIMEOUT) as resp:
                data = resp.read()
            expected = EXPECTED_SHA256.get(version)
            if expected:
                actual = hashlib.sha256(data).hexdigest()
                if actual != expected:
                    print(f"[ERROR] SHA256 mismatch for {name}: expected {expected}, got {actual}")
                    continue
            dest.write_bytes(data)
            print(f"[OK]   Downloaded {len(data)} bytes -> {dest}")
            return
        except (URLError, OSError) as e:
            print(f"[WARN] {name} failed: {e}")

    print("[ERROR] All sources failed", file=sys.stderr)
    sys.exit(1)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <output_dir> [version]")
        sys.exit(1)
    out_dir = sys.argv[1]
    ver = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_VERSION
    download(out_dir, ver)
