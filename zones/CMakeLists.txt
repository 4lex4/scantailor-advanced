PROJECT(zones)

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")

SET(
        sources
        SplineVertex.cpp SplineVertex.h
        SplineSegment.cpp SplineSegment.h
        EditableSpline.cpp EditableSpline.h
        SerializableSpline.cpp SerializableSpline.h
        Zone.cpp Zone.h
        ZoneSet.cpp ZoneSet.h
        EditableZoneSet.cpp EditableZoneSet.h
        BasicSplineVisualizer.cpp BasicSplineVisualizer.h
        ZoneInteractionContext.cpp ZoneInteractionContext.h
        ZoneDefaultInteraction.cpp ZoneDefaultInteraction.h
        ZoneCreationInteraction.cpp ZoneCreationInteraction.h
        ZoneVertexDragInteraction.cpp ZoneVertexDragInteraction.h
        ZoneContextMenuInteraction.cpp ZoneContextMenuInteraction.h
        ZoneContextMenuItem.h
        ZoneDragInteraction.cpp ZoneDragInteraction.h)

SOURCE_GROUP(Sources FILES ${sources})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

ADD_LIBRARY(zones STATIC ${sources})

TRANSLATION_SOURCES(scantailor ${sources})