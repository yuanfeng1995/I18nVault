[English](../README.md) | **中文**

# I18nVault

轻量级 C++17 国际化库，支持 **编译期 i18n**（零运行时开销）和 **运行时热更新**。

## 特性

- **编译期 i18n**：`constexpr` 翻译通过 `i18n<Key_X, Lang_Y>()` — 零运行时开销
- **强类型**：每个 key 是唯一的 struct 类型 — 编译期拼写错误检测
- **不定长格式化**：`i18n_fmt()` — 编译期 key 查找 + 运行时 `{0}` `{1}` 替换
- **运行时热更新**：`watchFile()` — 文件监听，修改后自动重载
- **双格式**：明文 JSON 与 SM4-GCM 加密 TRS
- **构建期代码生成**：Python 脚本从 JSON 自动生成 `i18n_keys.h`
- **Diff 校验**：构建期自动校验多语言 key 一致性
- **CMake install + CPack 打包**，支持 `find_package(I18nVault)`
- **跨平台**：Windows / macOS / Linux

## 快速开始

### 构建

```bash
cmake -S . -B build
cmake --build build --config Release
```

构建动态库（默认静态库）：

```bash
cmake -S . -B build -DBUILD_SHARED_LIBS=ON
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

### 编译期 i18n（推荐 — 零运行时开销）

```cpp
#include "i18n_manager.h"
using namespace I18nVault;

// 模板调用 — constexpr，零开销
constexpr auto text = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
// => "Login"

// 宏写法
constexpr auto menu = I18N_CT_SELECT(MENU_FILE, zh_CN);
// => "文件"

// 默认语言（en_US）
constexpr auto help = I18N_CT_DEFAULT(MENU_HELP);
// => "Help"
```

### 编译期 Key + 运行时格式化

```cpp
// 不定长参数 — key 编译期解析, 仅 {0} 替换在运行时执行
auto msg = I18N_CT_FMT(WELCOME_FMT, en_US, "Alice");
// => "Welcome, Alice!"

auto msg2 = I18N_CT_FMT(DIALOG_DELETE_FMT, zh_CN, filename);
// => "删除 photo.jpg？此操作无法撤销。"

// 函数调用风格
auto msg3 = i18n_fmt(i18n<Key_WELCOME_FMT, Lang_en_US>(), user, count);
```

### 运行时 i18n（向后兼容）

```cpp
auto& mgr = I18nVault::I18nManager::instance();
mgr.reload("i18n/en_US.json");

std::string text = I18nVault_TR(I18nVault::I18nKey::LOGIN_BUTTON);
// => "Login"

// 格式化
std::string msg = I18nVault_TR(I18nVault::I18nKey::WELCOME_FMT, "Alice");
// => "Welcome, Alice!"

// 强类型调用
auto text2 = mgr.translate<Key_LOGIN_BUTTON>();
```

### 运行时热更新

```cpp
auto& mgr = I18nVault::I18nManager::instance();
mgr.reload("i18n/zh_CN.json");

// 注册热更新回调（可选）
mgr.setReloadCallback([](bool ok, const std::string& path) {
    std::cout << (ok ? "OK" : "FAIL") << ": " << path << "\n";
});

// 启动文件监听（默认 1000ms，或自定义间隔）
mgr.watchFile("i18n/zh_CN.json", 500);

// ... 外部修改 zh_CN.json → 自动重载 ...

mgr.stopWatching();  // 或析构时自动停止
```

### 加载加密 TRS 文件

```cpp
auto& mgr = I18nVault::I18nManager::instance();

