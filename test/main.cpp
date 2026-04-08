#include "crypto/hex_utils.h"
#include "crypto/sm4_gcm.h"
#include "i18n_keys.h"
#include "i18n_manager.h"
#include "nlohmann/json.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <thread>
#include <vector>

namespace
{
constexpr const char* kTmpDir         = "build/tmp";
constexpr const char* kTmpTrs         = "build/tmp/zh_CN.test.trs";
constexpr const char* kTmpMissingJson = "build/tmp/missing.json";
constexpr const char* kTmpHotJson     = "build/tmp/hot_reload.json";
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

    int pass = 0;
    int fail = 0;

#define CHECK(expr, msg)                                              \
    do {                                                              \
        if (expr) { ++pass; }                                         \
        else { ++fail; std::cerr << "[FAIL] " << msg << std::endl; } \
    } while (0)

    auto& mgr = I18nVault::I18nManager::instance();

    // ================================================================
    // 1. Compile-time i18n (constexpr)
    // ================================================================
    {
        constexpr auto login_en = I18nVault::i18n<I18nVault::Key_LOGIN_BUTTON, I18nVault::Lang_en_US>();
        constexpr auto login_zh = I18nVault::i18n<I18nVault::Key_LOGIN_BUTTON, I18nVault::Lang_zh_CN>();
        CHECK(login_en == "Login", "CT: LOGIN_BUTTON en_US");
        CHECK(login_zh == "登录",  "CT: LOGIN_BUTTON zh_CN");

        constexpr auto menu_file = I18nVault::i18n<I18nVault::Key_MENU_FILE, I18nVault::Lang_en_US>();
        CHECK(menu_file == "File", "CT: MENU_FILE en_US");

        constexpr auto confirm_zh = I18nVault::i18n<I18nVault::Key_DIALOG_CONFIRM, I18nVault::Lang_zh_CN>();
        CHECK(confirm_zh == "确定吗？", "CT: DIALOG_CONFIRM zh_CN");
    }

    // ================================================================
    // 2. Compile-time macros (I18N_CT_SELECT, I18N_CT_DEFAULT)
    // ================================================================
    {
        constexpr auto v1 = I18N_CT_SELECT(MENU_EDIT, en_US);
        CHECK(v1 == "Edit", "CT macro: I18N_CT_SELECT(MENU_EDIT, en_US)");

        constexpr auto v2 = I18N_CT_DEFAULT(MENU_HELP);
        CHECK(v2 == "Help", "CT macro: I18N_CT_DEFAULT(MENU_HELP)");
    }

    // ================================================================
    // 3. i18n_fmt variadic (compile-time key + runtime format)
    // ================================================================
    {
        auto msg1 = I18nVault::i18n_fmt(
            I18nVault::i18n<I18nVault::Key_WELCOME_FMT, I18nVault::Lang_en_US>(),
            "Alice");
        CHECK(msg1 == "Welcome, Alice!", "i18n_fmt: WELCOME_FMT single arg");

        auto msg2 = I18nVault::i18n_fmt(
            I18nVault::i18n<I18nVault::Key_DIALOG_DELETE_FMT, I18nVault::Lang_zh_CN>(),
            "photo.jpg");
        CHECK(msg2 == "删除 photo.jpg？此操作无法撤销。", "i18n_fmt: DIALOG_DELETE_FMT zh_CN");

        // I18N_CT_FMT macro
        auto msg3 = I18N_CT_FMT(WELCOME_FMT, en_US, "Bob");
        CHECK(msg3 == "Welcome, Bob!", "I18N_CT_FMT: WELCOME_FMT");

        auto msg4 = I18N_CT_FMT_DEFAULT(WELCOME_FMT, "Charlie");
        CHECK(msg4 == "Welcome, Charlie!", "I18N_CT_FMT_DEFAULT: WELCOME_FMT");

        // i18n_fmt with no args returns template as-is
        auto msg5 = I18nVault::i18n_fmt(
            I18nVault::i18n<I18nVault::Key_LOGIN_BUTTON, I18nVault::Lang_en_US>());
        CHECK(msg5 == "Login", "i18n_fmt: no args passthrough");

        // I18N_CT_FMT with no extra args via __VA_OPT__
        auto msg6 = I18N_CT_FMT(LOGIN_BUTTON, en_US);
        CHECK(msg6 == "Login", "I18N_CT_FMT: 0 extra args");
    }

    // ================================================================
    // 4. Runtime i18n – JSON reload + translate
    // ================================================================
    if (!mgr.reload("i18n/en_US.json"))
    {
        std::cerr << "[FAIL] reload en_US.json failed" << std::endl;
        return 1;
    }
    CHECK(mgr.translate(I18nVault::I18nKey::LOGIN_BUTTON) == "Login",
          "RT: LOGIN_BUTTON en_US");

    CHECK(I18nVault_TR(I18nVault::I18nKey::LOGIN_BUTTON) == "Login",
          "RT: I18nVault_TR macro");

    CHECK(mgr.translate(I18nVault::I18nKey::WELCOME_FMT, {"Alice"}) == "Welcome, Alice!",
          "RT: translate WELCOME_FMT with placeholder");

