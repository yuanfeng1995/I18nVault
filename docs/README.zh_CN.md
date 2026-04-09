[English](../README.md) | **中文**

# I18nVault

轻量级 C++ 国际化库，支持明文 JSON 与 SM4-GCM 加密 TRS 双格式加载。

## 特性

- 运行时快速查词（枚举 key → 本地化字符串）
- 格式化翻译，支持 `{0}` `{1}` 占位符替换
- 支持明文 JSON 与加密 TRS 两种加载格式
- SM4-GCM 认证加密，防篡改
- **目录自动扫描**：`setI18nDirectory()` 一键加载整个目录下的翻译文件
- **热更新**：`reloadLanguage()` 运行时重新读取单个语言文件
- **TRS/JSON 共存**：同一 locale 两种格式并存时，已配置密钥则优先加载 `.trs`
- **UI 友好语言列表**：`availableLanguageInfos()` 提供显示名、文件信息及状态
- **动态语言切换**，支持可选回退语言
- **语言切换回调**，便于 UI 在语言变更时刷新
- 构建期自动校验多语言 key 一致性（包括 `_LANGUAGE_NAME`）
- 构建期自动生成 TRS 产物与枚举头文件
- pImpl 隔离私有实现，公共头文件干净
- 线程安全：shared_mutex 保护所有状态
- 跨平台：Windows / macOS / Linux

## 快速开始

### 构建

```bash
cmake -S . -B build
cmake --build build --config Release
```

### 运行测试

```bash
ctest --test-dir build -C Release --output-on-failure
```

## 集成到你的项目

将 I18nVault 作为子目录添加到你的工程：

```cmake
add_subdirectory(third_party/I18nVault)
target_link_libraries(your_target PRIVATE I18nVaultCore)
```

## 使用示例

### 基本翻译

```cpp
#include "i18n_manager.h"

// 获取单例
auto& mgr = I18nVault::I18nManager::instance();

// 加载语言
mgr.addLanguage("en_US", "i18n/en_US.json");

// 使用宏翻译
std::string text = I18nVault_TR(I18nVault::I18nKey::LOGIN_BUTTON);
// => "Login"

// 或直接调用方法
std::string text2 = mgr.translate(I18nVault::I18nKey::MENU_FILE);
// => "File"
```

### 格式化翻译

JSON 中使用 `{0}` `{1}` 等占位符：

```json
{
  "_LANGUAGE_NAME": "English",
  "WELCOME_FMT": "Welcome, {0}!",
  "dialog": {
    "delete_fmt": "Delete {0}? This action cannot be undone."
  }
}
```

```cpp
// 使用宏（推荐）
std::string msg = I18nVault_TR(I18nVault::I18nKey::WELCOME_FMT, "Alice");
// => "Welcome, Alice!"

std::string msg2 = I18nVault_TR(I18nVault::I18nKey::DIALOG_DELETE_FMT, "photo.jpg");
// => "Delete photo.jpg? This action cannot be undone."

// 或直接调用方法
std::string msg3 = mgr.translate(I18nVault::I18nKey::WELCOME_FMT, {"Alice"});
```

### 加载加密 TRS 文件

```cpp
auto& mgr = I18nVault::I18nManager::instance();

// 设置解密参数
mgr.setTrsCryptoConfig({
    .key_hex = "00112233445566778899AABBCCDDEEFF",  // ℹ️ 仅为示例，生产环境必须替换为安全密钥
    .aad     = "i18n:v1"                             // 附加认证数据
});

// 通过 addLanguage 加载 TRS（自动解密）
mgr.addLanguage("zh_CN", "i18n/zh_CN.trs");

// 翻译照常使用
std::string text = I18nVault_TR(I18nVault::I18nKey::LOGIN_BUTTON);

// 不再需要时清除密钥
mgr.clearTrsCryptoConfig();
```

### 目录自动扫描

无需逐个加载文件，指定目录即可批量加载：

```cpp
auto& mgr = I18nVault::I18nManager::instance();

// 可选：先配置 TRS 解密
mgr.setTrsCryptoConfig({"00112233445566778899AABBCCDDEEFF", "i18n:v1"});

// 自动扫描并加载目录下所有 .json 和 .trs 文件
// 同一 locale 两种格式同时存在时，已配置密钥则优先使用 .trs
int loaded = mgr.setI18nDirectory("i18n");
// loaded == 2 (en_US + zh_CN)
```

### 热更新

运行时重新读取单个语言文件，无需重启：

