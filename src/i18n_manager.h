#pragma once
#include "i18n_keys.h"

#include <initializer_list>
#include <memory>
#include <string>

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

    static I18nManager& instance();

    /// Load or reload an i18n file (.json or .trs).
    bool reload(const std::string& path);

    /// Translate an enum key to its localized string.
    std::string translate(I18nKey key);

    /// Translate with positional arguments: {0}, {1}, ... are replaced.
    std::string translateFmt(I18nKey key, std::initializer_list<std::string> args);

    /// Set runtime TRS decryption parameters.
    bool setTrsCryptoConfig(const TrsCryptoConfig& config);

    /// Clear runtime TRS decryption parameters.
    void clearTrsCryptoConfig();

    I18nManager(const I18nManager&)            = delete;
    I18nManager& operator=(const I18nManager&) = delete;

private:
    I18nManager();
    ~I18nManager();

    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

}  // namespace I18nVault

/// Convenience macro: I18nVault_TR(I18nKey::XXX) => translated string.
#define I18nVault_TR(key) (::I18nVault::I18nManager::instance().translate(key))

/// Format macro: I18nVault_TR_FMT(I18nKey::XXX, "a", "b") replaces {0},{1},...
#define I18nVault_TR_FMT(key, ...) \
    (::I18nVault::I18nManager::instance().translateFmt(key, {__VA_ARGS__}))
