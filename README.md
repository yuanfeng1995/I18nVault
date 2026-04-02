# i18n_core_trs

工业级 i18n 核心模块，支持 SM4 加密、GCM 签名、enum 热更新和 key diff 检查。

## i18n 加密控制台

项目提供 `i18n_crypto_cli`，用于将 JSON 文案加密为 TRS，或将 TRS 解密回 JSON。

- 构建：`cmake --build . --target i18n_crypto_cli`
- 详细用法见 `docs/trs_crypto.md`