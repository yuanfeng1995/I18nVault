#include "crypto/hex_utils.h"
#include "crypto/sm4_gcm.h"
#include "i18n_keys.h"
#include "i18n_manager.h"
#include "nlohmann/json.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace
{
constexpr const char* kTmpDir         = "build/tmp";
constexpr const char* kTmpTrs         = "build/tmp/zh_CN.test.trs";
constexpr const char* kTmpMissingJson = "build/tmp/missing.json";
constexpr const char* kZhJsonPath     = "i18n/zh_CN.json";

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
    uint8_t              iv[SM4_GCM_IV_SIZE] = {0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE, 0x12, 0x34, 0x56, 0x78};
    uint8_t              tag[SM4_GCM_TAG_SIZE] = {};

    if (!parse_hex_key(key_hex, key))
        return false;

    if (sm4_gcm_encrypt(key, iv, sizeof(iv), reinterpret_cast<const uint8_t*>(aad.data()), aad.size(), pt.data(),
                        pt.size(), ct.data(), tag) != 0)
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

    uint32_t ct_len    = static_cast<uint32_t>(ct.size());
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
}  // namespace

int main()
{
    namespace fs = std::filesystem;
    fs::create_directories(kTmpDir);

    auto& mgr = I18nVault::I18nManager::instance();

    // ---- Basic Translation Tests (addLanguage) ----

    if (!mgr.addLanguage("en_US", "i18n/en_US.json"))
    {
        std::cerr << "[FAIL] addLanguage en_US failed" << std::endl;
        return 1;
    }
    // First addLanguage auto-activates
    if (mgr.currentLanguage() != "en_US")
    {
        std::cerr << "[FAIL] currentLanguage after first addLanguage, got: " << mgr.currentLanguage() << std::endl;
        return 1;
    }
    if (mgr.translate(I18nVault::I18nKey::LOGIN_BUTTON) != "Login")
    {
        std::cerr << "[FAIL] LOGIN_BUTTON translation mismatch for en_US" << std::endl;
        return 1;
    }

    // Verify I18nVault_TR() macro
    if (I18nVault_TR(I18nVault::I18nKey::LOGIN_BUTTON) != "Login")
    {
        std::cerr << "[FAIL] I18nVault_TR() macro mismatch" << std::endl;
        return 1;
    }

    // Verify translate with {0} placeholder
    if (mgr.translate(I18nVault::I18nKey::WELCOME_FMT, {"Alice"}) != "Welcome, Alice!")
    {
        std::cerr << "[FAIL] translate WELCOME_FMT mismatch" << std::endl;
        return 1;
    }

    // Verify I18nVault_TR macro with format args
    if (I18nVault_TR(I18nVault::I18nKey::DIALOG_DELETE_FMT, "photo.jpg") !=
        "Delete photo.jpg? This action cannot be undone.")
    {
        std::cerr << "[FAIL] I18nVault_TR format DIALOG_DELETE_FMT mismatch" << std::endl;
        return 1;
    }

    // ---- Missing key validation ----
    {
        std::ofstream out(kTmpMissingJson, std::ios::binary);
        out << "{\"LOGIN_BUTTON\":\"OnlyOne\"}";
    }
    if (mgr.addLanguage("bad", kTmpMissingJson))
    {
        std::cerr << "[FAIL] missing-key json should fail validation" << std::endl;
        return 1;
    }

    // ---- Missing _LANGUAGE_NAME validation ----
    {
        // Write a file with all translation keys but without _LANGUAGE_NAME
        constexpr const char* kTmpNoNameJson = "build/tmp/no_name.json";
        std::ofstream         out(kTmpNoNameJson, std::ios::binary);
        out << R"({"LOGIN_BUTTON":"X","WELCOME_FMT":"{0}","menu":{"file":"F","edit":"E","help":"H"},"dialog":{"confirm":"Y","delete_fmt":"{0}"}})";
        out.close();
        if (mgr.addLanguage("no_name", kTmpNoNameJson))
        {
            std::cerr << "[FAIL] json without _LANGUAGE_NAME should fail validation" << std::endl;
            return 1;
        }
        fs::remove(kTmpNoNameJson);
    }

    // ---- TRS encrypted file tests ----
    const std::string key_hex = "00112233445566778899AABBCCDDEEFF";
    const std::string aad     = "i18n:v1";
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
    if (!mgr.addLanguage("zh_CN_trs", kTmpTrs))
    {
        std::cerr << "[FAIL] addLanguage trs failed with correct key/aad" << std::endl;
        return 1;
    }

    // Verify TRS-loaded data matches original JSON
    nlohmann::json zh_json;
    {
        std::ifstream zh_in(kZhJsonPath);
        zh_in >> zh_json;
    }
    if (!mgr.setLanguage("zh_CN_trs"))
    {
        std::cerr << "[FAIL] setLanguage zh_CN_trs failed" << std::endl;
        return 1;
    }
    if (mgr.translate(I18nVault::I18nKey::LOGIN_BUTTON) != zh_json["LOGIN_BUTTON"].get<std::string>())
    {
        std::cerr << "[FAIL] LOGIN_BUTTON translation mismatch after trs load" << std::endl;
        return 1;
    }

    // TRS with wrong key must fail
    if (!mgr.setTrsCryptoConfig({"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", aad}))
    {
        std::cerr << "[FAIL] setting wrong-key config should still be syntactically valid" << std::endl;
        return 1;
    }
    if (mgr.addLanguage("zh_CN_bad", kTmpTrs))
    {
        std::cerr << "[FAIL] trs addLanguage should fail with wrong key" << std::endl;
        return 1;
    }

    mgr.clearTrsCryptoConfig();
    if (!mgr.setLanguage("en_US"))
    {
        std::cerr << "[FAIL] setLanguage en_US before cleanup failed" << std::endl;
        return 1;
    }
    mgr.removeLanguage("zh_CN_trs");
    fs::remove(kTmpTrs);
    fs::remove(kTmpMissingJson);

    // ---- Language Management Tests ----

    if (!mgr.addLanguage("zh_CN", "i18n/zh_CN.json"))
    {
        std::cerr << "[FAIL] addLanguage zh_CN failed" << std::endl;
        return 1;
    }

    // currentLanguage: still en_US (first addLanguage, not replaced by subsequent ones)
    if (mgr.currentLanguage() != "en_US")
    {
        std::cerr << "[FAIL] currentLanguage after second addLanguage, got: " << mgr.currentLanguage() << std::endl;
        return 1;
    }
    if (mgr.translate(I18nVault::I18nKey::LOGIN_BUTTON) != "Login")
    {
        std::cerr << "[FAIL] LOGIN_BUTTON en_US mismatch after addLanguage" << std::endl;
        return 1;
    }

    // setLanguage: switch to zh_CN
    if (!mgr.setLanguage("zh_CN"))
    {
        std::cerr << "[FAIL] setLanguage zh_CN failed" << std::endl;
        return 1;
    }
    if (mgr.currentLanguage() != "zh_CN")
    {
        std::cerr << "[FAIL] currentLanguage after setLanguage, got: " << mgr.currentLanguage() << std::endl;
        return 1;
    }
    if (mgr.translate(I18nVault::I18nKey::LOGIN_BUTTON) != "\xe7\x99\xbb\xe5\xbd\x95" /* 登录 */)
    {
        std::cerr << "[FAIL] LOGIN_BUTTON zh_CN mismatch" << std::endl;
        return 1;
    }
    if (mgr.translate(I18nVault::I18nKey::WELCOME_FMT, {"世界"}) != "\xe6\xac\xa2\xe8\xbf\x8e, \xe4\xb8\x96\xe7\x95\x8c!" /* 欢迎, 世界! */)
    {
        std::cerr << "[FAIL] WELCOME_FMT zh_CN placeholder mismatch" << std::endl;
        return 1;
    }

    // availableLanguages: must include en_US and zh_CN
    {
        auto langs  = mgr.availableLanguages();
        bool has_en = std::find(langs.begin(), langs.end(), "en_US") != langs.end();
        bool has_zh = std::find(langs.begin(), langs.end(), "zh_CN") != langs.end();
        if (!has_en || !has_zh)
        {
            std::cerr << "[FAIL] availableLanguages missing expected locales" << std::endl;
            return 1;
        }
    }

    // setFallbackLanguage
    if (!mgr.setFallbackLanguage("en_US"))
    {
        std::cerr << "[FAIL] setFallbackLanguage en_US failed" << std::endl;
        return 1;
    }
    if (mgr.fallbackLanguage() != "en_US")
    {
        std::cerr << "[FAIL] fallbackLanguage() mismatch" << std::endl;
        return 1;
    }

    // setFallbackLanguage with unknown locale must fail
    if (mgr.setFallbackLanguage("fr_FR"))
    {
        std::cerr << "[FAIL] setFallbackLanguage unknown locale should fail" << std::endl;
        return 1;
    }

    // setLanguage with unknown locale must fail
    if (mgr.setLanguage("fr_FR"))
    {
        std::cerr << "[FAIL] setLanguage unknown locale should fail" << std::endl;
        return 1;
    }
    // current locale unchanged after failure
    if (mgr.currentLanguage() != "zh_CN")
    {
        std::cerr << "[FAIL] currentLanguage changed after failed setLanguage" << std::endl;
        return 1;
    }

    // removeLanguage: cannot remove current locale
    if (mgr.removeLanguage("zh_CN"))
    {
        std::cerr << "[FAIL] removeLanguage current locale should fail" << std::endl;
        return 1;
    }

    // removeLanguage: removing fallback locale clears fallbackLocale
    // (temporarily set zh_CN as current so en_US can be removed)
    // zh_CN is already current; remove en_US (the fallback) — should succeed and clear fallback
    if (!mgr.removeLanguage("en_US"))
    {
        std::cerr << "[FAIL] removeLanguage en_US (fallback) failed" << std::endl;
        return 1;
    }
    if (!mgr.fallbackLanguage().empty())
    {
        std::cerr << "[FAIL] fallbackLocale should be cleared after its locale is removed" << std::endl;
        return 1;
    }
    // Restore en_US for subsequent tests
    if (!mgr.addLanguage("en_US", "i18n/en_US.json"))
    {
        std::cerr << "[FAIL] re-addLanguage en_US failed" << std::endl;
        return 1;
    }

    // onLanguageChanged: callback fires on switch
    std::string cb_locale;
    size_t      cb_id = mgr.onLanguageChanged([&cb_locale](const std::string& locale) { cb_locale = locale; });
    if (cb_id == 0)
    {
        std::cerr << "[FAIL] onLanguageChanged returned 0 (invalid ID)" << std::endl;
        return 1;
    }

    if (!mgr.setLanguage("en_US"))
    {
        std::cerr << "[FAIL] setLanguage en_US failed" << std::endl;
        return 1;
    }
    if (cb_locale != "en_US")
    {
        std::cerr << "[FAIL] language change callback not invoked, got: '" << cb_locale << "'" << std::endl;
        return 1;
    }

    // setLanguage same locale: callback must NOT fire
    cb_locale.clear();
    mgr.setLanguage("en_US");
    if (!cb_locale.empty())
    {
        std::cerr << "[FAIL] callback should not fire when locale is already active" << std::endl;
        return 1;
    }

    // removeLanguageChangedCallback: no callback after unregister
    mgr.removeLanguageChangedCallback(cb_id);
    cb_locale.clear();
    if (!mgr.setLanguage("zh_CN"))
    {
        std::cerr << "[FAIL] setLanguage zh_CN after unregister failed" << std::endl;
        return 1;
    }
    if (!cb_locale.empty())
    {
        std::cerr << "[FAIL] callback fired after removeLanguageChangedCallback" << std::endl;
        return 1;
    }

    // addLanguage: reload existing locale updates its translations in-place
    if (!mgr.addLanguage("zh_CN", "i18n/zh_CN.json"))
    {
        std::cerr << "[FAIL] re-addLanguage zh_CN (update) failed" << std::endl;
        return 1;
    }
    if (mgr.currentLanguage() != "zh_CN")
    {
        std::cerr << "[FAIL] currentLanguage changed unexpectedly after re-addLanguage" << std::endl;
        return 1;
    }

    // ---- setI18nDirectory Tests ----

    // Switch back to en_US so we start clean
    if (!mgr.setLanguage("en_US"))
    {
        std::cerr << "[FAIL] setLanguage en_US before directory test failed" << std::endl;
        return 1;
    }
    // Remove all languages and re-load via directory scan
    mgr.removeLanguage("zh_CN");

    // Create a fresh manager-like state: remove en_US too (need to clear current first)
    // Instead, just test setI18nDirectory on a second manager instance is not possible (singleton).
    // Test: setI18nDirectory loads files from the i18n/ directory
    {
        // Remove en_US by switching away first
        // Re-add both via directory
        mgr.removeLanguage("en_US");  // will fail since it's current — that's OK

        // setI18nDirectory on non-existent dir must return -1
        int bad = mgr.setI18nDirectory("non_existent_directory_xyz");
        if (bad != -1)
        {
            std::cerr << "[FAIL] setI18nDirectory on bad dir should return -1, got: " << bad << std::endl;
            return 1;
        }

        // setI18nDirectory on the i18n/ directory
        int count = mgr.setI18nDirectory("i18n");
        if (count < 2)
        {
            std::cerr << "[FAIL] setI18nDirectory should load at least 2 languages, got: " << count << std::endl;
            return 1;
        }

        auto langs = mgr.availableLanguages();
        bool has_en = std::find(langs.begin(), langs.end(), "en_US") != langs.end();
        bool has_zh = std::find(langs.begin(), langs.end(), "zh_CN") != langs.end();
        if (!has_en || !has_zh)
        {
            std::cerr << "[FAIL] setI18nDirectory did not load en_US and zh_CN" << std::endl;
            return 1;
        }
    }

    // ---- reloadLanguage Tests ----
    {
        if (!mgr.setLanguage("en_US"))
        {
            std::cerr << "[FAIL] setLanguage en_US before reload test failed" << std::endl;
            return 1;
        }

        // Register callback to verify reload of active language fires notification
        std::string reload_cb_locale;
        size_t reload_cb_id = mgr.onLanguageChanged(
            [&reload_cb_locale](const std::string& locale) { reload_cb_locale = locale; });

        // reloadLanguage should succeed for a loaded locale
        if (!mgr.reloadLanguage("en_US"))
        {
            std::cerr << "[FAIL] reloadLanguage en_US failed" << std::endl;
            return 1;
        }
        // Callback must fire when reloading the active language
        if (reload_cb_locale != "en_US")
        {
            std::cerr << "[FAIL] reloadLanguage of active locale should fire callback, got: '"
                      << reload_cb_locale << "'" << std::endl;
            return 1;
        }

        // Reload a non-active language should NOT fire callback
        reload_cb_locale.clear();
        if (!mgr.reloadLanguage("zh_CN"))
        {
            std::cerr << "[FAIL] reloadLanguage zh_CN failed" << std::endl;
            return 1;
        }
        if (!reload_cb_locale.empty())
        {
            std::cerr << "[FAIL] reloadLanguage of non-active locale should not fire callback" << std::endl;
            return 1;
        }

        mgr.removeLanguageChangedCallback(reload_cb_id);

        // Translation should still work after reload
        if (mgr.translate(I18nVault::I18nKey::LOGIN_BUTTON) != "Login")
        {
            std::cerr << "[FAIL] LOGIN_BUTTON mismatch after reloadLanguage" << std::endl;
            return 1;
        }

        // reloadLanguage on unknown locale must fail
        if (mgr.reloadLanguage("fr_FR"))
        {
            std::cerr << "[FAIL] reloadLanguage unknown locale should fail" << std::endl;
            return 1;
        }
    }

    // ---- availableLanguageInfos Tests ----
    {
        auto infos = mgr.availableLanguageInfos();
        if (infos.size() < 2)
        {
            std::cerr << "[FAIL] availableLanguageInfos should have at least 2 entries, got: " << infos.size()
                      << std::endl;
            return 1;
        }

        bool foundActiveEn = false;
        bool foundZhName   = false;
        for (const auto& info : infos)
        {
            if (info.locale == "en_US")
            {
                foundActiveEn = info.isActive;
                if (info.displayName != "English")
                {
                    std::cerr << "[FAIL] en_US displayName should be 'English', got: " << info.displayName
                              << std::endl;
                    return 1;
                }
                if (info.fileType != "json")
                {
                    std::cerr << "[FAIL] en_US fileType should be 'json', got: " << info.fileType << std::endl;
                    return 1;
                }
                if (info.filePath.empty())
                {
                    std::cerr << "[FAIL] en_US filePath should not be empty" << std::endl;
                    return 1;
                }
            }
            if (info.locale == "zh_CN")
            {
                foundZhName = (info.displayName == "\xe7\xae\x80\xe4\xbd\x93\xe4\xb8\xad\xe6\x96\x87");
                if (!foundZhName)
                {
                    std::cerr << "[FAIL] zh_CN displayName should be '\xe7\xae\x80\xe4\xbd\x93\xe4\xb8\xad\xe6\x96\x87', got: " << info.displayName
                              << std::endl;
                    return 1;
                }
            }
        }
        if (!foundActiveEn)
        {
            std::cerr << "[FAIL] en_US should be marked isActive in availableLanguageInfos" << std::endl;
            return 1;
        }
        if (!foundZhName)
        {
            std::cerr << "[FAIL] zh_CN displayName not found in availableLanguageInfos" << std::endl;
            return 1;
        }
    }

    std::cout << "[OK] all i18n manager tests passed" << std::endl;
    return 0;
}