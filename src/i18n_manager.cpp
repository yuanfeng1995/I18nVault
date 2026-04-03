#include "i18n_manager.h"

#include "crypto/hex_utils.h"
#include "crypto/sm4_gcm.h"
#include "nlohmann/json.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

using I18nVault::I18nKey;
using I18nVault::i18n_keys_string;
using I18nVault::I18N_KEY_COUNT;

namespace
{
constexpr const char* kTrsMagic = "TRS1";
constexpr uint8_t     kTrsVersion = 1;
constexpr uint8_t     kTrsReserved = 0;

bool ends_with(const std::string& value, const std::string& suffix)
{
    if (value.size() < suffix.size())
        return false;
    return value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}



bool read_file_binary(const std::string& path, std::vector<uint8_t>& out)
{
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open())
        return false;

    f.seekg(0, std::ios::end);
    auto size = f.tellg();
    if (size < 0)
        return false;

    f.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(size));
    if (!out.empty())
        f.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(out.size()));

    return f.good() || f.eof();
}

uint32_t read_u32_le(const uint8_t* p)
{
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8)
           | (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

bool decrypt_trs_to_json_text(const std::string& path, const uint8_t key[SM4_GCM_KEY_SIZE], const std::string& aad,
                              std::string& json_text)
{
    std::vector<uint8_t> blob;
    if (!read_file_binary(path, blob))
    {
        std::cerr << "Failed to read trs file: " << path << std::endl;
        return false;
    }

    constexpr size_t kHeaderMin = 4 + 1 + 1 + 1 + 1 + 4;
    if (blob.size() < kHeaderMin)
    {
        std::cerr << "Invalid trs file, too short: " << path << std::endl;
        return false;
    }

    size_t off = 0;
    if (std::memcmp(blob.data(), kTrsMagic, 4) != 0)
    {
        std::cerr << "Invalid trs magic: " << path << std::endl;
        return false;
    }
    off += 4;

    uint8_t version = blob[off++];
    uint8_t iv_len = blob[off++];
    uint8_t tag_len = blob[off++];
    uint8_t reserved = blob[off++];
    uint32_t ct_len = read_u32_le(blob.data() + off);
    off += 4;

    if (version != kTrsVersion || reserved != kTrsReserved || iv_len != SM4_GCM_IV_SIZE || tag_len != SM4_GCM_TAG_SIZE)
    {
        std::cerr << "Unsupported trs header: " << path << std::endl;
        return false;
    }

    if (off + iv_len + tag_len + ct_len != blob.size())
    {
        std::cerr << "Invalid trs length fields: " << path << std::endl;
        return false;
    }

    const uint8_t* iv = blob.data() + off;
    off += iv_len;
    const uint8_t* tag = blob.data() + off;
    off += tag_len;
    const uint8_t* ct = blob.data() + off;

    std::vector<uint8_t> pt(ct_len);
    if (sm4_gcm_decrypt(key, iv, iv_len, reinterpret_cast<const uint8_t*>(aad.data()), aad.size(), ct, ct_len,
                        pt.data(), tag)
        != 0)
    {
        std::cerr << "Failed to decrypt trs file (auth verify failed): " << path << std::endl;
        return false;
    }

    json_text.assign(reinterpret_cast<const char*>(pt.data()), pt.size());
    return true;
}

std::vector<std::string> collect_required_keys()
{
    std::vector<std::string> keys;
    for (uint16_t i = 0; i < I18N_KEY_COUNT; ++i)
    {
        const char* k = i18n_keys_string(static_cast<I18nKey>(i));
        if (k && k[0] != '\0')
            keys.emplace_back(k);
    }

    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
    return keys;
}
} // anonymous namespace