    CHECK(I18nVault_TR(I18nVault::I18nKey::DIALOG_DELETE_FMT, "photo.jpg") ==
              "Delete photo.jpg? This action cannot be undone.",
          "RT: I18nVault_TR format DIALOG_DELETE_FMT");

    // ================================================================
    // 5. Strong-typed runtime translate<KeyType>
    // ================================================================
    {
        auto v1 = mgr.translate<I18nVault::Key_LOGIN_BUTTON>();
        CHECK(v1 == "Login", "RT strong-typed: translate<Key_LOGIN_BUTTON>()");

        auto v2 = mgr.translate<I18nVault::Key_WELCOME_FMT>({"World"});
        CHECK(v2 == "Welcome, World!", "RT strong-typed: translate<Key_WELCOME_FMT>({\"World\"})");
    }

    // ================================================================
    // 6. Missing key validation
    // ================================================================
    {
        std::ofstream out(kTmpMissingJson, std::ios::binary);
        out << "{\"LOGIN_BUTTON\":\"OnlyOne\"}";
    }
    CHECK(!mgr.reload(kTmpMissingJson),
          "RT: missing-key json should fail validation");

    // ================================================================
    // 7. TRS encryption / decryption
    // ================================================================
    {
        const std::string key_hex = "00112233445566778899AABBCCDDEEFF";
        const std::string aad     = "i18n:v1";
        CHECK(write_trs_file(kZhJsonPath, kTmpTrs, key_hex, aad),
              "TRS: generate test trs file");

        CHECK(mgr.setTrsCryptoConfig({key_hex, aad}),
              "TRS: setTrsCryptoConfig");

        CHECK(mgr.reload(kTmpTrs),
              "TRS: reload with correct key/aad");

        nlohmann::json zh_json;
        { std::ifstream zh_in(kZhJsonPath); zh_in >> zh_json; }
        CHECK(mgr.translate(I18nVault::I18nKey::LOGIN_BUTTON) == zh_json["LOGIN_BUTTON"].get<std::string>(),
              "TRS: LOGIN_BUTTON matches zh_CN after trs reload");

        // Wrong key should fail
        CHECK(mgr.setTrsCryptoConfig({"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", aad}),
              "TRS: wrong-key config syntactically valid");
        CHECK(!mgr.reload(kTmpTrs),
              "TRS: reload should fail with wrong key");

        mgr.clearTrsCryptoConfig();
    }

    // ================================================================
    // 8. Hot-reload (file watching)
    // ================================================================
    {
        // Prepare initial json
        {
            std::ofstream f(kTmpHotJson);
            f << R"({
  "LOGIN_BUTTON": "Initial",
  "WELCOME_FMT": "Hi, {0}!",
  "menu": {"file": "F", "edit": "E", "help": "H"},
  "dialog": {"confirm": "OK?", "delete_fmt": "Del {0}?"}
})";
        }
        CHECK(mgr.reload(kTmpHotJson), "HotReload: initial load");
        CHECK(mgr.translate(I18nVault::I18nKey::LOGIN_BUTTON) == "Initial",
              "HotReload: initial value");

        // Set up callback to detect reload
        std::atomic<int> reload_count{0};
        std::atomic<bool> last_ok{false};
        mgr.setReloadCallback([&](bool ok, const std::string&) {
            last_ok = ok;
            reload_count++;
        });

        // Start watching with 100ms poll
        CHECK(mgr.watchFile(kTmpHotJson, 100), "HotReload: watchFile started");
        CHECK(mgr.isWatching(), "HotReload: isWatching == true");

        // Wait a bit, then modify the file
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        {
            std::ofstream f(kTmpHotJson);
            f << R"({
  "LOGIN_BUTTON": "Updated",
  "WELCOME_FMT": "Hey, {0}!",
  "menu": {"file": "F2", "edit": "E2", "help": "H2"},
  "dialog": {"confirm": "Sure?", "delete_fmt": "Remove {0}?"}
})";
        }

        // Wait for the watcher to detect and reload
        for (int i = 0; i < 30 && reload_count.load() == 0; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        CHECK(reload_count.load() > 0, "HotReload: callback was invoked");
        CHECK(last_ok.load(), "HotReload: reload succeeded");
        CHECK(mgr.translate(I18nVault::I18nKey::LOGIN_BUTTON) == "Updated",
              "HotReload: value updated after file change");

        mgr.stopWatching();
        CHECK(!mgr.isWatching(), "HotReload: isWatching == false after stop");

        mgr.setReloadCallback(nullptr);
    }

    // ================================================================
    // Cleanup
    // ================================================================
    fs::remove(kTmpTrs);
    fs::remove(kTmpMissingJson);
    fs::remove(kTmpHotJson);

    // ================================================================
    // Summary
    // ================================================================
    std::cout << "\n========================================\n";
    std::cout << "  Results: " << pass << " passed, " << fail << " failed\n";
    std::cout << "========================================\n";

    if (fail > 0)
    {
        std::cerr << "[FAIL] " << fail << " test(s) failed" << std::endl;
        return 1;
    }

    std::cout << "[OK] all " << pass << " tests passed" << std::endl;
    return 0;

#undef CHECK
}