include_directories(BEFORE ..)

set(
    sources
    main.cpp
    TestBinaryImage.cpp TestReduceThreshold.cpp
    TestSlicedHistogram.cpp
    TestConnCompEraser.cpp TestConnCompEraserExt.cpp
    TestGrayscale.cpp
    TestRasterOp.cpp TestShear.cpp
    TestOrthogonalRotation.cpp
    TestSkewFinder.cpp
    TestScale.cpp
    TestTransform.cpp
    TestMorphology.cpp
    TestBinarize.cpp
    TestPolygonRasterizer.cpp
    TestSeedFill.cpp
    TestSEDM.cpp
    TestRastLineFinder.cpp
    Utils.cpp Utils.h
)
source_group("Sources" FILES ${sources})

set(
    libs
    imageproc math foundation Qt5::Widgets Qt5::Xml
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_PRG_EXECUTION_MONITOR_LIBRARY} ${EXTRA_LIBS}
)

remove_definitions(-DBUILDING_IMAGEPROC)
add_executable(imageproc_tests ${sources})
target_link_libraries(imageproc_tests ${libs})

# We want the executable located where we copy all the DLLs.
set_target_properties(
    imageproc_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

add_test(NAME imageproc_tests COMMAND imageproc_tests --log_level=message)

