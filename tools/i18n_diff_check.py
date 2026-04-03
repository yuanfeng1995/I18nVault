#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import json
import os
import re
import sys
from pathlib import Path


def parse_args(argv):
    strict_mode = "--strict" in argv
    github_annotations = "--github-annotations" in argv
    args = [a for a in argv if a not in ("--strict", "--github-annotations")]

    if len(args) < 2:
        print("Usage: python i18n_diff_check.py <base.json> [target1.json target2.json ...] [--strict] [--github-annotations]")
        print("Example: python i18n_diff_check.py en_US.json zh_CN.json fr_FR.json --strict --github-annotations")
        sys.exit(1)

    base_file = args[1]
    target_files = args[2:]
    return base_file, target_files, strict_mode, github_annotations


def is_github_actions_env():
    return os.getenv("GITHUB_ACTIONS", "").lower() == "true"


def escape_github_annotation(text):
    return str(text).replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")


def emit_github_annotation(level, file_path, line, message, enabled):
    if not enabled:
        return
    safe_msg = escape_github_annotation(message)
    safe_file = escape_github_annotation(Path(file_path).as_posix())
    print(f"::{level} file={safe_file},line={line}::{safe_msg}")


def find_key_line(path, key):
    # Best-effort line mapping: locate the key token in target JSON text.
    pattern = re.compile(rf'"{re.escape(key)}"\s*:')
    text = Path(path).read_text(encoding="utf-8")
    for idx, line in enumerate(text.splitlines(), start=1):
        if pattern.search(line):
            return idx
    return 1

def load_json(path):
    p = Path(path)
    if not p.exists():
        print(f"[ERROR] File not found: {path}")
        sys.exit(1)
    try:
        return json.loads(p.read_text(encoding='utf-8'))
    except Exception as e:
        print(f"[ERROR] JSON parse failed: {path}\n{e}")
        sys.exit(1)

def diff_keys(base, target):
    base_keys = set(base.keys())
    target_keys = set(target.keys())

    missing = base_keys - target_keys     # target 缺少
    extra = target_keys - base_keys       # target 多出

    return missing, extra

def check_value_type(base, target):
    mismatch = []
    for k in base.keys():
        if k in target:
            if type(base[k]) != type(target[k]):
                mismatch.append(k)
    return mismatch


def discover_targets(base_file):
    base_path = Path(base_file).resolve()
    i18n_dir = base_path.parent
    all_json = sorted(p.resolve() for p in i18n_dir.glob("*.json"))
    return [str(p) for p in all_json if p != base_path and not p.name.endswith(".missing.template.json")]


def write_missing_template(base, target_file, missing_keys):
    target_path = Path(target_file).resolve()
    template_path = target_path.with_name(f"{target_path.stem}.missing.template.json")

    # Use base locale values in the template for quick translation fill-in.
    template_data = {k: base[k] for k in sorted(missing_keys)}
    template_path.write_text(
        json.dumps(template_data, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    return template_path


def check_single_target(base, base_file, target_file, strict_mode, github_annotations):
    target = load_json(target_file)

    missing, extra = diff_keys(base, target)
    type_mismatch = check_value_type(base, target)

    has_error = False

    print(f"\n===== i18n Diff Report: {target_file} =====")

    # Missing keys always fail.
    if missing:
        print(f"\n[ERROR] Missing keys in {target_file}:")
        for k in sorted(missing):
            print(f"   - {k}")
            base_line = find_key_line(base_file, k)
            emit_github_annotation(
                "error",
                target_file,
                1,
                f"Missing i18n key: {k}",
                github_annotations,
            )
            emit_github_annotation(
                "notice",
                base_file,
                base_line,
                f"Reference key defined here: {k}",
                github_annotations,
            )
        template_path = write_missing_template(base, target_file, missing)
        print(f"\n[INFO] Missing key template generated: {template_path}")
        emit_github_annotation(
            "notice",
            template_path,
            1,
            f"Missing key template generated for {Path(target_file).name}",
            github_annotations,
        )
        has_error = True
    else:
        print("\n[OK] No missing keys")

    # Extra keys are warnings unless strict mode is enabled.
    if extra:
        print(f"\n[WARN] Extra keys in {target_file}:")
        annotation_level = "error" if strict_mode else "warning"
        for k in sorted(extra):
            print(f"   + {k}")
            key_line = find_key_line(target_file, k)
            emit_github_annotation(
                annotation_level,
                target_file,
                key_line,
                f"Extra i18n key: {k}",
                github_annotations,
            )
        if strict_mode:
            has_error = True
    else:
        print("\n[OK] No extra keys")

    # Type mismatch is warning unless strict mode is enabled.
    if type_mismatch:
        print(f"\n[WARN] Type mismatch keys:")
        annotation_level = "error" if strict_mode else "warning"
        for k in sorted(type_mismatch):
            print(f"   ~ {k} (base={type(base[k]).__name__}, target={type(target[k]).__name__})")
            key_line = find_key_line(target_file, k)
            emit_github_annotation(
                annotation_level,
                target_file,
                key_line,
                f"Type mismatch for key {k}: base={type(base[k]).__name__}, target={type(target[k]).__name__}",
                github_annotations,
            )
        if strict_mode:
            has_error = True
    else:
        print("\n[OK] No type mismatch")

    print("\n===== Summary =====")
    if has_error:
        print(f"[ERROR] Diff check FAILED: {target_file}")
    else:
        print(f"[OK] Diff check PASSED: {target_file}")

    return has_error


def main():
    base_file, target_files, strict_mode, github_annotations = parse_args(sys.argv)
    github_annotations = github_annotations or is_github_actions_env()

    base = load_json(base_file)

    if not target_files:
        target_files = discover_targets(base_file)

    if not target_files:
        print("[OK] No target locale files found, skip diff check")
        sys.exit(0)

    any_error = False
    for target_file in target_files:
        has_error = check_single_target(base, base_file, target_file, strict_mode, github_annotations)
        any_error = any_error or has_error

    if any_error:
        sys.exit(1)
    sys.exit(0)

if __name__ == "__main__":
    main()