```cpp
// 在磁盘上修改 i18n/en_US.json 后...
mgr.reloadLanguage("en_US");  // 重新解析并更新内存数据
```

### UI 友好语言列表

```cpp
auto infos = mgr.availableLanguageInfos();
for (const auto& info : infos) {
    // info.locale      => "en_US"
    // info.displayName => "English"       （来自 JSON 中的 _LANGUAGE_NAME）
    // info.fileType    => "json" 或 "trs"
    // info.isActive    => true/false
    // info.isFallback  => true/false
}
// 直接用 displayName 展示在 UI 语言选择器中
```

### 动态语言切换

启动时加载多个语言，运行时随时切换。
第一次调用 `addLanguage` 会自动将其设为活跃语言。

```cpp
auto& mgr = I18nVault::I18nManager::instance();

// 加载语言（第一个自动成为活跃语言）
mgr.addLanguage("en_US", "i18n/en_US.json");
mgr.addLanguage("zh_CN", "i18n/zh_CN.json");

// 可选：设置回退语言（当前语言缺少某 key 时使用）
mgr.setFallbackLanguage("en_US");

// 查询已加载的语言列表
std::vector<std::string> langs = mgr.availableLanguages();
// => ["en_US", "zh_CN"]

// 注册语言切换回调
size_t cbId = mgr.onLanguageChanged([](const std::string& locale) {
    std::cout << "语言切换为：" << locale << std::endl;
    // 例如：触发 UI 刷新
});

// 运行时切换语言
mgr.setLanguage("zh_CN");            // 触发回调
std::string text = I18nVault_TR(I18nVault::I18nKey::LOGIN_BUTTON);
// => "登录"

mgr.setLanguage("en_US");            // 触发回调
text = I18nVault_TR(I18nVault::I18nKey::LOGIN_BUTTON);
// => "Login"

// 不再需要时注销回调
mgr.removeLanguageChangedCallback(cbId);

// 移除语言（不能移除当前活跃语言）
mgr.removeLanguage("zh_CN");
```

## 项目结构

```
CMakeLists.txt                  # 顶层：项目设置
src/
  CMakeLists.txt                # I18nVaultCore 静态库
  i18n_manager.h                # 公共头文件（pImpl）
  i18n_manager.cpp              # 实现
  crypto/
    CMakeLists.txt              # SM4-GCM 加密源文件（编译进 Core）
    sm4.c / sm4.h               # SM4 分组加密
    gcm.c / gcm.h               # GCM 模式
    sm4_gcm.c / sm4_gcm.h       # SM4-GCM 一体化接口
    hex_utils.c / hex_utils.h   # Hex 解析工具
generated/
  i18n_keys.h                   # 自动生成的枚举与映射函数
i18n/
  en_US.json                    # 英文语言文件（基准）
  zh_CN.json                    # 中文语言文件
  *.trs                         # 构建期生成的加密文件
test/
  CMakeLists.txt                # 测试
  main.cpp                      # 回归测试
tools/
  CMakeLists.txt                # CLI 工具 + 构建期任务
  gen_i18n_keys.py              # 生成 i18n_keys.h
  i18n_diff_check.py            # 多语言 key 一致性校验
  gen_trs_files.py              # 批量生成 TRS
  encrypt_i18n.c                # 加解密 CLI
```

## CMake 目标

| 目标 | 类型 | 说明 |
|------|------|------|
| `I18nVaultCore` | 静态库 | i18n 核心库（含 SM4-GCM 加密） |
| `i18n_crypto_cli` | 可执行 | JSON/TRS 加解密 CLI |
| `i18n_test` | 可执行 | 回归测试 |
| `i18n_keys` | 自定义 | 生成枚举头文件 |
| `i18n_diff_check` | 自定义 | key 一致性校验 |
| `i18n_trs` | 自定义 | 构建期 TRS 生成 |

## 公共 API

### 翻译

| 方法 / 宏 | 说明 |
|-----------|------|
| `I18nManager::instance()` | 获取单例 |
| `translate(key)` | 普通翻译（args 默认为空） |
| `translate(key, {args...})` | 翻译并替换 `{0}` `{1}` 占位符 |
| `I18nVault_TR(key, ...)` | 统一便捷宏：无参为普通翻译，有参为格式化翻译 |

### 语言管理

