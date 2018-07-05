include_directories(BEFORE ..)

set(
    sources
    main.cpp TestContentSpanFinder.cpp
    TestSmartFilenameOrdering.cpp
    TestMatrixCalc.cpp
    ../ContentSpanFinder.cpp ../ContentSpanFinder.h
    ../SmartFilenameOrdering.cpp ../SmartFilenameOrdering.h
)

source_group("Sources" FILES ${sources})

set(
    libs
    imageproc math Qt5::Widgets ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_PRG_EXECUTION_MONITOR_LIBRARY} ${EXTRA_LIBS}
)

add_executable(generic_tests ${sources})
target_link_libraries(generic_tests ${libs})

# We want the executable located where we copy all the DLLs.
set_target_properties(
    generic_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

add_test(NAME generic_tests COMMAND generic_tests --log_level=message)


