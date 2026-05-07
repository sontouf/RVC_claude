add_executable(rvc_unit_tests
    unit/navigation_policy_test.cpp
    unit/cleaning_power_policy_test.cpp
    unit/cleaning_coordinator_test.cpp
    unit/grid_world_test.cpp
    unit/grid_world_loader_test.cpp
    unit/sensor_reading_test.cpp)
target_link_libraries(rvc_unit_tests PRIVATE rvc_core gtest_main gmock)
add_test(NAME rvc_unit_tests COMMAND rvc_unit_tests)
set_tests_properties(rvc_unit_tests PROPERTIES LABELS "unit")

add_executable(rvc_integration_tests
    integration/coordinator_grid_integration_test.cpp
    integration/scenario_smoke_test.cpp)
target_link_libraries(rvc_integration_tests PRIVATE rvc_core gtest_main gmock)
add_test(NAME rvc_integration_tests COMMAND rvc_integration_tests)
set_tests_properties(rvc_integration_tests PROPERTIES LABELS "integration")
