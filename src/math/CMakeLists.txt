set(generic_sources
    LinearSolver.cpp LinearSolver.h
    MatrixCalc.h
    HomographicTransform.h
    SidesOfLine.cpp SidesOfLine.h
    ToLineProjector.cpp ToLineProjector.h
    ArcLengthMapper.cpp ArcLengthMapper.h
    LineBoundedByRect.cpp LineBoundedByRect.h
    PolylineIntersector.cpp PolylineIntersector.h
    LinearFunction.cpp LinearFunction.h
    QuadraticFunction.cpp QuadraticFunction.h
    XSpline.cpp XSpline.h)

set(spfit_sources
    spfit/references.txt
    spfit/FittableSpline.h
    spfit/FrenetFrame.cpp spfit/FrenetFrame.h
    spfit/ConstraintSet.cpp spfit/ConstraintSet.h
    spfit/SqDistApproximant.cpp spfit/SqDistApproximant.h
    spfit/ModelShape.h
    spfit/PolylineModelShape.cpp spfit/PolylineModelShape.h
    spfit/LinearForceBalancer.cpp spfit/LinearForceBalancer.h
    spfit/OptimizationResult.cpp spfit/OptimizationResult.h
    spfit/Optimizer.cpp spfit/Optimizer.h
    spfit/SplineFitter.cpp spfit/SplineFitter.h)

set(adiff_sources
    adiff/references.txt
    adiff/SparseMap.cpp adiff/SparseMap.h
    adiff/Function.cpp adiff/Function.h)

add_library(math STATIC ${generic_sources} ${spfit_sources} ${adiff_sources})
target_link_libraries(math PUBLIC foundation)
target_include_directories(math PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(tests)