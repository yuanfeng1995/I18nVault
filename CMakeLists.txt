
cmake_minimum_required(VERSION 3.15)
project(I18nVault VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
include(CTest)
include(GNUInstallDirs)

# ---- Download nlohmann/json (configure-time) ----
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

# ---- Auto-generate i18n_keys.h ----
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/generated/i18n_keys.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/gen_i18n_keys.py
            ${CMAKE_SOURCE_DIR}/i18n/en_US.json
            ${CMAKE_SOURCE_DIR}/generated/i18n_keys.h
    DEPENDS ${CMAKE_SOURCE_DIR}/i18n/en_US.json
            ${CMAKE_SOURCE_DIR}/tools/gen_i18n_keys.py
)
add_custom_target(i18n_keys ALL DEPENDS ${CMAKE_SOURCE_DIR}/generated/i18n_keys.h)

# ---- Subdirectories ----
add_subdirectory(tools)
add_subdirectory(src)
add_subdirectory(test)

# ---- Install rules ----
# Public headers
install(FILES src/i18n_manager.h          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/I18nVault)
install(FILES generated/i18n_keys.h       DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/I18nVault)

# Libraries
install(TARGETS I18nVaultCore I18nVaultCrypto
    EXPORT  I18nVaultTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
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
