**English** | [中文](docs/README.zh_CN.md)

# I18nVault

A lightweight C++ internationalization library supporting both plain JSON and SM4-GCM encrypted TRS file loading.

## Features

- Fast runtime lookup (enum key → localized string)
- Formatted translations with `{0}` `{1}` placeholder substitution
- Dual format: plain JSON and encrypted TRS
- SM4-GCM authenticated encryption, tamper-proof
- **Directory auto-scan**: `setI18nDirectory()` loads all translations from a folder
- **Hot-reload**: `reloadLanguage()` re-reads a single locale file at runtime
- **TRS/JSON coexistence**: when both formats exist, `.trs` is preferred if crypto is configured
- **UI-friendly language list**: `availableLanguageInfos()` provides display names, file info, and active state
- **Dynamic language switching** with optional fallback locale
- **Language change callbacks** for UI refresh on locale switch
- Build-time i18n key consistency validation across locales (including `_LANGUAGE_NAME`)
- Build-time auto-generation of TRS files and enum header
- pImpl idiom keeps public headers clean
- Thread-safe: shared_mutex protects all state
- Cross-platform: Windows / macOS / Linux

## Quick Start

### Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

### Run Tests

```bash
ctest --test-dir build -C Release --output-on-failure
```

## Integration

Add I18nVault as a subdirectory in your project:

```cmake
add_subdirectory(third_party/I18nVault)
target_link_libraries(your_target PRIVATE I18nVaultCore)
```

## Usage

### Basic Translation

```cpp
#include "i18n_manager.h"

// Get the singleton
auto& mgr = I18nVault::I18nManager::instance();

// Load a language
mgr.addLanguage("en_US", "i18n/en_US.json");

// Translate using the macro
std::string text = I18nVault_TR(I18nVault::I18nKey::LOGIN_BUTTON);
// => "Login"

// Or call the method directly
std::string text2 = mgr.translate(I18nVault::I18nKey::MENU_FILE);
// => "File"
```

### Formatted Translation

Use `{0}` `{1}` placeholders in your JSON:

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
// Using the macro (recommended)
std::string msg = I18nVault_TR(I18nVault::I18nKey::WELCOME_FMT, "Alice");
// => "Welcome, Alice!"

std::string msg2 = I18nVault_TR(I18nVault::I18nKey::DIALOG_DELETE_FMT, "photo.jpg");
// => "Delete photo.jpg? This action cannot be undone."

// Or call the method directly
std::string msg3 = mgr.translate(I18nVault::I18nKey::WELCOME_FMT, {"Alice"});
```

### Loading Encrypted TRS Files

```cpp
auto& mgr = I18nVault::I18nManager::instance();

// Configure decryption
mgr.setTrsCryptoConfig({
    .key_hex = "00112233445566778899AABBCCDDEEFF",  // ℹ️ Demo only — replace with a secure key in production
    .aad     = "i18n:v1"                             // Additional authenticated data
});

// Load TRS via addLanguage (auto-decrypts)
mgr.addLanguage("zh_CN", "i18n/zh_CN.trs");

// Translation works as usual
std::string text = I18nVault_TR(I18nVault::I18nKey::LOGIN_BUTTON);

// Clear key material when no longer needed
mgr.clearTrsCryptoConfig();
```

### Directory Auto-Scan

Instead of loading files one by one, point to a directory and load all translations at once:

```cpp
auto& mgr = I18nVault::I18nManager::instance();

// Optionally configure TRS decryption first
mgr.setTrsCryptoConfig({"00112233445566778899AABBCCDDEEFF", "i18n:v1"});

// Auto-scan: loads all .json and .trs files from the directory.
// When both en_US.json and en_US.trs exist, .trs is preferred if crypto is configured.
int loaded = mgr.setI18nDirectory("i18n");
// loaded == 2 (en_US + zh_CN)
```

### Hot-Reload

Re-read a single translation file at runtime without restarting:

```cpp
// After editing i18n/en_US.json on disk...
mgr.reloadLanguage("en_US");  // re-parses and updates in-memory data
```

### UI-Friendly Language List

```cpp
auto infos = mgr.availableLanguageInfos();
for (const auto& info : infos) {
    // info.locale      => "en_US"
    // info.displayName => "English"       (from _LANGUAGE_NAME in JSON)
    // info.fileType    => "json" or "trs"
    // info.isActive    => true/false
    // info.isFallback  => true/false
}
// Use displayName directly in your UI language selector.
```

### Dynamic Language Switching

Load multiple locales at startup and switch between them at runtime.
The first `addLanguage` call automatically becomes the active locale.

```cpp
auto& mgr = I18nVault::I18nManager::instance();

