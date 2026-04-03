#pragma once
#include "i18n_keys.h"

#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

class I18nManager
{
public:
    struct TrsCryptoConfig
    {
        std::string key_hex;
        std::string aad;
    };

    static I18nManager& instance();
    std::string         translate(I18nKey key);
    bool                reload(const std::string& path);
    bool                setTrsCryptoConfig(const TrsCryptoConfig& config);
    void                clearTrsCryptoConfig();

    I18nManager(const I18nManager&)            = delete;
    I18nManager& operator=(const I18nManager&) = delete;

private:
    I18nManager()  = default;
    ~I18nManager() = default;
    mutable std::shared_mutex                    mu_;
    std::unordered_map<std::string, std::string> data_;
    std::optional<std::vector<uint8_t>>          trs_key_;
    std::string                                  trs_aad_;
};