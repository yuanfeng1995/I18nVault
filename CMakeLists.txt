
cmake_minimum_required(VERSION 3.15)
project(I18nVault VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# Enable conformant preprocessor on MSVC for __VA_OPT__ support
if(MSVC)
    string(APPEND CMAKE_C_FLAGS   " /Zc:preprocessor")
    string(APPEND CMAKE_CXX_FLAGS " /Zc:preprocessor")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
include(CTest)
include(GNUInstallDirs)

option(BUILD_SHARED_LIBS "Build I18nVaultCore as a shared library" OFF)

# Put all binaries (exe + dll) in the same directory so tests find shared libs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---- Auto-generate i18n_keys.h (with compile-time i18n support) ----
# Uses gen_i18n_keys_constexpr.py for strong-typed keys and compile-time translation
file(GLOB I18N_JSON_FILES "${CMAKE_SOURCE_DIR}/i18n/*.json")
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/generated/i18n_keys.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/gen_i18n_keys_constexpr.py
    DEPENDS ${I18N_JSON_FILES}
            ${CMAKE_SOURCE_DIR}/tools/gen_i18n_keys_constexpr.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating i18n_keys.h with compile-time i18n support..."
    VERBATIM
)
add_custom_target(i18n_keys ALL DEPENDS ${CMAKE_BINARY_DIR}/generated/i18n_keys.h)

message(STATUS "i18n_keys.h generation: using gen_i18n_keys_constexpr.py (compile-time i18n)")

# ---- Subdirectories ----
add_subdirectory(tools)
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(examples)

# ---- Install rules ----
# Public headers
install(FILES src/i18n_manager.h          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/I18nVault)
install(FILES ${CMAKE_BINARY_DIR}/exports/i18n_vault_export.h
                                          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/I18nVault)

# Key generation script (consumers generate i18n_keys.h from their own JSON)
install(FILES tools/gen_i18n_keys.py      DESTINATION ${CMAKE_INSTALL_DATADIR}/I18nVault/tools)

# Libraries
install(TARGETS I18nVaultCore
    EXPORT  I18nVaultTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# CLI tool
install(TARGETS i18n_crypto_cli
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# CMake package config for find_package(I18nVault)
install(EXPORT I18nVaultTargets
    FILE        I18nVaultTargets.cmake
    NAMESPACE   I18nVault::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/I18nVault
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/I18nVaultConfig.cmake.in
    ${CMAKE_BINARY_DIR}/I18nVaultConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/I18nVault
)
write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/I18nVaultConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_BINARY_DIR}/I18nVaultConfig.cmake
    ${CMAKE_BINARY_DIR}/I18nVaultConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/I18nVault
)

# ---- CPack ----
set(CPACK_PACKAGE_NAME "I18nVault")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "I18nVault")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Internationalization library with SM4-GCM encrypted TRS support")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

# Generator defaults per platform
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "TGZ")
else()
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "I18nVault maintainers")
endif()

include(CPack)