// Load languages (first one becomes active automatically)
mgr.addLanguage("en_US", "i18n/en_US.json");
mgr.addLanguage("zh_CN", "i18n/zh_CN.json");

// Optionally set a fallback for missing keys
mgr.setFallbackLanguage("en_US");

// Query available locales
std::vector<std::string> langs = mgr.availableLanguages();
// => ["en_US", "zh_CN"]

// Register a callback fired when the active language changes
size_t cbId = mgr.onLanguageChanged([](const std::string& locale) {
    std::cout << "Language switched to: " << locale << std::endl;
    // e.g. trigger a UI refresh here
});

// Switch locale at runtime
mgr.setLanguage("zh_CN");            // callback fires
std::string text = I18nVault_TR(I18nVault::I18nKey::LOGIN_BUTTON);
// => "登录"

mgr.setLanguage("en_US");            // callback fires
text = I18nVault_TR(I18nVault::I18nKey::LOGIN_BUTTON);
// => "Login"

// Unregister callback when no longer needed
mgr.removeLanguageChangedCallback(cbId);

// Remove a locale (cannot remove the currently active one)
mgr.removeLanguage("zh_CN");
```

## Project Structure

```
CMakeLists.txt                  # Top-level: project settings
src/
  CMakeLists.txt                # I18nVaultCore static library
  i18n_manager.h                # Public header (pImpl)
  i18n_manager.cpp              # Implementation
  crypto/
    CMakeLists.txt              # SM4-GCM crypto sources (compiled into Core)
    sm4.c / sm4.h               # SM4 block cipher
    gcm.c / gcm.h               # GCM mode
    sm4_gcm.c / sm4_gcm.h       # SM4-GCM unified API
    hex_utils.c / hex_utils.h   # Hex parsing utilities
generated/
  i18n_keys.h                   # Auto-generated enum & mapping functions
i18n/
  en_US.json                    # English locale (baseline)
  zh_CN.json                    # Chinese locale
  *.trs                         # Encrypted files (build-time generated)
test/
  CMakeLists.txt                # Tests
  main.cpp                      # Regression tests
tools/
  CMakeLists.txt                # CLI tools + build-time tasks
  gen_i18n_keys.py              # Generates i18n_keys.h
  i18n_diff_check.py            # Multi-locale key consistency checker
  gen_trs_files.py              # Batch TRS generation
  encrypt_i18n.c                # Encryption/decryption CLI
