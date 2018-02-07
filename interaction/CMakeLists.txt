PROJECT(interaction)

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")

SET(
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

SOURCE_GROUP(Sources FILES ${sources})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

ADD_LIBRARY(interaction STATIC ${sources})

TRANSLATION_SOURCES(scantailor ${sources})