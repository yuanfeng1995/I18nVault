
cmake_minimum_required(VERSION 3.15)
project(I18nVault)
set(CMAKE_CXX_STANDARD 17)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
include(CTest)

# 下载 nlohmann/json 单文件 header
set(NLOHMANN_DIR "${CMAKE_SOURCE_DIR}/src")
if(NOT EXISTS "${NLOHMANN_DIR}/nlohmann/json.hpp")
    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/download_nlohmann.py
                ${NLOHMANN_DIR}
        RESULT_VARIABLE _DL_RESULT
    )
    if(NOT _DL_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to download nlohmann/json")
    endif()
endif()

# 加密静态库
add_library(crypto STATIC src/crypto/sm4.c src/crypto/gcm.c src/crypto/sm4_gcm.c src/crypto/hex_utils.c)
target_include_directories(crypto PUBLIC src/crypto)

add_library(I18nVaultCore STATIC src/i18n_manager.cpp)
target_include_directories(I18nVaultCore PUBLIC src generated ${NLOHMANN_DIR})
target_link_libraries(I18nVaultCore PUBLIC crypto)

add_executable(i18n_test test/main.cpp)
target_include_directories(i18n_test PRIVATE src generated ${NLOHMANN_DIR})
target_link_libraries(i18n_test PRIVATE I18nVaultCore)

add_test(NAME i18n_manager_smoke COMMAND $<TARGET_FILE:i18n_test>)
set_tests_properties(i18n_manager_smoke PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(i18n_crypto_cli tools/encrypt_i18n.c)
target_link_libraries(i18n_crypto_cli PRIVATE crypto)

# 构建期生成 TRS：输出到 JSON 同级目录
set(I18N_TRS_KEY_HEX "00112233445566778899AABBCCDDEEFF" CACHE STRING "SM4-GCM key hex for build-time TRS generation")
set(I18N_TRS_AAD "i18n:v1" CACHE STRING "AAD for build-time TRS generation")
string(LENGTH "${I18N_TRS_KEY_HEX}" I18N_TRS_KEY_LEN)
if(NOT I18N_TRS_KEY_LEN EQUAL 32)
    message(FATAL_ERROR "I18N_TRS_KEY_HEX must be 32 hex chars")
endif()

add_custom_target(i18n_trs ALL
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/gen_trs_files.py
            --cli $<TARGET_FILE:i18n_crypto_cli>
            --i18n-dir ${CMAKE_SOURCE_DIR}/i18n
            --key ${I18N_TRS_KEY_HEX}
            --aad ${I18N_TRS_AAD}
    DEPENDS i18n_crypto_cli
            ${CMAKE_SOURCE_DIR}/tools/gen_trs_files.py
)

# 构建期 i18n key diff 检查：确保各语言 key 集合一致
set(I18N_REFERENCE_FILE ${CMAKE_SOURCE_DIR}/i18n/en_US.json)

add_custom_target(i18n_diff_check ALL
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/i18n_diff_check.py
        ${I18N_REFERENCE_FILE}
    DEPENDS ${I18N_REFERENCE_FILE}
        ${CMAKE_SOURCE_DIR}/tools/i18n_diff_check.py
)

# 自动生成 header
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/generated/i18n_keys.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/gen_i18n_keys.py
            ${CMAKE_SOURCE_DIR}/i18n/en_US.json
            ${CMAKE_SOURCE_DIR}/generated/i18n_keys.h
    DEPENDS ${CMAKE_SOURCE_DIR}/i18n/en_US.json
            ${CMAKE_SOURCE_DIR}/tools/gen_i18n_keys.py
)
add_custom_target(i18n_keys ALL DEPENDS ${CMAKE_SOURCE_DIR}/generated/i18n_keys.h)
add_dependencies(I18nVaultCore i18n_keys)
add_dependencies(I18nVaultCore i18n_diff_check)
add_dependencies(I18nVaultCore i18n_trs)
