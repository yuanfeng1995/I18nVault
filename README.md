**English** | [中文](docs/README.zh_CN.md)

# I18nVault

A lightweight C++17 internationalization library with **compile-time i18n** (zero runtime cost) and **runtime hot-reload**.

## Features

- **Compile-time i18n**: `constexpr` translated strings via `i18n<Key_X, Lang_Y>()` — zero runtime cost
- **Strong typing**: each key is a unique struct type — compile-time typo detection
- **Variadic format**: `i18n_fmt()` — compile-time key lookup + runtime `{0}` `{1}` substitution
- **Runtime hot-reload**: `watchFile()` — poll-based file watcher, auto-reload on change
- **Dual format**: plain JSON and SM4-GCM encrypted TRS
- **Build-time code generation**: Python script auto-generates `i18n_keys.h` from JSON
- **Diff validation**: build-time key consistency validation across locales
- **CMake install + CPack packaging**, supports `find_package(I18nVault)`
- **Cross-platform**: Windows / macOS / Linux

## Quick Start

### Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

Build as shared library (default is static):

```bash
cmake -S . -B build -DBUILD_SHARED_LIBS=ON
cmake --build build --config Release
```

### Run Tests

```bash
ctest --test-dir build -C Release --output-on-failure
```

### Install

```bash
cmake --install build --config Release --prefix /usr/local
```

### Package

```bash
cd build
cpack -C Release
# Output: I18nVault-1.0.0-<OS>-<Arch>.zip / .tar.gz / .deb
```

## Integration

### Option 1: CMake find_package

After installing, add to your `CMakeLists.txt`:

```cmake
find_package(I18nVault REQUIRED)
target_link_libraries(your_target PRIVATE I18nVault::I18nVaultCore)
```

### Option 2: add_subdirectory

Add I18nVault as a subdirectory:

```cmake
add_subdirectory(third_party/I18nVault)
target_link_libraries(your_target PRIVATE I18nVaultCore)
```

## Usage

### Compile-time i18n (Recommended — Zero Runtime Cost)

```cpp
#include "i18n_manager.h"
using namespace I18nVault;

// Direct template call — constexpr, zero overhead
constexpr auto text = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
// => "Login"

// Macro shorthand
constexpr auto menu = I18N_CT_SELECT(MENU_FILE, zh_CN);
// => "文件"

// Default language (en_US)
constexpr auto help = I18N_CT_DEFAULT(MENU_HELP);
// => "Help"
```

### Compile-time Key + Runtime Format

```cpp
// Variadic format — key resolved at compile time, only {0} substitution at runtime
auto msg = I18N_CT_FMT(WELCOME_FMT, en_US, "Alice");
// => "Welcome, Alice!"

auto msg2 = I18N_CT_FMT(DIALOG_DELETE_FMT, zh_CN, filename);
// => "删除 photo.jpg？此操作无法撤销。"

// Function call style
auto msg3 = i18n_fmt(i18n<Key_WELCOME_FMT, Lang_en_US>(), user, count);
```

### Runtime i18n (Backward Compatible)

```cpp
auto& mgr = I18nVault::I18nManager::instance();
mgr.reload("i18n/en_US.json");

// Macro
std::string text = I18nVault_TR(I18nVault::I18nKey::LOGIN_BUTTON);
// => "Login"

// With placeholder substitution
std::string msg = I18nVault_TR(I18nVault::I18nKey::WELCOME_FMT, "Alice");
// => "Welcome, Alice!"

// Strong-typed template call
auto text2 = mgr.translate<Key_LOGIN_BUTTON>();
```

### Runtime Hot-Reload

```cpp
auto& mgr = I18nVault::I18nManager::instance();
mgr.reload("i18n/zh_CN.json");

// Optional callback on each reload
mgr.setReloadCallback([](bool ok, const std::string& path) {
    std::cout << (ok ? "OK" : "FAIL") << ": " << path << "\n";
});

// Start watching (default 1000ms poll, or custom)
mgr.watchFile("i18n/zh_CN.json", 500);

// ... edit zh_CN.json externally → auto-reloaded ...

mgr.stopWatching();  // or auto-stops on destruction
```

