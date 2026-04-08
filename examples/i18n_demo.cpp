#include <iostream>
#include <string_view>
#include "i18n_manager.h"

using namespace I18nVault;

int main()
{
    std::cout << "========================================\n";
    std::cout << "I18nVault - Compile-time i18n Demo\n";
    std::cout << "========================================\n\n";

    // ================================================================
    // Example 1: Compile-time strongly-typed i18n
    // ================================================================
    std::cout << "1. Compile-time Strongly-Typed i18n:\n";
    std::cout << "   (Zero runtime cost, type-safe)\n\n";

    // Each key is a unique type - compile-time type checking!
    constexpr auto login_en = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
    constexpr auto login_zh = i18n<Key_LOGIN_BUTTON, Lang_zh_CN>();

    std::cout << "   LOGIN_BUTTON (en_US): " << login_en << "\n";
    std::cout << "   LOGIN_BUTTON (zh_CN): " << login_zh << "\n\n";

    // ================================================================
    // Example 2: Menu strings (compile-time optimized)
    // ================================================================
    std::cout << "2. Menu Strings (Compile-time Constants):\n\n";

    constexpr auto menu_file_en = i18n<Key_MENU_FILE, Lang_en_US>();
    constexpr auto menu_edit_en = i18n<Key_MENU_EDIT, Lang_en_US>();
    constexpr auto menu_help_en = i18n<Key_MENU_HELP, Lang_en_US>();

    std::cout << "   English Menu:\n";
    std::cout << "     File: " << menu_file_en << "\n";
    std::cout << "     Edit: " << menu_edit_en << "\n";
    std::cout << "     Help: " << menu_help_en << "\n\n";

    constexpr auto menu_file_zh = i18n<Key_MENU_FILE, Lang_zh_CN>();
    constexpr auto menu_edit_zh = i18n<Key_MENU_EDIT, Lang_zh_CN>();
    constexpr auto menu_help_zh = i18n<Key_MENU_HELP, Lang_zh_CN>();

    std::cout << "   Chinese Menu:\n";
    std::cout << "     File: " << menu_file_zh << "\n";
    std::cout << "     Edit: " << menu_edit_zh << "\n";
    std::cout << "     Help: " << menu_help_zh << "\n\n";

    // ================================================================
    // Example 3: Dialog strings
    // ================================================================
    std::cout << "3. Dialog Strings (With Format Templates):\n\n";

    constexpr auto dialog_confirm_en = i18n<Key_DIALOG_CONFIRM, Lang_en_US>();
    constexpr auto welcome_fmt_en = i18n<Key_WELCOME_FMT, Lang_en_US>();

    std::cout << "   English:\n";
    std::cout << "     Confirm: " << dialog_confirm_en << "\n";
    std::cout << "     Welcome Template: " << welcome_fmt_en << "\n\n";

    constexpr auto dialog_confirm_zh = i18n<Key_DIALOG_CONFIRM, Lang_zh_CN>();
    constexpr auto welcome_fmt_zh = i18n<Key_WELCOME_FMT, Lang_zh_CN>();

    std::cout << "   Chinese:\n";
    std::cout << "     Confirm: " << dialog_confirm_zh << "\n";
    std::cout << "     Welcome Template: " << welcome_fmt_zh << "\n\n";

    // ================================================================
    // Example 4: Type Safety - Keys are strongly typed
    // ================================================================
    std::cout << "4. Type Safety Demo:\n";
    std::cout << "   ✅ Key_LOGIN_BUTTON is type: Key_LOGIN_BUTTON\n";
    std::cout << "   ✅ Key_MENU_FILE is type: Key_MENU_FILE\n";
    std::cout << "   ❌ These cannot be mixed by compiler!\n";
    std::cout << "   ❌ Typos cause compile-time errors!\n\n";

    // ================================================================
    // Example 5: Runtime i18n (backward compatible)
    // ================================================================
    std::cout << "5. Runtime i18n (Backward Compatible):\n";
    std::cout << "   (For dynamic language selection and parameter substitution)\n\n";

    // Classic runtime i18n still works
    auto runtime_text = I18nVault_TR(I18nKey::LOGIN_BUTTON);
    std::cout << "   I18nVault_TR(LOGIN_BUTTON): " << runtime_text << "\n";

    // With parameter substitution
    auto welcome_msg = I18nVault_TR(I18nKey::WELCOME_FMT, "Alice");
    std::cout << "   I18nVault_TR(WELCOME_FMT, 'Alice'): " << welcome_msg << "\n";

    auto delete_msg = I18nVault_TR(I18nKey::DIALOG_DELETE_FMT, "important_file.txt");
    std::cout << "   I18nVault_TR(DIALOG_DELETE_FMT, 'important_file.txt'): "
              << delete_msg << "\n\n";

    // ================================================================
    // Performance Notes
    // ================================================================
    std::cout << "6. Performance Characteristics:\n\n";

    std::cout << "   Compile-time approach (i18n<Key_X, Lang_Y>()):\n";
    std::cout << "     - Zero runtime cost\n";
    std::cout << "     - String constant reference\n";
    std::cout << "     - Type-safe, compile-time checked\n";
    std::cout << "     - Best for UI constants\n\n";

    std::cout << "   Runtime approach (I18nVault_TR):\n";
    std::cout << "     - Hash table lookup cost\n";
    std::cout << "     - Supports parameter substitution\n";
    std::cout << "     - Supports dynamic language selection\n";
    std::cout << "     - Best for dynamic content\n\n";

    // ================================================================
    // Usage in constexpr
    // ================================================================
    std::cout << "7. Using in constexpr contexts:\n\n";

    // All translations can be used in compile-time computations
    constexpr std::string_view all_menus[] = {
        i18n<Key_MENU_FILE, Lang_en_US>(),
        i18n<Key_MENU_EDIT, Lang_en_US>(),
        i18n<Key_MENU_HELP, Lang_en_US>(),
    };

    std::cout << "   Compile-time menu array:\n";
    for (size_t i = 0; i < std::size(all_menus); ++i)
    {
        std::cout << "     [" << i << "] " << all_menus[i] << "\n";
    }

    std::cout << "\n========================================\n";
    std::cout << "All examples completed successfully!\n";
    std::cout << "========================================\n";

    return 0;
}
