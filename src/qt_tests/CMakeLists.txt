set(sources
    main.cpp
    tst_core_qt.cpp
    tst_ux_qt.cpp)

add_executable(qt_tests ${sources})
target_link_libraries(
    qt_tests
    PRIVATE core foundation ${Qt_Test_lib} ${Qt_Core_lib} ${Qt_Gui_lib} ${Qt_Widgets_lib} ${EXTRA_LIBS})
target_include_directories(qt_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/src")

add_test(NAME qt_tests COMMAND qt_tests)