### Loading Encrypted TRS Files

```cpp
auto& mgr = I18nVault::I18nManager::instance();

// Configure decryption
mgr.setTrsCryptoConfig({
    .key_hex = "00112233445566778899AABBCCDDEEFF",  // ℹ️ Demo only — replace with a secure key in production
    .aad     = "i18n:v1"                             // Additional authenticated data
});

// Load TRS (auto-decrypts)
mgr.reload("i18n/zh_CN.trs");

// Translation works as usual
std::string text = I18nVault_TR(I18nVault::I18nKey::LOGIN_BUTTON);

// Clear key material when no longer needed
mgr.clearTrsCryptoConfig();
```

## Project Structure

```
CMakeLists.txt                          # Top-level: project settings + install + CPack
cmake/
  I18nVaultConfig.cmake.in              # find_package() template
src/
  CMakeLists.txt                        # I18nVaultCore library (static/shared)
  i18n_manager.h                        # Public header: compile-time + runtime API
  i18n_manager.cpp                      # Implementation (hot-reload, TRS decryption)
  crypto/                               # SM4-GCM crypto sources
generated/  (build output)
  i18n_keys.h                           # Auto-generated: Key_* structs, Lang_* tags, i18n<> template
i18n/
  en_US.json                            # English locale (baseline)
  zh_CN.json                            # Chinese locale
  *.trs                                 # Encrypted files (build-time generated)
test/
  CMakeLists.txt                        # Test target
  main.cpp                              # 33 tests: CT, FMT, runtime, TRS, hot-reload
examples/
  CMakeLists.txt                        # Example targets (optional: -DI18NVAULT_BUILD_EXAMPLES=OFF)
  i18n_demo.cpp                         # Concise feature demo
  compile_time_i18n_example.cpp         # Detailed compile-time examples
tools/
  CMakeLists.txt                        # CLI tools + build-time tasks
  gen_i18n_keys_constexpr.py            # Generates i18n_keys.h with compile-time i18n
  gen_i18n_keys.py                      # Legacy enum-only generator
  i18n_diff_check.py                    # Multi-locale key consistency checker
  i18n_diff_check_enhanced.py           # Enhanced checker (8+ quality rules)
  gen_trs_files.py                      # Batch TRS generation
  trs_safety_manager.py                 # TRS integrity + version management
  encrypt_i18n.c                        # Encryption/decryption CLI
docs/                                   # Documentation
```

## Install Layout

```
<prefix>/
  include/I18nVault/
    i18n_manager.h              # Public header
    i18n_vault_export.h         # DLL export macros (auto-generated)
  share/I18nVault/tools/
    gen_i18n_keys.py            # Key enum generation script
  lib/
    I18nVaultCore.lib           # Core library (with crypto; static = full lib, shared = import lib)
    cmake/I18nVault/
      I18nVaultConfig.cmake
      I18nVaultConfigVersion.cmake
      I18nVaultTargets.cmake
  bin/
    i18n_crypto_cli             # Encryption/decryption CLI tool
    I18nVaultCore.dll           # Shared library (BUILD_SHARED_LIBS=ON only)
```

## CMake Targets

| Target | Type | Description |
|--------|------|-------------|
| `I18nVaultCore` | Static / Shared lib | Core i18n library (with SM4-GCM crypto, controlled by `BUILD_SHARED_LIBS`) |
| `i18n_crypto_cli` | Executable | JSON/TRS encryption/decryption CLI |
| `i18n_test` | Executable | Test suite (33 tests) |
| `i18n_demo` | Executable | Feature demo |
| `i18n_compile_time_example` | Executable | Compile-time i18n examples |
| `i18n_keys` | Custom | Generates `i18n_keys.h` (auto-runs on JSON change) |
| `i18n_diff_check` | Custom | Key consistency validation |
| `i18n_trs` | Custom | Build-time TRS generation |

## Public API

### Compile-time (Zero Cost)

