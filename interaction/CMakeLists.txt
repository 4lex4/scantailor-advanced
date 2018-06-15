project(interaction)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

set(
    sources
    DraggableObject.h
    DraggablePoint.cpp DraggablePoint.h
    DraggableLineSegment.cpp DraggableLineSegment.h
    ObjectDragHandler.cpp ObjectDragHandler.h
    InteractionHandler.cpp InteractionHandler.h
    InteractionState.cpp InteractionState.h
    DragHandler.cpp DragHandler.h
    DragWatcher.cpp DragWatcher.h
    ZoomHandler.cpp ZoomHandler.h
    InteractiveXSpline.cpp InteractiveXSpline.h
    DraggablePolygon.cpp DraggablePolygon.h)

source_group(Sources FILES ${sources})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_library(interaction STATIC ${sources})

translation_sources(scantailor ${sources})