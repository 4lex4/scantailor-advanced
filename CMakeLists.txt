cmake_minimum_required(VERSION 3.9.0)

project("ScanTailor Advanced")

# setting compiler flags
set(CMAKE_CXX_STANDARD 17)

if (MSVC)
  set(WIN_XP FALSE CACHE BOOLEAN "Whether to build for Windows XP.")

  if (WIN_XP)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D_USING_V110_SDK71_")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_USING_V110_SDK71_")
    set(CMAKE_EXE_LINKER_FLAGS "/SUBSYSTEM:CONSOLE,5.01 /SUBSYSTEM:WINDOWS,5.01 ${CMAKE_EXE_LINKER_FLAGS}")
  endif()

  # Disable checked iterators for extra performance.
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D_SECURE_SCL=0")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_SECURE_SCL=0")

  if (DEBUG_CLI)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /DDEBUG_CLI")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DDEBUG_CLI")
  endif()
endif()

if (UNIX)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

set(
    CMAKE_C_FLAGS "${CMAKE_C_FLAGS}"
    CACHE STRING "Common C flags for all build configurations." FORCE
)
set(
    CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}"
    CACHE STRING "Common C++ flags for all build configurations." FORCE
)
set(
    CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}"
    CACHE STRING "Common link flags for all build configurations." FORCE
)

enable_testing()

# An undocumented side-effect of configure_file() is that it makes
# the whole project depend on the file we are parsing / copying.
configure_file(
    "${PROJECT_SOURCE_DIR}/version.h"
    "${PROJECT_BINARY_DIR}/.version.h" COPYONLY
)

# Prevent this leftover from old builds to be used in favour
# of the one in ${PROJECT_SOURCE_DIR}
if (NOT "${PROJECT_BINARY_DIR}" STREQUAL "${PROJECT_SOURCE_DIR}")
  file(REMOVE "${PROJECT_BINARY_DIR}/version.h")
endif()

# Extract VERSION and VERSION_QUAD from version.h
file(READ "${PROJECT_SOURCE_DIR}/version.h" version_h_contents)
string(
    REGEX REPLACE
    ".*#define[ \\t]+VERSION[ \\t]+\"([^\"]*)\".*"
    "\\1" VERSION "${version_h_contents}"
)
if ("${VERSION}" STREQUAL "${version_h_contents}")
  message(FATAL_ERROR "Failed to extract VERSION from version.h")
endif()

# VERSION_QUAD must be either empty or be in the form of X.Y.Z.Y
string(
    REGEX REPLACE
    ".*#define[ \\t]+VERSION_QUAD[ \\t]+\"(([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)?)\".*"
    "\\1" VERSION_QUAD "${version_h_contents}"
)
if ("${VERSION_QUAD}" STREQUAL "${version_h_contents}")
  message(FATAL_ERROR "Failed to extract VERSION_QUAD from version.h")
endif()

# This has to go quite early on, as otherwise we risk picking
# up an identically named header from a system include path.
include_directories(. foundation math interaction zones)

# For config.h
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

include(cmake/SetDefaultBuildType.cmake)
include(cmake/UpdateTranslations.cmake)
include(cmake/CopyToBuildDir.cmake)
include(cmake/LibToDLL.cmake)

st_set_default_build_type(Release)

if (WIN32)
  file(GLOB libs_dirs "${PROJECT_SOURCE_DIR}/../libs/*libs-build*")
  find_path(LIBS_BUILD_DIR CMakeLists.txt
        HINTS "${libs_dirs}"
        DOC "Libs directory")
  if (NOT LIBS_BUILD_DIR)
    message(WARNING "Libs-build directory could not be found! "
        "You can specify it manually in LIBS_BUILD_DIR variable. Make sure you build the dependencies first!")
  else()
    set(LIB_DIR "${LIBS_BUILD_DIR}/..")
    file(GLOB jpeg_dirs "${LIB_DIR}/jpeg-[0-9]*")
    file(GLOB zlib_dirs "${LIB_DIR}/zlib-[0-9]*.[0-9]*.[0-9]*")
    file(GLOB png_dirs1 "${LIB_DIR}/libpng-[0-9]*.[0-9]*.[0-9]*")
    file(GLOB png_dirs2 "${LIB_DIR}/lpng[0-9]*")
    file(GLOB tiff_dirs "${LIB_DIR}/tiff-[0-9]*.[0-9]*.[0-9]*")
    file(GLOB qt_dirs "${LIB_DIR}/qt-*-*-[0-9]*.[0-9]*")
    file(GLOB boost_dirs "${LIB_DIR}/boost_[0-9]*_[0-9]*_[0-9]*")
  endif()
