project("Page Split Filter")

include_directories(BEFORE "${CMAKE_CURRENT_BINARY_DIR}")

file(GLOB ui_files "ui/*.ui")
qt5_wrap_ui(ui_sources ${ui_files})
set_source_files_properties(${ui_sources} PROPERTIES GENERATED TRUE)
source_group("UI Files" FILES ${ui_files})
source_group("Generated" FILES ${ui_sources})

set(
    sources
    SplitLineObject.h
    ImageView.cpp ImageView.h
    Thumbnail.cpp Thumbnail.h
    Params.cpp Params.h
    Dependencies.cpp Dependencies.h
    PageLayout.cpp PageLayout.h
    PageLayoutEstimator.cpp PageLayoutEstimator.h
    VertLineFinder.cpp VertLineFinder.h
    Filter.cpp Filter.h
    OptionsWidget.cpp OptionsWidget.h
    SplitModeDialog.cpp SplitModeDialog.h
    Settings.cpp Settings.h
    Task.cpp Task.h
    CacheDrivenTask.cpp CacheDrivenTask.h
    LayoutType.cpp LayoutType.h
    UnremoveButton.cpp UnremoveButton.h
    OrderBySplitTypeProvider.cpp OrderBySplitTypeProvider.h
    PageLayoutAdapter.cpp PageLayoutAdapter.h)
    
source_group("Sources" FILES ${sources})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_library(page_split STATIC ${sources} ${ui_sources})

translation_sources(scantailor ${sources} ${ui_files})