
cmake_minimum_required(VERSION 3.15)
project(i18n_core_trs)
set(CMAKE_CXX_STANDARD 17)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

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
add_library(crypto STATIC src/crypto/sm4.c src/crypto/gcm.c src/crypto/sm4_gcm.c)
target_include_directories(crypto PUBLIC src/crypto)

add_executable(test test/main.cpp src/i18n_manager.cpp)
target_include_directories(test PRIVATE src generated ${NLOHMANN_DIR})
target_link_libraries(test PRIVATE crypto)

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
add_dependencies(test i18n_keys)
add_dependencies(test i18n_diff_check)
