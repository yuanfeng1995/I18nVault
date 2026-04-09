#pragma once
#include "i18n_keys.h"

#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

namespace I18nVault
{

class I18nManager
{
public:
    // ---- Public API ----

    struct TrsCryptoConfig
    {
        std::string key_hex;
        std::string aad;
    };

    struct LanguageInfo
    {
        std::string locale;      // e.g. "en_US"
        std::string displayName; // e.g. "English", "简体中文"
        std::string filePath;    // e.g. "i18n/en_US.json"
        std::string fileType;    // "json" or "trs"
        bool        isActive   = false;
        bool        isFallback = false;
    };

    using LanguageChangedCallback = std::function<void(const std::string& newLocale)>;

    static I18nManager& instance();

    // ---- Directory-based Language Management ----

    /// Set the working directory for translation files and auto-scan/load all
    /// .json and .trs files found.  When both formats exist for the same locale,
    /// .trs is preferred if TRS crypto is configured, otherwise .json is used.
    /// Returns the number of languages successfully loaded, or -1 on error.
    int setI18nDirectory(const std::string& dir);

    /// Re-read and reload the translation file for a single locale.
    /// The locale must have been loaded via addLanguage() or setI18nDirectory().
    bool reloadLanguage(const std::string& locale);

    // ---- Language Management ----

    /// Load a language file and register it under the given locale name.
    /// Example: addLanguage("en_US", "i18n/en_US.json")
    bool addLanguage(const std::string& locale, const std::string& path);

    /// Remove a previously loaded language. Cannot remove the current language.
    bool removeLanguage(const std::string& locale);

    /// Switch to a previously loaded language.
    bool setLanguage(const std::string& locale);

    /// Get the current active language locale (empty if none set).
    std::string currentLanguage() const;

    /// Set a fallback language for missing keys. Must be a loaded locale.
    bool setFallbackLanguage(const std::string& locale);

    /// Get the fallback language locale (empty if none set).
    std::string fallbackLanguage() const;

    /// Get list of all loaded language locales.
    std::vector<std::string> availableLanguages() const;

    /// Get detailed info for all loaded languages (for UI display).
    std::vector<LanguageInfo> availableLanguageInfos() const;

    /// Register a callback invoked when the active language changes.
    /// Returns an ID that can be used to unregister.
    size_t onLanguageChanged(LanguageChangedCallback callback);

    /// Unregister a language-changed callback by ID.
    void removeLanguageChangedCallback(size_t id);

    // ---- Translation ----

    /// Translate an enum key, replacing {0}, {1}, ... with given arguments.
    std::string translate(I18nKey key, std::initializer_list<std::string> args = {});

    /// Set runtime TRS decryption parameters.
    bool setTrsCryptoConfig(const TrsCryptoConfig& config);

    /// Clear runtime TRS decryption parameters.
    void clearTrsCryptoConfig();

    I18nManager(const I18nManager&)            = delete;
    I18nManager& operator=(const I18nManager&) = delete;

private:
    I18nManager();
    ~I18nManager();

    std::string tr(I18nKey key);

    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

}  // namespace I18nVault

/// Convenience macro: I18nVault_TR(key) or I18nVault_TR(key, "a", "b") for {0},{1},... formatting.
#define I18nVault_TR(key, ...) \
    (::I18nVault::I18nManager::instance().translate(key, {__VA_ARGS__}))
