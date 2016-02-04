PROJECT("Deskew Filter")

INCLUDE_DIRECTORIES(BEFORE "${CMAKE_CURRENT_BINARY_DIR}")

FILE(GLOB ui_files "ui/*.ui")
QT5_WRAP_UI(ui_sources ${ui_files})
SET_SOURCE_FILES_PROPERTIES(${ui_sources} PROPERTIES GENERATED TRUE)
SOURCE_GROUP("UI Files" FILES ${ui_files})
SOURCE_GROUP("Generated" FILES ${ui_sources})

SET(
        sources
        ImageView.cpp ImageView.h
        Thumbnail.cpp Thumbnail.h
        Filter.cpp Filter.h
        OptionsWidget.cpp OptionsWidget.h
        Settings.cpp Settings.h
        Task.cpp Task.h
        CacheDrivenTask.cpp CacheDrivenTask.h
        Dependencies.cpp Dependencies.h
        Params.cpp Params.h
        ApplyDialog.cpp ApplyDialog.h
)
SOURCE_GROUP("Sources" FILES ${sources})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

ADD_LIBRARY(deskew STATIC ${sources} ${ui_sources})

TRANSLATION_SOURCES(scantailor ${sources} ${ui_files})
