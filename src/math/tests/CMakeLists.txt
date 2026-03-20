set(sources
    main.cpp
    TestHessians.cpp
    TestLineBoundedByRect.cpp
    TestLinearFunction.cpp
    TestMatrixCalc.cpp
    TestQuadraticFunction.cpp
    TestSidesOfLine.cpp
    TestSqDistApproximant.cpp)

add_executable(math_tests ${sources})
target_link_libraries(
    math_tests 
    PRIVATE math Boost::unit_test_framework ${EXTRA_LIBS})

add_test(NAME math_tests COMMAND math_tests --log_level=message)