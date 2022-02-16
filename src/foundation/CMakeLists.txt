set(sources
    Constants.h Constants.cpp
    NonCopyable.h
    AlignedArray.h
    FastQueue.h
    SafeDeletingQObjectPtr.h
    ScopedIncDec.h
    Span.h VirtualFunction.h FlagOps.h
    AutoRemovingFile.cpp AutoRemovingFile.h
    Proximity.cpp Proximity.h
    Property.h
    PropertyFactory.cpp PropertyFactory.h
    PropertySet.cpp PropertySet.h
    PerformanceTimer.cpp PerformanceTimer.h
    GridLineTraverser.cpp GridLineTraverser.h
    LineIntersectionScalar.cpp LineIntersectionScalar.h
    XmlMarshaller.cpp XmlMarshaller.h
    XmlUnmarshaller.cpp XmlUnmarshaller.h
    StaticPool.h
    DynamicPool.h
    NumericTraits.h
    TaskStatus.h
    VecNT.h
    VecT.h
    MatMNT.h
    MatT.h
    PriorityQueue.h
    Grid.h
    ValueConv.h
    Hashes.h
    Utils.cpp Utils.h)

add_library(foundation STATIC ${sources})
target_link_libraries(foundation PUBLIC Qt::Core Qt::Xml Qt::Gui)
target_include_directories(foundation PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
