project("Foundation library")

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

set(
    sources
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
    StaticPool.h
    DynamicPool.h
    NumericTraits.h
    VecNT.h
    VecT.h
    MatMNT.h
    MatT.h
    PriorityQueue.h
    Grid.h
    ValueConv.h
    Hashes.h)
source_group("Sources" FILES ${sources})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_library(foundation STATIC ${sources})