| 方法 | 说明 |
|------|------|
| `setI18nDirectory(dir)` | 自动扫描目录并加载所有 .json/.trs 文件。返回加载数量，出错返回 -1。 |
| `reloadLanguage(locale)` | 重新读取并加载单个 locale 的翻译文件。 |
| `addLanguage(locale, path)` | 加载语言文件并注册。首次调用自动激活。 |
| `setLanguage(locale)` | 切换活跃语言（必须已加载）。触发回调。 |
| `currentLanguage()` | 返回当前活跃 locale（未设置时为空）。 |
| `setFallbackLanguage(locale)` | 设置缺失 key 时使用的回退语言。 |
| `fallbackLanguage()` | 返回当前回退 locale。 |
| `availableLanguages()` | 列出所有已加载的 locale 名称。 |
| `availableLanguageInfos()` | 获取详细信息（displayName, filePath, fileType, isActive, isFallback），用于 UI 展示。 |
| `removeLanguage(locale)` | 卸载语言（当前活跃语言无法卸载）。 |
| `onLanguageChanged(callback)` | 注册回调；返回 ID（0 表示失败）。 |
| `removeLanguageChangedCallback(id)` | 按 ID 注销回调。 |

### TRS 加密

| 方法 | 说明 |
|------|------|
| `setTrsCryptoConfig({key_hex, aad})` | 设置 TRS 解密参数。 |
| `clearTrsCryptoConfig()` | 安全清除解密参数。 |

## 配置项

### 构建期（CMake Cache）

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `I18N_TRS_KEY_HEX` | `00112233...FF` | 32 hex 字符 SM4 密钥 |
| `I18N_TRS_AAD` | `i18n:v1` | 附加认证数据 |

### 运行期

优先通过 `setTrsCryptoConfig()` 注入。若未注入，回退到环境变量：

| 环境变量 | 说明 |
|---------|------|
| `I18N_TRS_KEY_HEX` / `I18N_SM4_KEY_HEX` | SM4 密钥 |
| `I18N_TRS_AAD` | AAD |

## JSON 文件格式

每个翻译文件**必须**包含顶层 `_LANGUAGE_NAME` 键，值为该语言的原生显示名称：

```json
{
  "_LANGUAGE_NAME": "简体中文",
  "LOGIN_BUTTON": "登录",
  "WELCOME_FMT": "欢迎, {0}!",
  "menu": {
    "file": "文件"
  }
}
```

- `_LANGUAGE_NAME` 是**必填项** — 缺少该键的文件在构建期会校验失败，运行时 `addLanguage()` 也会拒绝加载。
- `_LANGUAGE_NAME` 仅作为元数据 — **不会**被纳入生成的枚举（`i18n_keys.h`）。
- 该值用作 `availableLanguageInfos()` 中的 `displayName`，可直接用于 UI 展示。

## 添加新语言

1. 复制 `i18n/en_US.json` 为 `i18n/<locale>.json`
2. 设置 `_LANGUAGE_NAME` 为该语言的原生名称（如 `"繁體中文"`、`"Français"`）
3. 翻译所有值（保持 key 结构不变）
4. 构建时 `i18n_diff_check` 自动校验 key 完整性和 `_LANGUAGE_NAME`
5. 构建时自动生成对应 `.trs` 加密文件

## 添加新 Key

1. 在 `i18n/en_US.json` 中添加新的 key-value
2. 同步更新所有其他语言文件
3. 构建时自动重新生成 `generated/i18n_keys.h` 中的枚举
4. 代码中通过 `I18nVault::I18nKey::NEW_KEY` 使用

## 构建期数据流

```
i18n/en_US.json ──→ gen_i18n_keys.py ──→ generated/i18n_keys.h
i18n/*.json ────→ i18n_diff_check.py ──→ 校验通过 / 报错
i18n/*.json ────→ gen_trs_files.py ────→ i18n/*.trs
```

## CI

GitHub Actions 跨平台矩阵（Ubuntu / macOS / Windows），自动执行：

1. Key diff 严格校验
2. CMake 构建
3. CTest 测试
4. TRS 产物验证
5. SM4-GCM 加解密 roundtrip 验证
6. 源码包打包（tar.gz + zip）
7. `v*` 标签推送时发布 GitHub Release

## License

MIT

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
6. 源码包打包（`git archive` → tar.gz + zip）
7. `v*` 标签推送时，发布源码包到 GitHub Release

## 相关文档

- TRS 加解密文档：docs/trs_crypto.md

## 设计取舍

- 测试策略：采用轻量可执行测试，避免重型测试框架
- 配置策略：运行时显式配置优先，环境变量回退保证兼容
- 产物策略：TRS 在构建期自动生成，避免人工漏操作
