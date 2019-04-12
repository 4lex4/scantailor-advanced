file(GLOB ui_files "ui/*.ui")
qt5_wrap_ui(ui_sources ${ui_files})
set_source_files_properties(${ui_sources} PROPERTIES GENERATED TRUE)
source_group("UI Files" FILES ${ui_files})
source_group("Generated" FILES ${ui_sources})

set(sources
    Utils.cpp Utils.h
    ImageView.cpp ImageView.h
    Filter.cpp Filter.h
    OptionsWidget.cpp OptionsWidget.h
    Task.cpp Task.h
    CacheDrivenTask.cpp CacheDrivenTask.h
    Params.cpp Params.h
    Settings.cpp Settings.h
    Thumbnail.cpp Thumbnail.h
    ApplyDialog.cpp ApplyDialog.h
    Alignment.cpp Alignment.h
    OrderByWidthProvider.cpp OrderByWidthProvider.h
    OrderByHeightProvider.cpp OrderByHeightProvider.h
    Guide.cpp Guide.h)

source_group("Sources" FILES ${sources})

add_library(page_layout STATIC ${sources} ${ui_sources})
target_link_libraries(page_layout PUBLIC stcore interaction)
target_include_directories(page_layout PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

translation_sources(scantailor ${sources} ${ui_files})