endif()

#=================================== JPEG ===================================#

find_path(
    JPEG_INCLUDE_DIR jpeglib.h
    PATHS /usr/local/include /usr/include
    HINTS ${jpeg_dirs}
    DOC "Path to libjpeg headers."
)
if (NOT JPEG_INCLUDE_DIR)
  message(FATAL_ERROR "Could not find jpeg headers.\n")
endif()

include_directories("${JPEG_INCLUDE_DIR}")

find_library(
    JPEG_LIBRARY_REL NAMES jpeg libjpeg.lib
    PATHS /usr/local/lib /usr/lib
    HINTS "${jpeg_dirs}/stage/lib"
    DOC "Path to jpeg library."
)
find_library(
    JPEG_LIBRARY_DEB NAMES jpegd libjpegd.lib
    PATHS /usr/local/lib /usr/lib
    HINTS "${jpeg_dirs}/stage/lib"
    DOC "Path to jpeg library."
)

if (JPEG_LIBRARY_REL AND NOT JPEG_LIBRARY_DEB)
  set(JPEG_LIBRARY_DEB ${JPEG_LIBRARY_REL})
elseif (NOT JPEG_LIBRARY_REL)
  message(FATAL_ERROR "Could not find jpeg library.\n")
endif()

set(JPEG_LIBRARY optimized "${JPEG_LIBRARY_REL}" debug "${JPEG_LIBRARY_DEB}")

#=================================== ZLIB ===================================#

find_path(
    ZLIB_INCLUDE_DIR zlib.h
    PATHS /usr/local/include /usr/include
    HINTS ${zlib_dirs}
    DOC "Path to zlib headers."
)
if (NOT ZLIB_INCLUDE_DIR)
  message(FATAL_ERROR "Could not find zlib headers.\n")
endif()

include_directories("${ZLIB_INCLUDE_DIR}")

find_library(
    ZLIB_LIBRARY_REL NAMES z zdll.lib
    PATHS /usr/local/lib /usr/lib
    HINTS "${zlib_dirs}/stage/lib"
    DOC "Path to zlib library."
)
find_library(
    ZLIB_LIBRARY_DEB NAMES zd zdlld.lib
    PATHS /usr/local/lib /usr/lib
    HINTS "${zlib_dirs}/stage/lib"
    DOC "Path to zlib library."
)

if (ZLIB_LIBRARY_REL AND NOT ZLIB_LIBRARY_DEB)
  set(ZLIB_LIBRARY_DEB ${ZLIB_LIBRARY_REL})
elseif (NOT ZLIB_LIBRARY_REL)
  message(FATAL_ERROR "Could not find zlib library.\n")
endif()

set(ZLIB_LIBRARY optimized "${ZLIB_LIBRARY_REL}" debug "${ZLIB_LIBRARY_DEB}")

#================================== PNG ==================================#

find_path(
    PNG_INCLUDE_DIR png.h
    PATHS /usr/local/include /usr/include
    HINTS ${png_dirs1} ${png_dirs2}
    DOC "Path to libpng headers."
)
if (NOT PNG_INCLUDE_DIR)
  message(FATAL_ERROR "Could not find libpng headers.\n")
endif()

include_directories("${PNG_INCLUDE_DIR}")

find_library(
    PNG_LIBRARY_REL NAMES png libpng.lib
    PATHS /usr/local/lib /usr/lib
    HINTS "${png_dirs1}/stage/lib" "${png_dirs2}/stage/lib"
    DOC "Path to png library."
)
find_library(
    PNG_LIBRARY_DEB NAMES pngd libpngd.lib
    PATHS /usr/local/lib /usr/lib
    HINTS "${png_dirs1}/stage/lib" "${png_dirs2}/stage/lib"
    DOC "Path to png library."
)

if (PNG_LIBRARY_REL AND NOT PNG_LIBRARY_DEB)
  set(PNG_LIBRARY_DEB ${PNG_LIBRARY_REL})
elseif (NOT PNG_LIBRARY_REL)
  message(FATAL_ERROR "Could not find png library.\n")
endif()

set(PNG_LIBRARY optimized "${PNG_LIBRARY_REL}" debug "${PNG_LIBRARY_DEB}")

#=================================== TIFF ===================================#

