set(CMAKE_AUTOUIC ON)

set(ui_files
    SplitModeDialog.ui
    OptionsWidget.ui)

set(sources
    SplitLineObject.h
    ImageView.cpp ImageView.h
    Thumbnail.cpp Thumbnail.h
    Params.cpp Params.h
    Dependencies.cpp Dependencies.h
    PageLayout.cpp PageLayout.h
    PageLayoutEstimator.cpp PageLayoutEstimator.h
    VertLineFinder.cpp VertLineFinder.h
    Filter.cpp Filter.h
    OptionsWidget.cpp OptionsWidget.h
    SplitModeDialog.cpp SplitModeDialog.h
    Settings.cpp Settings.h
    Task.cpp Task.h
    CacheDrivenTask.cpp CacheDrivenTask.h
    LayoutType.cpp LayoutType.h
    UnremoveButton.cpp UnremoveButton.h
    OrderBySplitTypeProvider.cpp OrderBySplitTypeProvider.h
    PageLayoutAdapter.cpp PageLayoutAdapter.h
    Utils.cpp Utils.h)

add_library(page_split STATIC ${sources} ${ui_files})
target_link_libraries(page_split PUBLIC core)

translation_sources(scantailor ${sources} ${ui_files})