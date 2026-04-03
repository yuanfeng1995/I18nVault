add_executable(i18n_crypto_cli encrypt_i18n.c)
target_link_libraries(i18n_crypto_cli PRIVATE I18nVaultCrypto)

# ---- Build-time TRS generation ----
set(I18N_TRS_KEY_HEX "00112233445566778899AABBCCDDEEFF" CACHE STRING "SM4-GCM key hex for build-time TRS generation")
set(I18N_TRS_AAD "i18n:v1" CACHE STRING "AAD for build-time TRS generation")
string(LENGTH "${I18N_TRS_KEY_HEX}" _key_len)
if(NOT _key_len EQUAL 32)
    message(FATAL_ERROR "I18N_TRS_KEY_HEX must be 32 hex chars")
endif()

add_custom_target(i18n_trs ALL
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/gen_trs_files.py
            --cli $<TARGET_FILE:i18n_crypto_cli>
            --i18n-dir ${PROJECT_SOURCE_DIR}/i18n
            --key ${I18N_TRS_KEY_HEX}
            --aad ${I18N_TRS_AAD}
    DEPENDS i18n_crypto_cli
            ${PROJECT_SOURCE_DIR}/tools/gen_trs_files.py
)

# ---- Build-time i18n key diff check ----
set(I18N_REFERENCE_FILE ${PROJECT_SOURCE_DIR}/i18n/en_US.json)

add_custom_target(i18n_diff_check ALL
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/i18n_diff_check.py
            ${I18N_REFERENCE_FILE}
    DEPENDS ${I18N_REFERENCE_FILE}
            ${PROJECT_SOURCE_DIR}/tools/i18n_diff_check.py
)
