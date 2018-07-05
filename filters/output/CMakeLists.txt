project("Output Filter")

include_directories(BEFORE "${CMAKE_CURRENT_BINARY_DIR}")

file(GLOB ui_files "ui/*.ui")
qt5_wrap_ui(ui_sources ${ui_files})
set_source_files_properties(${ui_sources} PROPERTIES GENERATED TRUE)
source_group("UI Files" FILES ${ui_files})
source_group("Generated" FILES ${ui_sources})

set(
    sources
    ApplyColorsDialog.cpp ApplyColorsDialog.h
    ChangeDpiDialog.cpp ChangeDpiDialog.h
    ImageView.cpp ImageView.h
    ImageViewTab.h
    TabbedImageView.cpp TabbedImageView.h
    Filter.cpp Filter.h
    OptionsWidget.cpp OptionsWidget.h
    Task.cpp Task.h
    CacheDrivenTask.cpp CacheDrivenTask.h
    OutputGenerator.cpp OutputGenerator.h
    OutputMargins.h
    Settings.cpp Settings.h
    Thumbnail.cpp Thumbnail.h
    Utils.cpp Utils.h
    Params.cpp Params.h
    BlackWhiteOptions.cpp BlackWhiteOptions.h
    ColorCommonOptions.cpp ColorCommonOptions.h
    RenderParams.cpp RenderParams.h
    ColorParams.cpp ColorParams.h
    OutputImageParams.cpp OutputImageParams.h
    OutputFileParams.cpp OutputFileParams.h
    OutputParams.cpp OutputParams.h
    PictureLayerProperty.cpp PictureLayerProperty.h
    ZoneCategoryProperty.cpp ZoneCategoryProperty.h
    PictureZonePropFactory.cpp PictureZonePropFactory.h
    PictureZonePropDialog.cpp PictureZonePropDialog.h
    PictureZoneComparator.cpp PictureZoneComparator.h
    PictureZoneEditor.cpp PictureZoneEditor.h
    FillColorProperty.cpp FillColorProperty.h
    FillZonePropFactory.cpp FillZonePropFactory.h
    FillZoneComparator.cpp FillZoneComparator.h
    FillZoneEditor.cpp FillZoneEditor.h
    ColorPickupInteraction.cpp ColorPickupInteraction.h
    DespeckleState.cpp DespeckleState.h
    DespeckleView.cpp DespeckleView.h
    DespeckleVisualization.cpp DespeckleVisualization.h
    DespeckleLevel.cpp DespeckleLevel.h
    DewarpingView.cpp DewarpingView.h
    DewarpingOptions.cpp DewarpingOptions.h
    ChangeDewarpingDialog.cpp ChangeDewarpingDialog.h
    DepthPerception.cpp DepthPerception.h
    SplitImage.cpp SplitImage.h
    SplittingOptions.cpp SplittingOptions.h
    OtsuBinarizationOptionsWidget.cpp OtsuBinarizationOptionsWidget.h
    SauvolaBinarizationOptionsWidget.cpp SauvolaBinarizationOptionsWidget.h 
    WolfBinarizationOptionsWidget.cpp WolfBinarizationOptionsWidget.h 
    BinarizationOptionsWidget.h 
    PictureShapeOptions.cpp PictureShapeOptions.h 
    OutputProcessingParams.cpp OutputProcessingParams.h)

source_group("Sources" FILES ${sources})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_library(output STATIC ${sources} ${ui_sources})
add_dependencies(output toplevel_ui_sources)

translation_sources(scantailor ${sources} ${ui_files})