find_path(
    TIFF_INCLUDE_DIR tiff.h
    PATHS /usr/local/include /usr/include
    HINTS ${tiff_dirs}/libtiff
    PATH_SUFFIXES libtiff
    DOC "Path to libtiff headers."
)
if (NOT TIFF_INCLUDE_DIR)
  message(FATAL_ERROR "Could not find libtiff headers.\n")
endif()

include_directories("${TIFF_INCLUDE_DIR}")

find_library(
    TIFF_LIBRARY_REL tiff libtiff.lib
    PATHS /usr/local/lib /usr/lib
    HINTS "${tiff_dirs}/stage/lib"
    PATH_SUFFIXES libtiff
    DOC "Path to tiff library."
)
find_library(
    TIFF_LIBRARY_DEB tiffd libtiffd.lib
    PATHS /usr/local/lib /usr/lib
    HINTS "${tiff_dirs}/stage/lib"
    PATH_SUFFIXES libtiff
    DOC "Path to tiff library."
)

if (TIFF_LIBRARY_REL AND NOT TIFF_LIBRARY_DEB)
  set(TIFF_LIBRARY_DEB ${TIFF_LIBRARY_REL})
elseif (NOT TIFF_LIBRARY_REL)
  message(FATAL_ERROR "Could not find libtiff library.\n")
endif()

set(TIFF_LIBRARY optimized "${TIFF_LIBRARY_REL}" debug "${TIFF_LIBRARY_DEB}")

if (WIN32)
  add_definitions(-DUSE_LIBTIFF_DLL)
endif()

#================================= Boost ================================#

if (WIN32)
  find_path(
      BOOST_ROOT boost-build.jam PATHS ${boost_dirs}
      DOC "Path to top-level Boost source directory."
  )
  set(Boost_USE_STATIC_LIBS ON)
else()
  add_definitions(-DBOOST_TEST_DYN_LINK)
endif()
set(Boost_USE_MULTITHREADED ON)

find_package(Boost 1.60 REQUIRED COMPONENTS unit_test_framework prg_exec_monitor)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

#=================================== Qt ===================================#

if (WIN32)
  set(Qt5Core_DIR "${qt_dirs}/qtbase/lib/cmake/Qt5Core")
  set(Qt5Gui_DIR "${qt_dirs}/qtbase/lib/cmake/Qt5Gui")
  set(Qt5Widgets_DIR "${qt_dirs}/qtbase/lib/cmake/Qt5Widgets")
  set(Qt5Xml_DIR "${qt_dirs}/qtbase/lib/cmake/Qt5Xml")
  set(Qt5Network_DIR "${qt_dirs}/qtbase/lib/cmake/Qt5Network")
  set(Qt5OpenGL_DIR "${qt_dirs}/qtbase/lib/cmake/Qt5OpenGL")
  set(Qt5LinguistTools_DIR "${qt_dirs}/qttools/lib/cmake/Qt5LinguistTools")
  if (WIN_XP)
    set(Qt5LinguistTools_DIR "${qt_dirs}/qtbase/lib/cmake/Qt5LinguistTools")
  endif()
endif()

set(qt_min_version 5.6)
find_package(Qt5Core ${qt_min_version} REQUIRED)
find_package(Qt5Gui ${qt_min_version} REQUIRED)
find_package(Qt5Widgets ${qt_min_version} REQUIRED)
find_package(Qt5Xml ${qt_min_version} REQUIRED)
find_package(Qt5Network ${qt_min_version} REQUIRED)
find_package(Qt5LinguistTools ${qt_min_version} REQUIRED)
find_package(Qt5OpenGL ${qt_min_version} REQUIRED)

include_directories(${Qt5Core_INCLUDE_DIRS})
link_directories(${Qt5Core_LIBRARIES})
include_directories(${Qt5GUI_INCLUDE_DIRS})
link_directories(${Qt5GUI_LIBRARIES})
include_directories(${Qt5Widgets_INCLUDE_DIRS})
add_definitions(${Qt5Widgets_DEFINITIONS})
link_directories(${Qt5Widgets_LIBRARIES})
include_directories(${Qt5Xml_INCLUDE_DIRS})
link_directories(${Qt5Xml_LIBRARIES})
include_directories(${Qt5Network_INCLUDE_DIRS})
link_directories(${Qt5Network_LIBRARIES})
include_directories(${Qt5OpenGL_INCLUDE_DIRS})
link_directories(${Qt5OpenGL_LIBRARIES})
include_directories(${Qt5LinguistTools_INCLUDE_DIRS})
link_directories(${Qt5LinguistTools_LIBRARIES})

