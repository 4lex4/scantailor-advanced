set(CMAKE_AUTOUIC ON)

set(ui_files
    ApplyDialog.ui
    OptionsWidget.ui)

set(sources
    ImageView.cpp ImageView.h
    Thumbnail.cpp Thumbnail.h
    Filter.cpp Filter.h
    OptionsWidget.cpp OptionsWidget.h
    Settings.cpp Settings.h
    Task.cpp Task.h
    CacheDrivenTask.cpp CacheDrivenTask.h
    Dependencies.cpp Dependencies.h
    Params.cpp Params.h
    ApplyDialog.cpp ApplyDialog.h)

add_library(deskew STATIC ${sources} ${ui_files})
target_link_libraries(deskew PUBLIC stcore)
target_include_directories(deskew PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

translation_sources(scantailor ${sources} ${ui_files})