| API | Description |
|-----|-------------|
| `i18n<Key_X, Lang_Y>()` | Compile-time translation (constexpr) |
| `i18n_fmt(fmt, args...)` | Compile-time key + runtime variadic format |
| `I18N_CT_SELECT(key, lang)` | Macro shorthand for `i18n<>()` |
| `I18N_CT_DEFAULT(key)` | Macro shorthand (defaults to en_US) |
| `I18N_CT_FMT(key, lang, ...)` | Macro: compile-time key + runtime format |
| `I18N_CT_FMT_DEFAULT(key, ...)` | Macro: format with en_US default |

### Runtime

| API | Description |
|-----|-------------|
| `I18nManager::instance()` | Get the singleton |
| `reload(path)` | Load a .json or .trs file |
| `translate(key)` | Basic translation |
| `translate(key, {args...})` | Translate with `{0}` `{1}` substitution |
| `translate<KeyType>({args...})` | Strong-typed translate |
| `watchFile(path, interval_ms)` | Start hot-reload file watcher |
| `stopWatching()` | Stop the file watcher |
| `isWatching()` | Query watcher status |
| `setReloadCallback(cb)` | Register hot-reload callback |
| `setTrsCryptoConfig({key_hex, aad})` | Set TRS decryption parameters |
| `clearTrsCryptoConfig()` | Clear decryption parameters |
| `I18nVault_TR(key, ...)` | Convenience macro for runtime translate |

## Configuration

### Build-time (CMake Cache)

| Variable | Default | Description |
|----------|---------|-------------|
| `BUILD_SHARED_LIBS` | `OFF` | Set to `ON` to build I18nVaultCore as a shared library |
| `I18NVAULT_BUILD_EXAMPLES` | `ON` | Set to `OFF` to skip building examples |
| `I18N_TRS_KEY_HEX` | `00112233...FF` | 32-char hex SM4 key |
| `I18N_TRS_AAD` | `i18n:v1` | Additional authenticated data |

### Runtime

Prefer injecting via `setTrsCryptoConfig()`. Falls back to environment variables if not set:

| Environment Variable | Description |
|---------------------|-------------|
| `I18N_TRS_KEY_HEX` / `I18N_SM4_KEY_HEX` | SM4 key |
| `I18N_TRS_AAD` | AAD |

## Adding a New Locale

1. Copy `i18n/en_US.json` to `i18n/<locale>.json`
2. Translate all values (keep key structure unchanged)
3. `i18n_diff_check` automatically validates key completeness at build time
4. Corresponding `.trs` encrypted file is auto-generated during build

## Adding a New Key

1. Add a new key-value pair to `i18n/en_US.json`
2. Update all other locale files accordingly
3. `generated/i18n_keys.h` enum is regenerated automatically at build time
4. Use `I18nVault::I18nKey::NEW_KEY` in your code

## Build-time Data Flow

```
i18n/*.json ──→ gen_i18n_keys_constexpr.py ──→ generated/i18n_keys.h
                  (Key_* structs, Lang_* tags, constexpr i18n<> template)
i18n/*.json ──→ i18n_diff_check.py ──────────→ Pass / Fail
i18n/*.json ──→ gen_trs_files.py ────────────→ i18n/*.trs
```

## CI

GitHub Actions cross-platform matrix (Ubuntu / macOS / Windows), automatically runs:

1. Strict key diff validation
2. CMake build
3. CTest tests
4. TRS artifact verification
5. SM4-GCM encrypt/decrypt roundtrip verification
6. cmake install artifact verification
7. CPack packaging
8. Upload packages as Actions artifacts

## License

MIT

## CI Design

CI workflow at `.github/workflows/ci.yml`, matrix execution across:

- ubuntu-latest
- macos-latest
- windows-latest

Pipeline includes:

1. Strict key diff validation
2. CMake build
3. CTest smoke tests
4. TRS artifact existence check
5. Decrypt roundtrip verification using build-produced TRS

## Related Documentation

- TRS encryption/decryption: docs/trs_crypto.md

## Design Trade-offs

- Testing: lightweight executable tests, no heavy test frameworks
- Configuration: explicit runtime configuration preferred, environment variable fallback for compatibility
- Artifacts: TRS files auto-generated at build time to avoid manual errors