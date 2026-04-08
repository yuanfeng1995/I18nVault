/// Compile-time i18n Example (Zero Runtime Cost)
/// ============================================
/// This example demonstrates the improved I18nVault system with:
/// ✅ Strong type safety (Key_... types prevent accidental key mixing)
/// ✅ Compile-time i18n (all translations embedded at compile time, zero runtime lookup)
/// ✅ Type checking at compile time (no missing keys, no typos)
/// ✅ Full backward compatibility with runtime i18n

#include "i18n_manager.h"
#include <iostream>
#include <string_view>

using namespace I18nVault;

// ============================================================================
// Example 1: Compile-time i18n with strong typing
// ============================================================================
void example_compiletime_i18n()
{
    std::cout << "=== Example 1: Compile-time i18n ===" << std::endl;
    
    // Use strong type to access compile-time translation
    // Result: std::string_view pointing to compile-time constant string
    // Zero runtime lookup cost!
    constexpr auto login_text = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
    std::cout << "Login button (en_US): " << login_text << std::endl;
    
    // Different language - same key type, different language type
    constexpr auto login_text_zh = i18n<Key_LOGIN_BUTTON, Lang_zh_CN>();
    std::cout << "Login button (zh_CN): " << login_text_zh << std::endl;
    
    // With template parameters, compiler fully optimizes this away
    // At runtime, this is just a string constant reference
    std::cout << std::endl;
}

// ============================================================================
// Example 2: Compile-time macro for convenience
// ============================================================================
void example_compiletime_macro()
{
    std::cout << "=== Example 2: Compile-time macro ===" << std::endl;
    
    // Shorter version using macro
    // All resolved at compile time
    constexpr auto menu_file = I18N_CT_SELECT(MENU_FILE, en_US);
    constexpr auto menu_file_zh = I18N_CT_SELECT(MENU_FILE, zh_CN);
    
    std::cout << "Menu File (en_US): " << menu_file << std::endl;
    std::cout << "Menu File (zh_CN): " << menu_file_zh << std::endl;
    std::cout << std::endl;
}

// ============================================================================
// Example 3: Using in constexpr context (compile-time string building)
// ============================================================================
constexpr std::string_view get_compile_time_menu()
{
    // This entire context runs at compile time!
    // All translations are embedded, no runtime lookup
    return i18n<Key_MENU_EDIT, Lang_en_US>();
}

void example_constexpr_context()
{
    std::cout << "=== Example 3: constexpr context ===" << std::endl;
    
    constexpr auto menu_edit = get_compile_time_menu();
    std::cout << "Menu Edit (compile-time): " << menu_edit << std::endl;
    std::cout << std::endl;
}

// ============================================================================
// Example 4: Format strings with parameters (compile-time + runtime)
// ============================================================================
void example_format_strings()
{
    std::cout << "=== Example 4: Format strings ===" << std::endl;
    
    // Get compile-time template string
    constexpr auto welcome_template = i18n<Key_WELCOME_FMT, Lang_en_US>();
    std::cout << "Template (en_US): " << welcome_template << std::endl;
    
    // Runtime substitution of parameters (traditional way, with runtime lookup)
    // This still uses the old manager for parameter substitution
    std::string welcome_with_name = 
        I18nManager::instance().translate(I18nKey::WELCOME_FMT, {"Alice"});
    std::cout << "With params (runtime): " << welcome_with_name << std::endl;
    
    // Chinese version
    constexpr auto welcome_template_zh = i18n<Key_WELCOME_FMT, Lang_zh_CN>();
    std::string welcome_with_name_zh = 
        I18nManager::instance().translate(I18nKey::WELCOME_FMT, {"小明"});
    std::cout << "With params (zh_CN): " << welcome_with_name_zh << std::endl;
    std::cout << std::endl;
}

// ============================================================================
// Example 5: Type safety - compiler prevents mistakes
// ============================================================================
void example_type_safety()
{
    std::cout << "=== Example 5: Type Safety ===" << std::endl;
    
    // ✅ This compiles - correct key type
    constexpr auto text1 = i18n<Key_MENU_FILE, Lang_en_US>();
    
    // ✅ This compiles - correct key type for different key
    constexpr auto text2 = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
    
    // ❌ This would NOT compile - if you tried it:
    // constexpr auto text3 = i18n<Key_WELCOME_FMT, int>();  // Wrong language type!
    // constexpr auto text4 = i18n<NonExistentKey, Lang_en_US>();  // Key doesn't exist!
    // ^ Compiler error at compile time = no runtime bugs!
    
    std::cout << "All type checks passed at compile time!" << std::endl;
    std::cout << std::endl;
}

// ============================================================================
// Example 6: Backward compatibility with runtime i18n
// ============================================================================
void example_runtime_compatibility()
{
    std::cout << "=== Example 6: Backward Compatibility ===" << std::endl;
    
    // Old API still works for dynamic scenarios where you need runtime selection
    std::string old_style = I18nVault_TR(I18nKey::LOGIN_BUTTON);
    std::cout << "Old style macro: " << old_style << std::endl;
    
    // Can also use I18nManager directly
    auto managers_method = 
        I18nManager::instance().translate(I18nKey::DIALOG_CONFIRM);
    std::cout << "Manager method: " << managers_method << std::endl;
    std::cout << std::endl;
}

// ============================================================================
// Example 7: Performance comparison
// ============================================================================
void example_performance_note()
{
    std::cout << "=== Example 7: Performance ===" << std::endl;
    
    // Compile-time approach:
    // constexpr auto text = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
    // Cost: ZERO at runtime. Just a constant reference to embedded string.
    
    // Runtime approach:
    // auto text = I18nManager::instance().translate(I18nKey::LOGIN_BUTTON);
    // Cost: Hash table lookup, string copy, potential memory allocations
    
    std::cout << "Use i18n<Key_..., Lang_...>() for performance-critical code" << std::endl;
    std::cout << "Use I18nManager for dynamic/runtime selection" << std::endl;
    std::cout << std::endl;
}

// ============================================================================
// Main
// ============================================================================
int main()
{
    std::cout << "I18nVault - Compile-time i18n with Strong Typing" << std::endl;
    std::cout << "================================================" << std::endl;
    std::cout << std::endl;
    
    example_compiletime_i18n();
    example_compiletime_macro();
    example_constexpr_context();
    example_format_strings();
    example_type_safety();
    example_runtime_compatibility();
    example_performance_note();
    
    std::cout << "All examples completed successfully!" << std::endl;
    return 0;
}
