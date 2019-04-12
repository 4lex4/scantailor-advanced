set(sources
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

add_library(interaction STATIC ${sources})
target_link_libraries(interaction PUBLIC stcore)
target_include_directories(
    interaction
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

translation_sources(scantailor ${sources})