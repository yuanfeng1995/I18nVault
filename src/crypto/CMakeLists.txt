add_library(I18nVaultCrypto STATIC sm4.c gcm.c sm4_gcm.c hex_utils.c)
target_include_directories(I18nVaultCrypto
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
