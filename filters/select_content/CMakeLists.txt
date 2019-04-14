file(GLOB ui_files "ui/*.ui")
qt5_wrap_ui(ui_sources ${ui_files})
set_source_files_properties(${ui_sources} PROPERTIES GENERATED TRUE)

set(sources
    ImageView.cpp ImageView.h
    Filter.cpp Filter.h
    OptionsWidget.cpp OptionsWidget.h
    ApplyDialog.cpp ApplyDialog.h
    ContentBoxFinder.cpp ContentBoxFinder.h
    PageFinder.cpp PageFinder.h
    Task.cpp Task.h
    CacheDrivenTask.cpp CacheDrivenTask.h
    Dependencies.cpp Dependencies.h
    Params.cpp Params.h
    Settings.cpp Settings.h
    Thumbnail.cpp Thumbnail.h
    PhysSizeCalc.cpp PhysSizeCalc.h
    OrderByWidthProvider.cpp OrderByWidthProvider.h
    OrderByHeightProvider.cpp OrderByHeightProvider.h)

add_library(select_content STATIC ${sources} ${ui_sources})
target_link_libraries(select_content PUBLIC stcore)
target_include_directories(select_content PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

translation_sources(scantailor ${sources} ${ui_files})
