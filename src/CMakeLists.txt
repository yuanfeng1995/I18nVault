add_subdirectory(crypto)

add_library(I18nVaultCore STATIC i18n_manager.cpp)
target_include_directories(I18nVaultCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/generated>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/nlohmann>
        $<INSTALL_INTERFACE:include/I18nVault>
)
target_link_libraries(I18nVaultCore PRIVATE I18nVaultCrypto)
add_dependencies(I18nVaultCore i18n_keys i18n_diff_check i18n_trs)
