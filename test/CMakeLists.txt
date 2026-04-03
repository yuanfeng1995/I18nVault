add_executable(i18n_test main.cpp)
target_link_libraries(i18n_test PRIVATE I18nVaultCore I18nVaultCrypto)

add_test(NAME i18n_manager_smoke COMMAND $<TARGET_FILE:i18n_test>)
set_tests_properties(i18n_manager_smoke PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
