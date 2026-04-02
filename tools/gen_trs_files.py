#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import subprocess
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate .trs files next to each i18n .json file by invoking i18n_crypto_cli."
    )
    parser.add_argument("--cli", required=True, help="Path to i18n_crypto_cli executable")
    parser.add_argument("--i18n-dir", required=True, help="Directory containing i18n json files")
    parser.add_argument("--key", required=True, help="SM4-GCM key hex (32 hex chars)")
    parser.add_argument("--aad", default="", help="AAD text")
    return parser.parse_args()


def collect_json_files(i18n_dir: Path) -> list[Path]:
    files = sorted(i18n_dir.glob("*.json"))
    result = []
    for path in files:
        if path.name.endswith(".missing.template.json"):
            continue
        result.append(path)
    return result


def run_encrypt(cli: Path, json_file: Path, trs_file: Path, key: str, aad: str) -> int:
    cmd = [
        str(cli),
        "enc",
        "-i",
        str(json_file),
        "-o",
        str(trs_file),
        "-k",
        key,
        "--aad",
        aad,
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.stdout:
        print(proc.stdout.strip())
    if proc.returncode != 0:
        if proc.stderr:
            print(proc.stderr.strip(), file=sys.stderr)
        print(f"[ERROR] Failed to generate {trs_file.name} from {json_file.name}", file=sys.stderr)
    return proc.returncode


def main() -> int:
    args = parse_args()

    if len(args.key) != 32:
        print("[ERROR] --key must be 32 hex chars", file=sys.stderr)
        return 2

    cli = Path(args.cli)
    i18n_dir = Path(args.i18n_dir)

    if not cli.exists():
        print(f"[ERROR] cli not found: {cli}", file=sys.stderr)
        return 2
    if not i18n_dir.exists() or not i18n_dir.is_dir():
        print(f"[ERROR] i18n dir not found: {i18n_dir}", file=sys.stderr)
        return 2

    json_files = collect_json_files(i18n_dir)
    if not json_files:
        print("[OK] no i18n json files found")
        return 0

    has_error = False
    for json_file in json_files:
        trs_file = json_file.with_suffix(".trs")
        print(f"[INFO] Generating {trs_file.name} from {json_file.name}")
        ret = run_encrypt(cli, json_file, trs_file, args.key, args.aad)
        if ret != 0:
            has_error = True

    return 1 if has_error else 0


if __name__ == "__main__":
    sys.exit(main())
