# I18nVault

轻量级 C++ 国际化库，支持明文 JSON 与 SM4-GCM 加密 TRS 双格式加载。

## 特性

- 运行时快速查词（枚举 key → 本地化字符串）
- 格式化翻译，支持 `{0}` `{1}` 占位符替换
- 支持明文 JSON 与加密 TRS 两种加载格式
- SM4-GCM 认证加密，防篡改
- 构建期自动校验多语言 key 一致性
- 构建期自动生成 TRS 产物与枚举头文件
- pImpl 隔离私有实现，公共头文件干净
- CMake install + CPack 打包，支持 `find_package(I18nVault)`
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

### 安装

```bash
cmake --install build --config Release --prefix /usr/local
```

### 打包

```bash
cd build
cpack -C Release
# 输出: I18nVault-1.0.0-<OS>-<Arch>.zip / .tar.gz / .deb
```

## 集成到你的项目

### 方式一：CMake find_package

安装后，在你的 `CMakeLists.txt` 中：

```cmake
find_package(I18nVault REQUIRED)
target_link_libraries(your_target PRIVATE I18nVault::I18nVaultCore)
```

### 方式二：add_subdirectory

将 I18nVault 作为子目录添加：

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

// 加载 JSON 语言文件
mgr.reload("i18n/en_US.json");

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
    .key_hex = "00112233445566778899AABBCCDDEEFF",  // 32 hex = 128-bit SM4 key
    .aad     = "i18n:v1"                             // 附加认证数据
});

// 加载 TRS（自动解密）
mgr.reload("i18n/zh_CN.trs");

// 翻译照常使用
std::string text = I18nVault_TR(I18nVault::I18nKey::LOGIN_BUTTON);

// 不再需要时清除密钥
mgr.clearTrsCryptoConfig();
```

## 项目结构

```
CMakeLists.txt                  # 顶层：项目设置 + install + CPack
cmake/
  I18nVaultConfig.cmake.in      # find_package() 模板
src/
  CMakeLists.txt                # I18nVaultCore 静态库
  i18n_manager.h                # 公共头文件（pImpl）
  i18n_manager.cpp              # 实现
  crypto/
    CMakeLists.txt              # I18nVaultCrypto 静态库（内部）
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

## 安装产物布局

```
<prefix>/
  include/I18nVault/
    i18n_manager.h              # 公共头文件
  share/I18nVault/tools/
    gen_i18n_keys.py            # key 枚举生成脚本
  lib/
    I18nVaultCore.lib           # 核心库
    I18nVaultCrypto.lib         # 加密库（内部依赖，无公共头文件）
    cmake/I18nVault/
      I18nVaultConfig.cmake
      I18nVaultConfigVersion.cmake
      I18nVaultTargets.cmake
  bin/
    i18n_crypto_cli             # 加解密 CLI 工具
```

## CMake 目标

| 目标 | 类型 | 说明 |
|------|------|------|
| `I18nVaultCore` | 静态库 | i18n 核心库 |
| `I18nVaultCrypto` | 静态库 | SM4-GCM 加密（内部） |
| `i18n_crypto_cli` | 可执行 | JSON/TRS 加解密 CLI |
| `i18n_test` | 可执行 | 回归测试 |
| `i18n_keys` | 自定义 | 生成枚举头文件 |
| `i18n_diff_check` | 自定义 | key 一致性校验 |
| `i18n_trs` | 自定义 | 构建期 TRS 生成 |

## 公共 API

| 方法 / 宏 | 说明 |
|-----------|------|
| `I18nManager::instance()` | 获取单例 |
| `reload(path)` | 加载 .json 或 .trs 文件 |
| `translate(key)` | 普通翻译（args 默认为空） |
| `translate(key, {args...})` | 翻译并替换 `{0}` `{1}` 占位符 |
| `setTrsCryptoConfig({key_hex, aad})` | 设置 TRS 解密参数 |
| `clearTrsCryptoConfig()` | 清除解密参数 |
| `I18nVault_TR(key, ...)` | 统一便捷宏：无额外参数为普通翻译，有参数为格式化翻译 |

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

## 添加新语言

1. 复制 `i18n/en_US.json` 为 `i18n/<locale>.json`
2. 翻译所有值（保持 key 结构不变）
3. 构建时 `i18n_diff_check` 自动校验 key 完整性
4. 构建时自动生成对应 `.trs` 加密文件

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
6. cmake install 产物验证
7. CPack 打包
8. 上传打包产物为 Actions artifact

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

## 相关文档

- TRS 加解密文档：docs/trs_crypto.md

## 设计取舍

- 测试策略：采用轻量可执行测试，避免重型测试框架
- 配置策略：运行时显式配置优先，环境变量回退保证兼容
- 产物策略：TRS 在构建期自动生成，避免人工漏操作