#=================================== Main ===================================#

set(EXTRA_LIBS "")

if (UNIX)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  link_libraries(Threads::Threads)
elseif (WIN32 AND MSVC)
  add_definitions(-DNOMINMAX)
endif()

add_definitions(-DBOOST_MULTI_INDEX_DISABLE_SERIALIZATION)

if (WIN32)
  list(APPEND EXTRA_LIBS winmm imm32 ws2_32 ole32 oleaut32 uuid gdi32 comdlg32 winspool)
endif()

list(APPEND EXTRA_LIBS ${TIFF_LIBRARY} ${PNG_LIBRARY} ${ZLIB_LIBRARY} ${JPEG_LIBRARY})

# Prepare config.h
set(PORTABLE_VERSION TRUE CACHE BOOLEAN "Whether to build the portable version or not.")
if (PORTABLE_VERSION)
  set(PORTABLE_CONFIG_DIR "config")
endif()
set(APPLICATION_NAME "scantailor-advanced")
if (WIN32)
  set(TRANSLATION_DIRS ".:translations")
  set(PLUGIN_DIRS ".")
else()
  set(TRANSLATION_DIRS ".:translations:../share/${APPLICATION_NAME}/translations")
  set(PLUGIN_DIRS ".:../lib/${APPLICATION_NAME}")
endif()

configure_file(config.h.in ${CMAKE_BINARY_DIR}/config.h @ONLY)

add_subdirectory(dewarping)
add_subdirectory(foundation)
add_subdirectory(math)
add_subdirectory(imageproc)
add_subdirectory(interaction)
add_subdirectory(zones)
add_subdirectory(tests)

file(GLOB common_ui_files ui/ErrorWidget.ui)
file(GLOB gui_only_ui_files "ui/*.ui")
foreach (ui_file ${common_ui_files})
  list(REMOVE_ITEM gui_only_ui_files "${ui_file}")
endforeach()

source_group("UI Files" FILES ${common_ui_files} ${gui_only_ui_files})
qt5_wrap_ui(common_ui_sources ${common_ui_files})
qt5_wrap_ui(gui_only_ui_sources ${gui_only_ui_files})
set_source_files_properties(${common_ui_sources} ${gui_only_ui_files} PROPERTIES GENERATED TRUE)
add_subdirectory(ui)
include_directories("${CMAKE_CURRENT_BINARY_DIR}") # for ui files

add_custom_target(toplevel_ui_sources DEPENDS ${common_ui_sources} ${gui_only_ui_sources})

add_subdirectory(filters/fix_orientation)
add_subdirectory(filters/page_split)
add_subdirectory(filters/deskew)
add_subdirectory(filters/select_content)
add_subdirectory(filters/page_layout)
add_subdirectory(filters/output)

set(resource_files "resources/resources.qrc" "resources/DarkScheme.qrc" "resources/LightScheme.qrc")
set(resource_sources)
foreach (resource_file ${resource_files})
  qt5_add_resources(resource_sources ${resource_file})
endforeach()
set_source_files_properties(${resource_sources} PROPERTIES GENERATED TRUE)
source_group("Generated" FILES ${common_ui_sources} ${gui_only_ui_sources} ${resource_sources})
source_group("Resources" FILES ${resource_files})
if (WIN32)
  source_group("Resources" FILES resources/win32/resources.rc)
endif()

