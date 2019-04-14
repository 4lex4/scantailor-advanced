set(sources
    Curve.cpp Curve.h
    DistortionModel.cpp DistortionModel.h
    DistortionModelBuilder.cpp DistortionModelBuilder.h
    DetectVertContentBounds.cpp DetectVertContentBounds.h
    TowardsLineTracer.cpp TowardsLineTracer.h
    TextLineTracer.cpp TextLineTracer.h
    TextLineRefiner.cpp TextLineRefiner.h
    TopBottomEdgeTracer.cpp TopBottomEdgeTracer.h
    CylindricalSurfaceDewarper.cpp CylindricalSurfaceDewarper.h
    DewarpingPointMapper.cpp DewarpingPointMapper.h
    RasterDewarper.cpp RasterDewarper.h)

add_library(dewarping STATIC ${sources})
target_link_libraries(dewarping PUBLIC imageproc)
target_include_directories(dewarping PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")