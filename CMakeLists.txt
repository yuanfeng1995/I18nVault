
cmake_minimum_required(VERSION 3.15)
project(I18nVault VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
include(CTest)

# Put all binaries (exe + dll) in the same directory so tests find shared libs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---- Auto-generate i18n_keys.h ----
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/generated/i18n_keys.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/gen_i18n_keys.py
            ${CMAKE_SOURCE_DIR}/i18n/en_US.json
            ${CMAKE_BINARY_DIR}/generated/i18n_keys.h
    DEPENDS ${CMAKE_SOURCE_DIR}/i18n/en_US.json
            ${CMAKE_SOURCE_DIR}/tools/gen_i18n_keys.py
)
add_custom_target(i18n_keys ALL DEPENDS ${CMAKE_BINARY_DIR}/generated/i18n_keys.h)

# ---- Subdirectories ----
add_subdirectory(tools)
add_subdirectory(src)
add_subdirectory(test)
