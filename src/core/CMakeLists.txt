set(common_ui_files
    ErrorWidget.ui)

set(common_sources
    Application.cpp Application.h
    BackgroundExecutor.cpp BackgroundExecutor.h
    PixmapRenderer.cpp PixmapRenderer.h
    BubbleAnimation.cpp BubbleAnimation.h
    ProcessingIndicationWidget.cpp ProcessingIndicationWidget.h
    NonOwningWidget.cpp NonOwningWidget.h
    SmartFilenameOrdering.cpp SmartFilenameOrdering.h
    AbstractRelinker.h
    RelinkablePath.cpp RelinkablePath.h
    ImageInfo.cpp ImageInfo.h
    ImageFileInfo.cpp ImageFileInfo.h
    ImageMetadata.cpp ImageMetadata.h
    RecentProjects.cpp RecentProjects.h
    OutOfMemoryHandler.cpp OutOfMemoryHandler.h
    PageSelectionAccessor.cpp PageSelectionAccessor.h
    PageSelectionProvider.h
    ContentSpanFinder.cpp ContentSpanFinder.h
    ImageTransformation.cpp ImageTransformation.h
    ImagePixmapUnion.h
    ImageViewBase.cpp ImageViewBase.h
    BasicImageView.cpp BasicImageView.h
    StageListView.cpp StageListView.h
    DebugImageView.cpp DebugImageView.h
    TabbedDebugImages.cpp TabbedDebugImages.h
    ThumbnailLoadResult.h
    ThumbnailPixmapCache.cpp ThumbnailPixmapCache.h
    ThumbnailBase.cpp ThumbnailBase.h
    ThumbnailFactory.cpp ThumbnailFactory.h
    IncompleteThumbnail.cpp IncompleteThumbnail.h
    ContentBoxPropagator.cpp ContentBoxPropagator.h
    PageOrientationPropagator.cpp PageOrientationPropagator.h
    DebugImagesImpl.cpp DebugImagesImpl.h
    ImageId.cpp ImageId.h
    PageId.cpp PageId.h
    PageInfo.cpp PageInfo.h
    BackgroundTask.cpp BackgroundTask.h
    ProcessingTaskQueue.cpp ProcessingTaskQueue.h
    PageSequence.cpp PageSequence.h
    StageSequence.cpp StageSequence.h
    ProjectPages.cpp ProjectPages.h
    FilterData.cpp FilterData.h
    ImageMetadataLoader.cpp ImageMetadataLoader.h
    TiffReader.cpp TiffReader.h
    TiffWriter.cpp TiffWriter.h
    PngMetadataLoader.cpp PngMetadataLoader.h
    TiffMetadataLoader.cpp TiffMetadataLoader.h
    JpegMetadataLoader.cpp JpegMetadataLoader.h
    ImageLoader.cpp ImageLoader.h
    ErrorWidget.cpp ErrorWidget.h
    OrthogonalRotation.cpp OrthogonalRotation.h
    WorkerThreadPool.cpp WorkerThreadPool.h
    LoadFileTask.cpp LoadFileTask.h
    FilterOptionsWidget.cpp FilterOptionsWidget.h
    FilterUiInterface.h
    ProjectReader.cpp ProjectReader.h
    ProjectWriter.cpp ProjectWriter.h
    AtomicFileOverwriter.cpp AtomicFileOverwriter.h
    EstimateBackground.cpp EstimateBackground.h
    Despeckle.cpp Despeckle.h
    FileNameDisambiguator.cpp FileNameDisambiguator.h
    OpenGLSupport.cpp OpenGLSupport.h
    OutputFileNameGenerator.cpp OutputFileNameGenerator.h
    Margins.cpp Margins.h
    ColorScheme.h
    DarkColorScheme.cpp DarkColorScheme.h
    LightColorScheme.cpp LightColorScheme.h
    NativeColorScheme.cpp NativeColorScheme.h
    ColorSchemeManager.cpp ColorSchemeManager.h
    PageRange.cpp PageRange.h
    SelectedPage.cpp SelectedPage.h
    Utils.cpp Utils.h
    PageView.h
    AutoManualMode.cpp AutoManualMode.h
    AbstractCommand.h
    AbstractFilter.h
    BeforeOrAfter.h
    FilterResult.h
    CompositeCacheDrivenTask.h
    ChangedStateItemDelegate.h
    PageOrderProvider.h PageOrderProvider.cpp
    PageOrderOption.h
    PayloadEvent.h
    AbstractFilterDataCollector.h
    ThumbnailCollector.h
    ContentBoxCollector.h
    PageOrientationCollector.h
    ImageViewInfoProvider.cpp ImageViewInfoProvider.h
    ImageViewInfoListener.h
    UnitsProvider.cpp UnitsProvider.h
    UnitsListener.h UnitsListener.cpp
    UnitsConverter.cpp UnitsConverter.h
    Units.cpp Units.h
    DefaultParams.cpp DefaultParams.h
    DefaultParamsProfileManager.cpp DefaultParamsProfileManager.h
    DefaultParamsProvider.cpp DefaultParamsProvider.h
    DeviationProvider.h
    OrderByDeviationProvider.cpp OrderByDeviationProvider.h
    BlackOnWhiteEstimator.cpp BlackOnWhiteEstimator.h
    ImageSettings.cpp ImageSettings.h
    NullTaskStatus.h
    OrderByCompletenessProvider.cpp OrderByCompletenessProvider.h
    SkinnedButton.cpp SkinnedButton.h
    CollapsibleGroupBox.cpp CollapsibleGroupBox.h
    ColorSchemeFactory.cpp ColorSchemeFactory.h
    ApplicationSettings.cpp ApplicationSettings.h
    IconProvider.cpp IconProvider.h
    IconPack.h
    AbstractIconPack.cpp AbstractIconPack.h
    StyledIconPack.cpp StyledIconPack.h
    FontIconPack.cpp FontIconPack.h
    ConnectionManager.cpp ConnectionManager.h)

add_library(core STATIC ${common_sources} ${common_ui_files})
target_link_libraries(
    core
    PRIVATE TIFF::TIFF PNG::PNG ZLIB::ZLIB JPEG::JPEG
    PUBLIC Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Xml Qt5::Network Qt5::OpenGL Qt5::Svg
    imageproc
    fix_orientation page_split deskew select_content page_layout output)
target_include_directories(core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(core PROPERTIES AUTOUIC ON)

translation_sources(scantailor ${common_sources} ${common_ui_files})

add_subdirectory(interaction)
add_subdirectory(zones)
add_subdirectory(tests)
add_subdirectory(filters/fix_orientation)
add_subdirectory(filters/page_split)
add_subdirectory(filters/deskew)
add_subdirectory(filters/select_content)
add_subdirectory(filters/page_layout)
add_subdirectory(filters/output)