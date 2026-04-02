# I18nVault

I18nVault 是一个轻量级 i18n 核心库，围绕以下目标构建：

- 运行期快速查词（字符串 key -> 文案）
- 支持明文 JSON 与加密 TRS 两种加载格式
- 构建期自动校验多语言 key 一致性
- 构建期自动生成 TRS 产物，减少人工步骤
- 保持轻量依赖，不引入重型测试框架

## 核心能力

- I18n 核心库：I18nVaultCore（静态库）
- 运行时加载：支持 JSON 与 TRS
- 加密算法：SM4-GCM（带认证标签）
- 构建期检查：key diff 严格校验
- 构建期产物：自动生成 generated/i18n_keys.h 与 i18n/*.trs
- 跨平台 CI：Windows / macOS / Linux

## 架构设计

### 1. 模块分层

- Core 层：src/i18n_manager.h, src/i18n_manager.cpp
	- 对外提供 reload / translate
	- 管理线程安全缓存和版本号
	- 负责 TRS 解密后的 JSON 解析与 key 校验

- Crypto 层：src/crypto/sm4.c, src/crypto/gcm.c, src/crypto/sm4_gcm.c
	- 提供 SM4-GCM one-shot 加解密接口
	- 作为基础静态库 crypto

- Build Tools 层：tools/gen_i18n_keys.py, tools/i18n_diff_check.py, tools/gen_trs_files.py, tools/encrypt_i18n.c
	- 生成枚举头文件
	- 校验多语言 key 差异
	- 调用加密 CLI 生成 TRS

- Test 层：test/main.cpp
	- 轻量可执行回归测试
	- 覆盖 JSON 加载、缺 key、TRS 解密与错误 key 失败路径

### 2. 构建期数据流

1. i18n/en_US.json -> tools/gen_i18n_keys.py -> generated/i18n_keys.h
2. tools/i18n_diff_check.py 严格校验 i18n/*.json key 集合
3. tools/gen_trs_files.py 调用 i18n_crypto_cli，将 i18n/*.json 生成同级 i18n/*.trs

### 3. 运行期数据流

- 当 reload(path) 传入 .json：直接解析并扁平化 key
- 当 reload(path) 传入 .trs：
	1) 解析 TRS 头
	2) 使用 SM4-GCM 解密得到 JSON 文本
	3) 解析 JSON 并扁平化 key
	4) 与枚举全集做必需 key 校验

## CMake 目标说明

- I18nVaultCore：i18n 核心静态库
- crypto：底层加密静态库
- i18n_crypto_cli：JSON/TRS 加解密命令行工具
- i18n_test：轻量测试可执行文件
- i18n_keys：生成 i18n key 枚举头
- i18n_diff_check：构建期 key diff 校验
- i18n_trs：构建期 TRS 生成

## 配置项

### 构建期 TRS 生成参数（CMake Cache）

- I18N_TRS_KEY_HEX：32 位 hex 密钥
- I18N_TRS_AAD：AAD 字符串

### 运行期 TRS 解密参数

优先使用 I18nManager::setTrsCryptoConfig 注入：

- key_hex：32 位 hex 密钥
- aad：AAD 字符串

若未显式注入，兼容回退环境变量：

- I18N_TRS_KEY_HEX（或 I18N_SM4_KEY_HEX）
- I18N_TRS_AAD

## 构建与测试

### 本地构建

```bash
cmake -S . -B build
cmake --build build
```

### 运行测试

```bash
ctest --test-dir build --output-on-failure
```

## CI 设计

CI 工作流位于 .github/workflows/ci.yml，采用矩阵执行：

- ubuntu-latest
- macos-latest
- windows-latest

流水线包含：

1. 严格 key diff 校验
2. CMake 构建
3. CTest 冒烟测试
4. TRS 产物存在性检查
5. 基于构建产出的 TRS 做解密回环校验

## 相关文档

- TRS 加解密文档：docs/trs_crypto.md

## 设计取舍

- 测试策略：采用轻量可执行测试，避免重型测试框架
- 配置策略：运行时显式配置优先，环境变量回退保证兼容
- 产物策略：TRS 在构建期自动生成，避免人工漏操作