namespace I18nVault {

struct I18nManager::Impl
{
    mutable std::shared_mutex                    mu;
    std::unordered_map<std::string, std::string> data;
    std::optional<std::vector<uint8_t>>          trs_key;
    std::string                                  trs_aad;
};

I18nManager::I18nManager()  : pImpl_(std::make_unique<Impl>()) {}
I18nManager::~I18nManager() = default;

bool I18nManager::setTrsCryptoConfig(const TrsCryptoConfig& config)
{
    if (config.key_hex.size() != SM4_GCM_KEY_SIZE * 2)
        return false;

    std::vector<uint8_t> key(SM4_GCM_KEY_SIZE);
    if (hex_parse(config.key_hex.c_str(), key.data(), SM4_GCM_KEY_SIZE) != 0)
        return false;

    {
        std::unique_lock lock(pImpl_->mu);
        pImpl_->trs_key = std::move(key);
        pImpl_->trs_aad = config.aad;
    }
    return true;
}

void I18nManager::clearTrsCryptoConfig()
{
    std::unique_lock lock(pImpl_->mu);
    pImpl_->trs_key.reset();
    pImpl_->trs_aad.clear();
}

I18nManager& I18nManager::instance()
{
    static I18nManager inst;
    return inst;
}

std::string I18nManager::translate(I18nKey key)
{
    std::shared_lock lock(pImpl_->mu);
    auto             it = pImpl_->data.find(i18n_keys_string(key));
    if (it != pImpl_->data.end())
        return it->second;
    return "";
}

std::string I18nManager::translateFmt(I18nKey key, std::initializer_list<std::string> args)
{
    std::string result = translate(key);
    size_t idx = 0;
    for (const auto& arg : args)
    {
        std::string placeholder = "{" + std::to_string(idx) + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos)
        {
            result.replace(pos, placeholder.size(), arg);
            pos += arg.size();
        }
        ++idx;
    }
    return result;
}

static void flatten(const nlohmann::json& j, const std::string& prefix,
                    std::unordered_map<std::string, std::string>& out)
{
    for (auto it = j.begin(); it != j.end(); ++it)
    {
        std::string key = prefix.empty() ? it.key() : prefix + "." + it.key();
        if (it->is_object())
        {
            flatten(*it, key, out);
        }
        else if (it->is_string())
        {
            out[key] = it->get<std::string>();
        }
        else
        {
            std::cerr << "Skip non-string key: " << key << std::endl;
        }
    }
}

bool I18nManager::reload(const std::string& path)
{
    nlohmann::json j;
    try
    {
        if (ends_with(path, ".trs"))
        {
            std::vector<uint8_t> key;
            std::string          aad;
            {
                std::shared_lock lock(pImpl_->mu);
                if (pImpl_->trs_key.has_value())
                {
                    key = *pImpl_->trs_key;
                    aad = pImpl_->trs_aad;
                }
            }

            // Backward-compatible fallback to environment variables.
            if (key.empty())
            {
                const char* key_hex = std::getenv("I18N_TRS_KEY_HEX");
                if (!key_hex)
                    key_hex = std::getenv("I18N_SM4_KEY_HEX");

                uint8_t env_key[SM4_GCM_KEY_SIZE] = {};
                if (hex_parse(key_hex, env_key, SM4_GCM_KEY_SIZE) != 0)
                {
                    std::cerr << "TRS key is not configured. Call setTrsCryptoConfig() or set I18N_TRS_KEY_HEX."
                              << std::endl;
                    return false;
                }

                key.assign(env_key, env_key + SM4_GCM_KEY_SIZE);
                const char* aad_env = std::getenv("I18N_TRS_AAD");
                aad = aad_env ? aad_env : "";
            }

            std::string json_text;
            if (!decrypt_trs_to_json_text(path, key.data(), aad, json_text))
                return false;
            j = nlohmann::json::parse(json_text);
        }
        else
        {
            std::ifstream f(path);
            if (!f.is_open())
                return false;
            f >> j;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to parse i18n file " << path << ": " << e.what() << std::endl;
        return false;
    }

    std::unordered_map<std::string, std::string> new_data;
    flatten(j, "", new_data);

    static const std::vector<std::string> required_keys = collect_required_keys();
    for (const auto& key : required_keys)
    {
        if (new_data.find(key) == new_data.end())
        {
            std::cerr << "Missing required key: " << key << std::endl;
            return false;
        }
    }

    {
        std::unique_lock lock(pImpl_->mu);
        pImpl_->data.swap(new_data);
    }
    return true;
}

} // namespace I18nVault