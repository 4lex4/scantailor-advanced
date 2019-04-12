set(sources
    Constants.h Constants.cpp
    BinaryImage.cpp BinaryImage.h
    BinaryThreshold.cpp BinaryThreshold.h
    SlicedHistogram.cpp SlicedHistogram.h
    ByteOrder.h BWColor.h
    ConnComp.h Connectivity.h
    BitOps.cpp BitOps.h
    SeedFill.cpp SeedFill.h
    ConnCompEraser.cpp ConnCompEraser.h
    ConnCompEraserExt.cpp ConnCompEraserExt.h
    GrayImage.cpp GrayImage.h
    Grayscale.cpp Grayscale.h
    RasterOp.h GrayRasterOp.h RasterOpGeneric.h
    UpscaleIntegerTimes.cpp UpscaleIntegerTimes.h
    ReduceThreshold.cpp ReduceThreshold.h
    Shear.cpp Shear.h
    SkewFinder.cpp SkewFinder.h
    OrthogonalRotation.cpp OrthogonalRotation.h
    Scale.cpp Scale.h
    Transform.cpp Transform.h
    Morphology.cpp Morphology.h
    IntegralImage.h
    Binarize.cpp Binarize.h
    PolygonUtils.cpp PolygonUtils.h
    PolygonRasterizer.cpp PolygonRasterizer.h
    HoughLineDetector.cpp HoughLineDetector.h
    GaussBlur.cpp GaussBlur.h
    Sobel.h
    MorphGradientDetect.cpp MorphGradientDetect.h
    PolynomialLine.cpp PolynomialLine.h
    PolynomialSurface.cpp PolynomialSurface.h
    SavGolKernel.cpp SavGolKernel.h
    SavGolFilter.cpp SavGolFilter.h
    DrawOver.cpp DrawOver.h
    AdjustBrightness.cpp AdjustBrightness.h
    SEDM.cpp SEDM.h
    ConnectivityMap.cpp ConnectivityMap.h
    InfluenceMap.cpp InfluenceMap.h
    MaxWhitespaceFinder.cpp MaxWhitespaceFinder.h
    RastLineFinder.cpp RastLineFinder.h
    ColorInterpolation.cpp ColorInterpolation.h
    LocalMinMaxGeneric.h
    SeedFillGeneric.cpp SeedFillGeneric.h
    FindPeaksGeneric.h
    ColorMixer.h
    ColorForId.h
    BackgroundColorCalculator.cpp BackgroundColorCalculator.h
    BadAllocIfNull.cpp BadAllocIfNull.h
    ColorSegmenter.cpp ColorSegmenter.h
    ColorTable.cpp ColorTable.h
    ImageCombination.h ImageCombination.cpp
    Dpi.cpp Dpi.h
    Dpm.cpp Dpm.h
    DebugImages.h)

source_group(Sources FILES ${sources})

add_library(imageproc STATIC ${sources})
target_link_libraries(imageproc PUBLIC foundation math)
target_include_directories(imageproc PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(tests)
