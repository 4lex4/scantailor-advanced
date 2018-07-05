project("Dewarping library")

set(
    sources
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
    RasterDewarper.cpp RasterDewarper.h
)
source_group("Sources" FILES ${sources})

add_library(dewarping STATIC ${sources})
