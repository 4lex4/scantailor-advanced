project(zones)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

set(
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

source_group(Sources FILES ${sources})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_library(zones STATIC ${sources})

translation_sources(scantailor ${sources})