set(
    common_sources
    BackgroundExecutor.cpp BackgroundExecutor.h
    PixmapRenderer.cpp PixmapRenderer.h
    BubbleAnimation.cpp BubbleAnimation.h
    ProcessingIndicationWidget.cpp ProcessingIndicationWidget.h
    NonOwningWidget.cpp NonOwningWidget.h
    Dpi.cpp Dpi.h Dpm.cpp Dpm.h
    SmartFilenameOrdering.cpp SmartFilenameOrdering.h
    AbstractRelinker.h
    RelinkablePath.cpp RelinkablePath.h
    ImageInfo.cpp ImageInfo.h
    ImageFileInfo.cpp ImageFileInfo.h
    ImageMetadata.cpp ImageMetadata.h
    RecentProjects.cpp RecentProjects.h
    OutOfMemoryHandler.cpp OutOfMemoryHandler.h
    CommandLine.cpp CommandLine.h
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
    DebugImages.cpp DebugImages.h
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
    TaskStatus.h FilterUiInterface.h
    ProjectReader.cpp ProjectReader.h
    ProjectWriter.cpp ProjectWriter.h
    XmlMarshaller.cpp XmlMarshaller.h
    XmlUnmarshaller.cpp XmlUnmarshaller.h
    AtomicFileOverwriter.cpp AtomicFileOverwriter.h
    EstimateBackground.cpp EstimateBackground.h
    Despeckle.cpp Despeckle.h
    ThreadPriority.cpp ThreadPriority.h
    FileNameDisambiguator.cpp FileNameDisambiguator.h
    OpenGLSupport.cpp OpenGLSupport.h
    OutputFileNameGenerator.cpp OutputFileNameGenerator.h
    ColorScheme.h
    DarkScheme.cpp DarkScheme.h
    LightScheme.cpp LightScheme.h
    ColorSchemeManager.cpp ColorSchemeManager.h
    PageRange.cpp PageRange.h
    SelectedPage.cpp SelectedPage.h
    Utils.cpp Utils.h
    PageView.h
    AutoManualMode.h
    AbstractCommand.h
    AbstractFilter.h
    BeforeOrAfter.h
    FilterResult.h
    CompositeCacheDrivenTask.h
    Margins.h
    ChangedStateItemDelegate.h
    PageOrderProvider.h
    PageOrderOption.h
    PayloadEvent.h
    filter_dc/AbstractFilterDataCollector.h
    filter_dc/ThumbnailCollector.h
    filter_dc/ContentBoxCollector.h
    filter_dc/PageOrientationCollector.h
    ImageViewInfoProvider.cpp ImageViewInfoProvider.h
    ImageViewInfoObserver.h
    UnitsProvider.cpp UnitsProvider.h
    UnitsObserver.h UnitsObserver.cpp
    UnitsConverter.cpp UnitsConverter.h
    Units.cpp Units.h
    DefaultParams.cpp DefaultParams.h
    DefaultParamsProfileManager.cpp DefaultParamsProfileManager.h
    DefaultParamsProvider.cpp DefaultParamsProvider.h
    DeviationProvider.h
    OrderByDeviationProvider.cpp OrderByDeviationProvider.h
    BlackOnWhiteEstimator.cpp BlackOnWhiteEstimator.h
    ImageSettings.cpp ImageSettings.h
    EmptyTaskStatus.h
    version.h
    config.h.in
    ${common_ui_files})

set(
    gui_only_sources
    Application.cpp Application.h
    SkinnedButton.cpp SkinnedButton.h
    RelinkablePathVisualization.cpp RelinkablePathVisualization.h
    RelinkingModel.cpp RelinkingModel.h
    RelinkingSortingModel.cpp RelinkingSortingModel.h
    RelinkingListView.cpp RelinkingListView.h
    RelinkingDialog.cpp RelinkingDialog.h
    SettingsDialog.cpp SettingsDialog.h
    FixDpiDialog.cpp FixDpiDialog.h
    LoadFilesStatusDialog.cpp LoadFilesStatusDialog.h
    ProjectCreationContext.cpp ProjectCreationContext.h
    ProjectOpeningContext.cpp ProjectOpeningContext.h
    OutOfMemoryDialog.cpp OutOfMemoryDialog.h
    ThumbnailSequence.cpp ThumbnailSequence.h
    ProjectFilesDialog.cpp ProjectFilesDialog.h
    NewOpenProjectPanel.cpp NewOpenProjectPanel.h
    SystemLoadWidget.cpp SystemLoadWidget.h
    MainWindow.cpp MainWindow.h
    main.cpp
    StatusBarPanel.cpp StatusBarPanel.h
    DefaultParamsDialog.cpp DefaultParamsDialog.h
    CollapsibleGroupBox.cpp CollapsibleGroupBox.h)

set(
    cli_only_sources
    ConsoleBatch.cpp ConsoleBatch.h
    main-cli.cpp
)

source_group("Sources" FILES ${common_sources} ${gui_only_sources} ${cli_only_sources})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if (POLICY CMP0071)
  cmake_policy(SET CMP0071 NEW)
endif()

source_group("Special Headers" FILES version.h config.h.in)