```

## CMake Targets

| Target | Type | Description |
|--------|------|-------------|
| `I18nVaultCore` | Static lib | Core i18n library (with SM4-GCM crypto) |
| `i18n_crypto_cli` | Executable | JSON/TRS encryption/decryption CLI |
| `i18n_test` | Executable | Regression tests |
| `i18n_keys` | Custom | Generates enum header |
| `i18n_diff_check` | Custom | Key consistency validation |
| `i18n_trs` | Custom | Build-time TRS generation |

## Public API

### Translation

| Method / Macro | Description |
|----------------|-------------|
| `I18nManager::instance()` | Get the singleton |
| `translate(key)` | Basic translation (no arguments) |
| `translate(key, {args...})` | Translate with `{0}` `{1}` placeholder substitution |
| `I18nVault_TR(key, ...)` | Convenience macro: `translate()` without / with format args |

### Language Management

| Method | Description |
|--------|-------------|
| `setI18nDirectory(dir)` | Auto-scan a directory and load all .json/.trs files. Returns count loaded, or -1 on error. |
| `reloadLanguage(locale)` | Re-read and reload a single locale's translation file. |
| `addLanguage(locale, path)` | Load a locale file (.json/.trs) and register it. First call auto-activates. |
| `setLanguage(locale)` | Switch active locale (must be previously loaded). Fires callbacks. |
| `currentLanguage()` | Return the active locale string (empty if none). |
| `setFallbackLanguage(locale)` | Set fallback locale for missing keys. |
| `fallbackLanguage()` | Return the current fallback locale string. |
| `availableLanguages()` | List all loaded locale names. |
| `availableLanguageInfos()` | Get detailed info (displayName, filePath, fileType, isActive, isFallback) for UI. |
| `removeLanguage(locale)` | Unload a locale (fails if it is currently active). |
| `onLanguageChanged(callback)` | Register a callback; returns an ID (0 means failure). |
| `removeLanguageChangedCallback(id)` | Unregister a callback by ID. |

### TRS Encryption

| Method | Description |
|--------|-------------|
| `setTrsCryptoConfig({key_hex, aad})` | Set TRS decryption parameters. |
| `clearTrsCryptoConfig()` | Wipe and clear decryption parameters. |

## Configuration

### Build-time (CMake Cache)

| Variable | Default | Description |
|----------|---------|-------------|
| `I18N_TRS_KEY_HEX` | `00112233...FF` | 32-char hex SM4 key |
| `I18N_TRS_AAD` | `i18n:v1` | Additional authenticated data |

### Runtime

Prefer injecting via `setTrsCryptoConfig()`. Falls back to environment variables if not set:

| Environment Variable | Description |
|---------------------|-------------|
| `I18N_TRS_KEY_HEX` / `I18N_SM4_KEY_HEX` | SM4 key |
| `I18N_TRS_AAD` | AAD |

## JSON File Format

Every translation file **must** contain a top-level `_LANGUAGE_NAME` key with the language's native display name:

```json
{
  "_LANGUAGE_NAME": "English",
  "LOGIN_BUTTON": "Login",
  "WELCOME_FMT": "Welcome, {0}!",
  "menu": {
    "file": "File"
  }
}
```

- `_LANGUAGE_NAME` is **required** — files without it will fail validation at build time and be rejected by `addLanguage()` at runtime.
- `_LANGUAGE_NAME` is metadata only — it is **not** included in the generated enum (`i18n_keys.h`).
- The value is used as `displayName` in `availableLanguageInfos()` for UI display.

## Adding a New Locale

1. Copy `i18n/en_US.json` to `i18n/<locale>.json`
2. Set `_LANGUAGE_NAME` to the native display name (e.g. `"繁體中文"`, `"Français"`)
3. Translate all values (keep key structure unchanged)
4. `i18n_diff_check` automatically validates key completeness and `_LANGUAGE_NAME` at build time
5. Corresponding `.trs` encrypted file is auto-generated during build

## Adding a New Key

1. Add a new key-value pair to `i18n/en_US.json`
2. Update all other locale files accordingly
3. `generated/i18n_keys.h` enum is regenerated automatically at build time
4. Use `I18nVault::I18nKey::NEW_KEY` in your code

## Build-time Data Flow

```
i18n/en_US.json ──→ gen_i18n_keys.py ──→ generated/i18n_keys.h
i18n/*.json ────→ i18n_diff_check.py ──→ Pass / Fail
i18n/*.json ────→ gen_trs_files.py ────→ i18n/*.trs
```

## CI

GitHub Actions cross-platform matrix (Ubuntu / macOS / Windows), automatically runs:

1. Strict key diff validation
2. CMake build
3. CTest tests
4. TRS artifact verification
5. SM4-GCM encrypt/decrypt roundtrip verification
6. Source archive packaging (tar.gz + zip)
7. GitHub Release publishing on `v*` tags

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
6. Source archive packaging (`git archive` → tar.gz + zip)
7. On `v*` tag push, publish source archives to GitHub Release

## Related Documentation

- TRS encryption/decryption: docs/trs_crypto.md

## Design Trade-offs

- Testing: lightweight executable tests, no heavy test frameworks
- Configuration: explicit runtime configuration preferred, environment variable fallback for compatibility
- Artifacts: TRS files auto-generated at build time to avoid manual errors