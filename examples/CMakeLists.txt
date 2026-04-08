option(I18NVAULT_BUILD_EXAMPLES "Build I18nVault examples" ON)
if(NOT I18NVAULT_BUILD_EXAMPLES)
    return()
endif()

add_executable(i18n_demo i18n_demo.cpp)
target_link_libraries(i18n_demo PRIVATE I18nVaultCore)

add_executable(i18n_compile_time_example compile_time_i18n_example.cpp)
target_link_libraries(i18n_compile_time_example PRIVATE I18nVaultCore)
