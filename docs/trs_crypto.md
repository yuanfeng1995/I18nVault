# TRS Encryption/Decryption Guide

This document describes the `i18n_crypto_cli` workflow for converting i18n JSON files to encrypted TRS files using SM4-GCM.

## 1. Build CLI

From project root:

```bash
cmake -S . -B build
cmake --build build --target i18n_crypto_cli
```

Executable path:
- Linux/macOS: `build/i18n_crypto_cli`
- Windows: `build/Debug/i18n_crypto_cli.exe` (or your current build config directory)

## 2. Command Syntax

```text
i18n_crypto_cli enc -i <input.json> -o <output.trs> -k <32-hex-key> [--aad <text>] [--iv-hex <24-hex-iv>]
i18n_crypto_cli dec -i <input.trs>  -o <output.json> -k <32-hex-key> [--aad <text>]
```

Arguments:
- `enc` / `dec`: encrypt or decrypt mode.
- `-i`: input file path.
- `-o`: output file path.
- `-k`: 16-byte SM4 key in hex (32 hex chars).
- `--aad`: optional AAD string. Must be identical in encryption and decryption.
- `--iv-hex`: optional IV in hex (24 hex chars, 12 bytes). If omitted in `enc`, random IV is generated.

## 3. Examples

Encrypt JSON to TRS:

```bash
i18n_crypto_cli enc -i i18n/zh_CN.json -o i18n/zh_CN.trs -k 00112233445566778899AABBCCDDEEFF --aad i18n:v1
```

Decrypt TRS back to JSON:

```bash
i18n_crypto_cli dec -i i18n/zh_CN.trs -o i18n/zh_CN.dec.json -k 00112233445566778899AABBCCDDEEFF --aad i18n:v1
```

## 4. TRS Binary Format

Current format (`TRS1`) layout:

1. Magic: 4 bytes (`"TRS1"`)
2. Version: 1 byte (`0x01`)
3. IV length: 1 byte (currently `12`)
4. Tag length: 1 byte (currently `16`)
5. Reserved: 1 byte (`0x00`)
6. Cipher length: 4 bytes little-endian
7. IV bytes
8. GCM tag bytes
9. Cipher bytes

## 5. Security Notes

- Use a strong random key and protect key storage (do not hardcode key in source or CI logs).
- If `--aad` is used, it becomes part of authentication; mismatch will cause decryption failure.
- Never reuse the same `(key, iv)` pair for different plaintexts.
- In `enc` mode, if `--iv-hex` is not provided, the tool generates a random IV.

## 6. Troubleshooting

- `[ERROR] invalid key hex`: key must be exactly 32 hex chars.
- `[ERROR] invalid iv hex`: iv must be exactly 24 hex chars.
- `[ERROR] decrypt/auth verify failed`: key/AAD/IV/tag mismatch or file tampered.
- `[ERROR] invalid TRS magic/header`: input is not recognized TRS format.
