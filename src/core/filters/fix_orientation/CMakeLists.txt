set(CMAKE_AUTOUIC ON)

set(ui_files
    ApplyDialog.ui
    OptionsWidget.ui)

set(sources
    ImageView.cpp ImageView.h
    Filter.cpp Filter.h
    OptionsWidget.cpp OptionsWidget.h
    ApplyDialog.cpp ApplyDialog.h
    Settings.cpp Settings.h
    Task.cpp Task.h
    CacheDrivenTask.cpp CacheDrivenTask.h)

add_library(fix_orientation STATIC ${sources} ${ui_files})
target_link_libraries(fix_orientation PUBLIC core)

translation_sources(scantailor ${sources} ${ui_files})