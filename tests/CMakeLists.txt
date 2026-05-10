# ─── Test Build Rules ────────────────────────────────────────

# Helper function to add a test executable
function(add_collect_test TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})

    target_link_libraries(${TEST_NAME}
        PRIVATE
            collect_core
            GTest::gtest_main
            GTest::gmock
    )

    target_include_directories(${TEST_NAME}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src
    )

    include(GoogleTest)
    gtest_discover_tests(${TEST_NAME})
endfunction()

# ─── Unit Tests ──────────────────────────────────────────────
add_collect_test(test_value_list         unit/test_value_list.cpp)
add_collect_test(test_value              unit/test_value.cpp)
add_collect_test(test_cdtime             unit/test_cdtime.cpp)
add_collect_test(test_plugin_manager     unit/test_plugin_manager.cpp)
add_collect_test(test_dispatcher         unit/test_dispatcher.cpp)
add_collect_test(test_write_queue        unit/test_write_queue.cpp)
add_collect_test(test_json_formatter     unit/test_json_formatter.cpp)
add_collect_test(test_csv_formatter      unit/test_csv_formatter.cpp)
add_collect_test(test_app_config_manager unit/test_app_config_manager.cpp)
add_collect_test(test_logger             unit/test_logger.cpp)
add_collect_test(test_string_utils       unit/test_string_utils.cpp)
add_collect_test(test_config_parser      unit/test_config_parser.cpp)
add_collect_test(test_types_db           unit/test_types_db.cpp)
add_collect_test(test_snapshot_manager   unit/test_snapshot_manager.cpp)

# ─── Integration Tests ──────────────────────────────────────
add_collect_test(test_pipeline       integration/test_pipeline.cpp)
