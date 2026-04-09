add_subdirectory(crypto)

add_library(I18nVaultCore STATIC
    i18n_manager.cpp
    $<TARGET_OBJECTS:I18nVaultCrypto>
)
target_include_directories(I18nVaultCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated>
    PRIVATE
        $<TARGET_PROPERTY:I18nVaultCrypto,INTERFACE_INCLUDE_DIRECTORIES>
)
add_dependencies(I18nVaultCore i18n_keys i18n_diff_check i18n_trs)