// 设置解密参数
mgr.setTrsCryptoConfig({
    .key_hex = "00112233445566778899AABBCCDDEEFF",  // ℹ️ 仅为示例，生产环境必须替换为安全密钥
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
CMakeLists.txt                          # 顶层：项目设置 + install + CPack
cmake/
  I18nVaultConfig.cmake.in              # find_package() 模板
src/
  CMakeLists.txt                        # I18nVaultCore 库（静态/动态）
  i18n_manager.h                        # 公共头文件：编译期 + 运行时 API
  i18n_manager.cpp                      # 实现（热更新、TRS 解密）
  crypto/                               # SM4-GCM 加密源文件
generated/  (构建输出)
  i18n_keys.h                           # 自动生成：Key_* 结构体、Lang_* 标签、i18n<> 模板
i18n/
  en_US.json                            # 英文语言文件（基准）
  zh_CN.json                            # 中文语言文件
  *.trs                                 # 构建期生成的加密文件
test/
  CMakeLists.txt                        # 测试目标
  main.cpp                              # 33 项测试：CT、FMT、运行时、TRS、热更新
examples/
  CMakeLists.txt                        # 示例目标（可选：-DI18NVAULT_BUILD_EXAMPLES=OFF）
  i18n_demo.cpp                         # 简洁功能演示
  compile_time_i18n_example.cpp         # 详细编译期示例
tools/
  gen_i18n_keys_constexpr.py            # 生成编译期 i18n_keys.h
  i18n_diff_check.py                    # 多语言 key 一致性校验
  i18n_diff_check_enhanced.py           # 增强校验（8+ 规则）
  gen_trs_files.py                      # 批量生成 TRS
  trs_safety_manager.py                 # TRS 完整性 + 版本管理
  encrypt_i18n.c                        # 加解密 CLI
docs/                                   # 文档
```

## 安装产物布局

```
<prefix>/
  include/I18nVault/
    i18n_manager.h              # 公共头文件
    i18n_vault_export.h         # DLL 导出宏（自动生成）
  share/I18nVault/tools/
    gen_i18n_keys.py            # key 枚举生成脚本
  lib/
    I18nVaultCore.lib           # 核心库（含加密，静态时为完整库，动态时为导入库）
    cmake/I18nVault/
      I18nVaultConfig.cmake
      I18nVaultConfigVersion.cmake
      I18nVaultTargets.cmake
  bin/
    i18n_crypto_cli             # 加解密 CLI 工具
    I18nVaultCore.dll           # 动态库（仅 BUILD_SHARED_LIBS=ON 时）
```

## CMake 目标

| 目标 | 类型 | 说明 |
|------|------|------|
| `I18nVaultCore` | 静态库 / 动态库 | i18n 核心库（含 SM4-GCM 加密，由 `BUILD_SHARED_LIBS` 控制） |
| `i18n_crypto_cli` | 可执行 | JSON/TRS 加解密 CLI |
| `i18n_test` | 可执行 | 测试套件（33 项测试） |
| `i18n_demo` | 可执行 | 功能演示 |
| `i18n_compile_time_example` | 可执行 | 编译期 i18n 示例 |
| `i18n_keys` | 自定义 | 生成 `i18n_keys.h`（JSON 变化时自动重新运行） |
| `i18n_diff_check` | 自定义 | key 一致性校验 |
| `i18n_trs` | 自定义 | 构建期 TRS 生成 |

## 公共 API

### 编译期（零开销）

| API | 说明 |
|-----|------|
| `i18n<Key_X, Lang_Y>()` | 编译期翻译（constexpr） |
| `i18n_fmt(fmt, args...)` | 编译期 key + 运行时不定长格式化 |
| `I18N_CT_SELECT(key, lang)` | 宏：`i18n<>()` 简写 |
| `I18N_CT_DEFAULT(key)` | 宏：默认 en_US |
| `I18N_CT_FMT(key, lang, ...)` | 宏：编译期 key + 运行时格式化 |
| `I18N_CT_FMT_DEFAULT(key, ...)` | 宏：格式化（默认 en_US） |

### 运行时

| API | 说明 |
|-----|------|
| `I18nManager::instance()` | 获取单例 |
| `reload(path)` | 加载 .json 或 .trs 文件 |
| `translate(key)` | 普通翻译 |
| `translate(key, {args...})` | 翻译并替换 `{0}` `{1}` 占位符 |
| `translate<KeyType>({args...})` | 强类型翻译 |
| `watchFile(path, interval_ms)` | 启动热更新文件监听 |
| `stopWatching()` | 停止文件监听 |
| `isWatching()` | 查询监听状态 |
| `setReloadCallback(cb)` | 注册热更新回调 |
| `setTrsCryptoConfig({key_hex, aad})` | 设置 TRS 解密参数 |
| `clearTrsCryptoConfig()` | 清除解密参数 |
| `I18nVault_TR(key, ...)` | 运行时翻译便捷宏 |

## 配置项

### 构建期（CMake Cache）

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `BUILD_SHARED_LIBS` | `OFF` | 设为 `ON` 时将 I18nVaultCore 构建为动态库 |
| `I18NVAULT_BUILD_EXAMPLES` | `ON` | 设为 `OFF` 跳过构建示例 |
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
i18n/*.json ──→ gen_i18n_keys_constexpr.py ──→ generated/i18n_keys.h
                  (Key_* 结构体、Lang_* 标签、constexpr i18n<> 模板)
i18n/*.json ──→ i18n_diff_check.py ──────────→ 校验通过 / 报错
i18n/*.json ──→ gen_trs_files.py ────────────→ i18n/*.trs
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