set(win32_resource_file)
if (WIN32)
  set(rc_file "${CMAKE_SOURCE_DIR}/resources/win32/resources.rc")
  file(GLOB win32_resources resources/win32/*.ico)
  set_source_files_properties(
      "${rc_file}" PROPERTIES
      OBJECT_DEPENDS ${win32_resources}
  )
  if (MINGW)
    # CMake doesn't know how to process .rc files with MinGW.
    set(win32_resource_file "${CMAKE_BINARY_DIR}/win32_resources.o")
    add_custom_command(
        OUTPUT "${win32_resource_file}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/resources/win32"
        COMMAND windres -i "${rc_file}" -o "${win32_resource_file}"
        MAIN_DEPENDENCY "${rc_file}"
        DEPENDS ${win32_resources}
    )
  else()
    set(win32_resource_file "${rc_file}")
  endif()
endif()

add_library(stcore STATIC ${common_sources} ${common_ui_sources})

add_executable(
    scantailor WIN32 ${gui_only_sources} ${common_ui_sources} ${gui_only_ui_sources}
    ${resource_sources} ${win32_resource_file} resources/icons/COPYING
)

target_link_libraries(
    scantailor
    fix_orientation page_split deskew select_content page_layout output stcore
    dewarping zones interaction imageproc math foundation
    ${Qt5Core_LIBRARIES} ${Qt5GUI_LIBRARIES} ${Qt5Widgets_LIBRARIES} ${Qt5Xml_LIBRARIES} ${Qt5Network_LIBRARIES}
    ${Qt5OpenGL_LIBRARIES} ${Qt5LinguistTools_LIBRARIES} ${EXTRA_LIBS}
)

if (WIN32)
  install(TARGETS scantailor RUNTIME DESTINATION .)
else()
  install(TARGETS scantailor RUNTIME DESTINATION bin)
endif()

# Translations
translation_sources(
    scantailor
    ${common_sources} ${gui_only_sources} ${cli_only_sources}
)
file(GLOB TRANSLATION_FILES translations/scantailor_*.ts)

finalize_translation_set(scantailor ${TRANSLATION_FILES})
update_translations_target(update_translations scantailor)

set(ts_files ${TRANSLATION_FILES})

# Don't build *.qm files from *untranslated.ts
set(FILTERED_TRANSLATION_FILES)
foreach (ts_file ${ts_files})
  if ("${ts_file}" MATCHES ".*untranslated.ts")
    # Just skip it.
  else()
    list(APPEND FILTERED_TRANSLATION_FILES "${ts_file}")
  endif()
endforeach()

qt5_add_translation(QM_FILES ${FILTERED_TRANSLATION_FILES})
add_custom_target(compile_translations ALL DEPENDS ${QM_FILES})

if (WIN32)
  install(FILES ${QM_FILES} DESTINATION translations)
else()
  install(FILES ${QM_FILES} DESTINATION "share/${APPLICATION_NAME}/translations")
endif()

if (WIN32)
  macro (add_runtime_libs_to_install Configuration Libs)
    set(configurations "${Configuration}")
    if (${configurations} MATCHES "ALL")
      set(configurations "DEBUG;RELEASE")
    endif()
    foreach (_config ${configurations})
      foreach (_lib ${Libs})
        if (EXISTS "${_lib}")
          list(APPEND "ADDITIONAL_RUNTIME_LIBS_${_config}" "${_lib}")
        endif()
      endforeach()
    endforeach()
  endmacro()

  # Copy some DLLs to the staging dir.
  set(
      qt5_libs
      ${Qt5Widgets_LIBRARIES} ${Qt5Gui_LIBRARIES}
      ${Qt5Core_LIBRARIES} ${Qt5Xml_LIBRARIES}
      ${Qt5Network_LIBRARIES} ${Qt5OpenGL_LIBRARIES}
  )

  foreach (target ${qt5_libs})
    get_target_property(debug_loc "${target}" LOCATION_DEBUG)
    get_target_property(release_loc "${target}" LOCATION_RELEASE)
    copy_to_build_dir("${debug_loc}" CONFIGURATIONS Debug)
    copy_to_build_dir("${release_loc}" CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
    add_runtime_libs_to_install(DEBUG "${debug_loc}")
    add_runtime_libs_to_install(RELEASE "${release_loc}")
  endforeach()

  # Qt's plugins.
  macro (set_release_and_debug_libs VarName ReleaseLib DebugLib)
    set(${VarName}_RELEASE "${ReleaseLib}")
    if (EXISTS "${DebugLib}")
      set(${VarName}_DEBUG "${DebugLib}")
    else()
      set(${VarName}_DEBUG "${ReleaseLib}")
    endif()
  endmacro()

  set(PLUGINS_DIR "${qt_dirs}/qtbase/plugins")

  set_release_and_debug_libs(qwindows
      "${PLUGINS_DIR}/platforms/qwindows.dll"
      "${PLUGINS_DIR}/platforms/qwindowsd.dll")
  copy_to_build_dir("${qwindows_RELEASE}" SUBDIR platforms CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
  copy_to_build_dir("${qwindows_DEBUG}" SUBDIR platforms CONFIGURATIONS Debug)
  install(PROGRAMS "${qwindows_DEBUG}" CONFIGURATIONS Debug DESTINATION platforms)
  install(PROGRAMS "${qwindows_RELEASE}" CONFIGURATIONS Release DESTINATION platforms)

  set_release_and_debug_libs(qjpeg "${PLUGINS_DIR}/imageformats/qjpeg.dll" "${PLUGINS_DIR}/imageformats/qjpegd.dll")
  copy_to_build_dir("${qjpeg_RELEASE}" SUBDIR imageformats CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
  copy_to_build_dir("${qjpeg_DEBUG}" SUBDIR imageformats CONFIGURATIONS Debug)
  install(PROGRAMS "${qjpeg_DEBUG}" CONFIGURATIONS Debug DESTINATION imageformats)
  install(PROGRAMS "${qjpeg_RELEASE}" CONFIGURATIONS Release DESTINATION imageformats)

  set_release_and_debug_libs(qtaccessible
      "${PLUGINS_DIR}/accessible/qtaccessiblewidgets.dll"
      "${PLUGINS_DIR}/accessible/qtaccessiblewidgetsd.dll")
  if (EXISTS "${qtaccessible_RELEASE}")
    copy_to_build_dir("${qtaccessible_RELEASE}" SUBDIR accessible CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
    copy_to_build_dir("${qtaccessible_DEBUG}" SUBDIR accessible CONFIGURATIONS Debug)
    install(PROGRAMS "${qtaccessible_DEBUG}" CONFIGURATIONS Debug DESTINATION accessible)
    install(PROGRAMS "${qtaccessible_RELEASE}" CONFIGURATIONS Release DESTINATION accessible)
  endif()

  # Copy image libs
  find_file(JPEG_LIBRARY_RT_REL NAMES libjpeg.dll HINTS "${jpeg_dirs}/bin")
  find_file(JPEG_LIBRARY_RT_DEB NAMES libjpegd.dll HINTS "${jpeg_dirs}/bin")
  if (NOT JPEG_LIBRARY_RT_DEB)
    set(JPEG_LIBRARY_RT_DEB "${JPEG_LIBRARY_RT_REL}")
  endif()

  find_file(ZLIB_LIBRARY_RT_REL NAMES libz.dll zdll.dll HINTS "${zlib_dirs}/bin")
  find_file(ZLIB_LIBRARY_RT_DEB NAMES libzd.dll zdlld.dll HINTS "${zlib_dirs}/bin")
  if (NOT ZLIB_LIBRARY_RT_DEB)
    set(ZLIB_LIBRARY_RT_DEB "${ZLIB_LIBRARY_RT_REL}")
  endif()

  find_file(PNG_LIBRARY_RT_REL NAMES libpng.dll HINTS "${png_dirs1}/bin" "${png_dirs2}/bin")
  find_file(PNG_LIBRARY_RT_DEB NAMES libpngd.dll HINTS "${png_dirs1}/bin" "${png_dirs2}/bin")
  if (NOT PNG_LIBRARY_RT_DEB)
    set(PNG_LIBRARY_RT_DEB "${PNG_LIBRARY_RT_REL}")
  endif()

  find_file(TIFF_LIBRARY_RT_REL NAMES libtiff.dll HINTS "${tiff_dirs}/bin")
  find_file(TIFF_LIBRARY_RT_DEB NAMES libtiffd.dll HINTS "${tiff_dirs}/bin")
  if (NOT TIFF_LIBRARY_RT_DEB)
    set(TIFF_LIBRARY_RT_DEB "${TIFF_LIBRARY_RT_REL}")
  endif()

  copy_to_build_dir(
      "${JPEG_LIBRARY_RT_REL};${ZLIB_LIBRARY_RT_REL};${TIFF_LIBRARY_RT_REL};${PNG_LIBRARY_RT_REL}"
      CONFIGURATIONS Release MinSizeRel RelWithDebInfo
  )
  copy_to_build_dir(
      "${JPEG_LIBRARY_RT_DEB};${TIFF_LIBRARY_RT_DEB};${ZLIB_LIBRARY_RT_DEB};${PNG_LIBRARY_RT_DEB}"
      CONFIGURATIONS Debug
  )
  add_runtime_libs_to_install(DEBUG
      "${JPEG_LIBRARY_RT_DEB};${TIFF_LIBRARY_RT_DEB};${ZLIB_LIBRARY_RT_DEB};${PNG_LIBRARY_RT_DEB}")
  add_runtime_libs_to_install(RELEASE
      "${JPEG_LIBRARY_RT_REL};${ZLIB_LIBRARY_RT_REL};${TIFF_LIBRARY_RT_REL};${PNG_LIBRARY_RT_REL}")

  if (MINGW)
    get_filename_component(_mingw_path ${CMAKE_CXX_COMPILER} PATH)
    file(GLOB libgcc_s "${_mingw_path}/libgcc_s_*.dll")
    file(GLOB libstdcpp "${_mingw_path}/libstdc++*.dll")
    file(GLOB libwinpthread "${_mingw_path}/libwinpthread*.dll")
    copy_to_build_dir("${libgcc_s};${libstdcpp};${libwinpthread}")
    add_runtime_libs_to_install(ALL "${libgcc_s};${libstdcpp};${libwinpthread}")
  endif()

  # Generate the target that will actually do the copying.
  generate_copy_to_build_dir_target(copy_to_build_dir)

  install(PROGRAMS ${ADDITIONAL_RUNTIME_LIBS_DEBUG} CONFIGURATIONS Debug DESTINATION .)
  install(PROGRAMS ${ADDITIONAL_RUNTIME_LIBS_RELEASE} CONFIGURATIONS Release DESTINATION .)
endif()

if (UNIX)
  install(FILES "${CMAKE_SOURCE_DIR}/resources/unix/scantailor.desktop" DESTINATION "share/applications")
  install(FILES "${CMAKE_SOURCE_DIR}/resources/appicon.svg"
      DESTINATION "share/icons/hicolor/scalable/apps"
      RENAME "ScanTailor.svg")
  install(FILES "${CMAKE_SOURCE_DIR}/resources/unix/mime/scantailor-project.xml" DESTINATION "share/mime/packages")
endif()

# Packaging
if (WIN32)
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
  if (MSVC)
    set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
  endif()
endif()
include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME "${APPLICATION_NAME}")
string(REGEX REPLACE "(.*)\\..*\\..*" "\\1" CPACK_PACKAGE_VERSION_MAJOR "${VERSION}")
string(REGEX REPLACE ".*\\.(.*)\\..*" "\\1" CPACK_PACKAGE_VERSION_MINOR "${VERSION}")
string(REGEX REPLACE ".*\\..*\\.(.*)" "\\1" CPACK_PACKAGE_VERSION_PATCH "${VERSION}")
set(CPACK_PACKAGE_VENDOR "4lex4 <4lex49@zoho.com>")
set(CPACK_PACKAGE_CONTACT "${CPACK_PACKAGE_VENDOR}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Interactive post-processing tool for scanned pages.")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_EXECUTABLES "scantailor;${CMAKE_PROJECT_NAME}")
set(CPACK_CREATE_DESKTOP_LINKS "scantailor")
if (WIN32)
  set(CPACK_NSIS_INSTALLED_ICON_NAME "scantailor.exe")
  set(CPACK_NSIS_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
  set(CPACK_NSIS_DISPLAY_NAME "${CMAKE_PROJECT_NAME} ${VERSION}")
  set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL "ON")
  set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CMAKE_PROJECT_NAME}")
  set(CPACK_NSIS_CREATE_ICONS_EXTRA
      "CreateShortCut \\\"$DESKTOP\\\\${CMAKE_PROJECT_NAME}.lnk\\\" \\\"$INSTDIR\\\\scantailor.exe\\\""
  )
  set(CPACK_NSIS_DELETE_ICONS_EXTRA
      "Delete \\\"$DESKTOP\\\\${CMAKE_PROJECT_NAME}.lnk\\\""
  )
endif()
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${APPLICATION_NAME}-${VERSION}")
set(
  CPACK_SOURCE_IGNORE_FILES
  "/\\\\.svn/"
  "/\\\\.git/"
  "~$"
  "\\\\.pcs$"
  "TODO.txt"
  "CMakeLists.txt.user"
  "/doxygen/"
  "${CMAKE_BINARY_DIR}"
)
include(CPack)

# uninstall target
if (NOT TARGET uninstall)
  configure_file(
      "${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
      "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake"
      @ONLY)

  add_custom_target(uninstall
      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/cmake_uninstall.cmake)
endif()