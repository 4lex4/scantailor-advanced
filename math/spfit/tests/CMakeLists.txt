include_directories(BEFORE ..)

set(
    sources
    ${CMAKE_SOURCE_DIR}/tests/main.cpp
    TestSqDistApproximant.cpp
)

source_group("Sources" FILES ${sources})

set(
    libs
    math Qt5::Core ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_PRG_EXECUTION_MONITOR_LIBRARY} ${EXTRA_LIBS}
)

add_executable(spfit_tests ${sources})
target_link_libraries(spfit_tests ${libs})

# We want the executable located where we copy all the DLLs.
set_target_properties(
    spfit_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

add_test(NAME spfit_tests COMMAND spfit_tests --log_level=message)


