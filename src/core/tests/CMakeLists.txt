set(sources
    main.cpp
    TestContentSpanFinder.cpp
    TestSmartFilenameOrdering.cpp)

add_executable(core_tests ${sources})
target_link_libraries(
    core_tests
    PRIVATE core Boost::unit_test_framework
    Boost::prg_exec_monitor ${EXTRA_LIBS})

add_test(NAME core_tests COMMAND core_tests --log_level=message)