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

add_library(interaction STATIC ${sources})
target_link_libraries(interaction PUBLIC core)
target_include_directories(interaction PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

translation_sources(scantailor ${sources})