set(sources
    main.cpp
    TestHessians.cpp
    TestSqDistApproximant.cpp
    TestMatrixCalc.cpp)

add_executable(math_tests ${sources})
target_link_libraries(
    math_tests 
    PRIVATE math Boost::unit_test_framework
    Boost::prg_exec_monitor ${EXTRA_LIBS})

add_test(NAME math_tests COMMAND math_tests --log_level=message)