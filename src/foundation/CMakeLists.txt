set(sources
    Constants.h Constants.cpp
    NonCopyable.h intrusive_ptr.h ref_countable.h
    AlignedArray.h
    FastQueue.h
    SafeDeletingQObjectPtr.h
    ScopedIncDec.h ScopedDecInc.h
    Span.h VirtualFunction.h FlagOps.h
    AutoRemovingFile.cpp AutoRemovingFile.h
    Proximity.cpp Proximity.h
    Property.h
    PropertyFactory.cpp PropertyFactory.h
    PropertySet.cpp PropertySet.h
    PerformanceTimer.cpp PerformanceTimer.h
    QtSignalForwarder.cpp QtSignalForwarder.h
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
target_link_libraries(foundation PUBLIC Qt5::Core Qt5::Xml Qt5::Gui)
target_include_directories(foundation PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")