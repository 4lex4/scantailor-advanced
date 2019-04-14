set(sources
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

add_library(zones STATIC ${sources})
target_link_libraries(zones PUBLIC stcore)
target_include_directories(
    zones
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

translation_sources(scantailor ${sources})