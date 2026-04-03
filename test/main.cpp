#include "i18n_keys.h"
#include "i18n_manager.h"
#include "crypto/hex_utils.h"
#include "crypto/sm4_gcm.h"
#include "nlohmann/json.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace
{
constexpr const char* kTmpDir = "build/tmp";
constexpr const char* kTmpTrs = "build/tmp/zh_CN.test.trs";
constexpr const char* kTmpMissingJson = "build/tmp/missing.json";
constexpr const char* kZhJsonPath = "i18n/zh_CN.json";

bool parse_hex_key(const std::string& hex, uint8_t out[SM4_GCM_KEY_SIZE])
{
    return hex_parse(hex.c_str(), out, SM4_GCM_KEY_SIZE) == 0;
}

bool write_trs_file(const std::string& json_path, const std::string& trs_path, const std::string& key_hex,
                    const std::string& aad)
{
    std::ifstream in(json_path, std::ios::binary);
    if (!in.is_open())
        return false;

    std::vector<uint8_t> pt((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::vector<uint8_t> ct(pt.size());
    uint8_t              key[SM4_GCM_KEY_SIZE] = {};
    uint8_t              iv[SM4_GCM_IV_SIZE] = {0x10, 0x32, 0x54, 0x76, 0x98, 0xBA,
                                   0xDC, 0xFE, 0x12, 0x34, 0x56, 0x78};
    uint8_t              tag[SM4_GCM_TAG_SIZE] = {};

    if (!parse_hex_key(key_hex, key))
        return false;

    if (sm4_gcm_encrypt(key, iv, sizeof(iv), reinterpret_cast<const uint8_t*>(aad.data()), aad.size(), pt.data(),
                        pt.size(), ct.data(), tag)
        != 0)
    {
        return false;
    }

    std::ofstream out(trs_path, std::ios::binary);
    if (!out.is_open())
        return false;

    uint8_t header[] = {
        'T', 'R', 'S', '1', 1, SM4_GCM_IV_SIZE, SM4_GCM_TAG_SIZE, 0,
    };
    out.write(reinterpret_cast<const char*>(header), sizeof(header));

    uint32_t ct_len = static_cast<uint32_t>(ct.size());
    uint8_t  len_le[4] = {
        static_cast<uint8_t>(ct_len & 0xFF),
        static_cast<uint8_t>((ct_len >> 8) & 0xFF),
        static_cast<uint8_t>((ct_len >> 16) & 0xFF),
        static_cast<uint8_t>((ct_len >> 24) & 0xFF),
    };
    out.write(reinterpret_cast<const char*>(len_le), sizeof(len_le));
    out.write(reinterpret_cast<const char*>(iv), sizeof(iv));
    out.write(reinterpret_cast<const char*>(tag), sizeof(tag));
    out.write(reinterpret_cast<const char*>(ct.data()), static_cast<std::streamsize>(ct.size()));
    return static_cast<bool>(out);
}
} // namespace

int main()
{
    namespace fs = std::filesystem;
    fs::create_directories(kTmpDir);

    auto& mgr = I18nManager::instance();

    if (!mgr.reload("i18n/en_US.json"))
    {
        std::cerr << "[FAIL] reload en_US.json failed" << std::endl;
        return 1;
    }
    if (mgr.translate(I18nKey::LOGIN_BUTTON) != "Login")
    {
        std::cerr << "[FAIL] LOGIN_BUTTON translation mismatch for en_US" << std::endl;
        return 1;
    }

    {
        std::ofstream out(kTmpMissingJson, std::ios::binary);
        out << "{\"LOGIN_BUTTON\":\"OnlyOne\"}";
    }
    if (mgr.reload(kTmpMissingJson))
    {
        std::cerr << "[FAIL] missing-key json should fail on first-load validation" << std::endl;
        return 1;
    }

    const std::string key_hex = "00112233445566778899AABBCCDDEEFF";
    const std::string aad = "i18n:v1";
    if (!write_trs_file(kZhJsonPath, kTmpTrs, key_hex, aad))
    {
        std::cerr << "[FAIL] failed to generate test trs file" << std::endl;
        return 1;
    }

    if (!mgr.setTrsCryptoConfig({key_hex, aad}))
    {
        std::cerr << "[FAIL] setTrsCryptoConfig failed" << std::endl;
        return 1;
    }
    if (!mgr.reload(kTmpTrs))
    {
        std::cerr << "[FAIL] reload trs failed with correct key/aad" << std::endl;
        return 1;
    }

    nlohmann::json zh_json;
    {
        std::ifstream zh_in(kZhJsonPath);
        zh_in >> zh_json;
    }
    if (mgr.translate(I18nKey::LOGIN_BUTTON) != zh_json["LOGIN_BUTTON"].get<std::string>())
    {
        std::cerr << "[FAIL] LOGIN_BUTTON translation mismatch after trs reload" << std::endl;
        return 1;
    }

    if (!mgr.setTrsCryptoConfig({"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", aad}))
    {
        std::cerr << "[FAIL] setting wrong-key config should still be syntactically valid" << std::endl;
        return 1;
    }
    if (mgr.reload(kTmpTrs))
    {
        std::cerr << "[FAIL] trs reload should fail with wrong key" << std::endl;
        return 1;
    }

    mgr.clearTrsCryptoConfig();
    fs::remove(kTmpTrs);
    fs::remove(kTmpMissingJson);
    std::cout << "[OK] all i18n manager tests passed" << std::endl;
    return 0;
}