CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0)

PROJECT("ScanTailor")

# setting compiler flags
SET(CMAKE_CXX_STANDARD 17)

IF (MSVC AND WIN_XP)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D_USING_V110_SDK71_")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_USING_V110_SDK71_")
    SET(CMAKE_EXE_LINKER_FLAGS "/SUBSYSTEM:CONSOLE,5.01 /SUBSYSTEM:WINDOWS,5.01 ${CMAKE_EXE_LINKER_FLAGS}")
ENDIF ()

IF (MSVC)
    # Disable checked iterators for extra performance.
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D_SECURE_SCL=0")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_SECURE_SCL=0")

    IF (DEBUG_CLI)
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /DDEBUG_CLI")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DDEBUG_CLI")
    ENDIF (DEBUG_CLI)
ENDIF ()

IF (UNIX)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
ENDIF ()

SET(
        CMAKE_C_FLAGS "${CMAKE_C_FLAGS}"
        CACHE STRING "Common C flags for all build configurations." FORCE
)
SET(
        CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}"
        CACHE STRING "Common C++ flags for all build configurations." FORCE
)
SET(
        CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}"
        CACHE STRING "Common link flags for all build configurations." FORCE
)

ENABLE_TESTING()

# An undocumented side-effect of CONFIGURE_FILE() is that it makes
# the whole project depend on the file we are parsing / copying.
CONFIGURE_FILE(
        "${PROJECT_SOURCE_DIR}/version.h"
        "${PROJECT_BINARY_DIR}/.version.h" COPYONLY
)

# Prevent this leftover from old builds to be used in favour
# of the one in ${PROJECT_SOURCE_DIR}
IF (NOT "${PROJECT_BINARY_DIR}" STREQUAL "${PROJECT_SOURCE_DIR}")
    FILE(REMOVE "${PROJECT_BINARY_DIR}/version.h")
ENDIF ()

# Extract VERSION and VERSION_QUAD from version.h
FILE(READ "${PROJECT_SOURCE_DIR}/version.h" version_h_contents)
STRING(
        REGEX REPLACE
        ".*#define[ \\t]+VERSION[ \\t]+\"([^\"]*)\".*"
        "\\1" VERSION "${version_h_contents}"
)
IF ("${VERSION}" STREQUAL "${version_h_contents}")
    MESSAGE(FATAL_ERROR "Failed to extract VERSION from version.h")
ENDIF ()

# VERSION_QUAD must be either empty or be in the form of X.Y.Z.Y
STRING(
        REGEX REPLACE
        ".*#define[ \\t]+VERSION_QUAD[ \\t]+\"(([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)?)\".*"
        "\\1" VERSION_QUAD "${version_h_contents}"
)
IF ("${VERSION_QUAD}" STREQUAL "${version_h_contents}")
    MESSAGE(FATAL_ERROR "Failed to extract VERSION_QUAD from version.h")
ENDIF ()

# This has to go quite early on, as otherwise we risk picking
# up an identically named header from a system include path.
INCLUDE_DIRECTORIES(. foundation math interaction zones)

# For config.h
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")

INCLUDE(cmake/FindPthreads.cmake)
INCLUDE(cmake/SetDefaultBuildType.cmake)
INCLUDE(cmake/UpdateTranslations.cmake)
INCLUDE(cmake/CopyToBuildDir.cmake)
INCLUDE(cmake/LibToDLL.cmake)

ST_SET_DEFAULT_BUILD_TYPE(Release)

IF (WIN32)
    FILE(GLOB libs_dir_ "${PROJECT_SOURCE_DIR}/../libs/*libs-build*")
    get_filename_component(libs_dir_name_ ${libs_dir_} NAME)
    FIND_PATH(
            LIB_DIR ${libs_dir_name_}/CMakeLists.txt
            HINTS "${libs_dir_}/.."
            DOC "Libs directory"
    )
    IF (NOT LIB_DIR)
        MESSAGE(
                FATAL_ERROR "Libs directory could not be found! "
                "You can specify it manually in LIB_DIR variable. Make sure you build the dependencies first!"
        )
    ENDIF ()

    FILE(GLOB jpeg_dir_ "${LIB_DIR}/jpeg-[0-9]*")
    FILE(GLOB zlib_dir_ "${LIB_DIR}/zlib-[0-9]*.[0-9]*.[0-9]*")
    FILE(GLOB png_dir_1 "${LIB_DIR}/libpng-[0-9]*.[0-9]*.[0-9]*")
    FILE(GLOB png_dir_2 "${LIB_DIR}/lpng[0-9]*")
    FILE(GLOB tiff_dir_ "${LIB_DIR}/tiff-[0-9]*.[0-9]*.[0-9]*")
    FILE(GLOB qt_dir_ "${LIB_DIR}/qt-*-*-[0-9]*.[0-9]*")
    FILE(GLOB boost_dir_ "${LIB_DIR}/boost_[0-9]*_[0-9]*_[0-9]*")
    #FILE(GLOB opencv_dir_ "${LIB_DIR}/opencv*")
ENDIF ()

#=================================== JPEG ===================================#

FIND_PATH(
        JPEG_INCLUDE_DIR jpeglib.h
        PATHS /usr/local/include /usr/include
        HINTS ${jpeg_dir_}
        DOC "Path to libjpeg headers."
)
IF (NOT JPEG_INCLUDE_DIR)
    MESSAGE(
            FATAL_ERROR
            "Could not find jpeg headers.\n"
    )
ENDIF ()

INCLUDE_DIRECTORIES("${JPEG_INCLUDE_DIR}")

FIND_LIBRARY(
        JPEG_LIBRARY_REL NAMES jpeg libjpeg.lib
        PATHS /usr/local/lib /usr/lib
        HINTS "${jpeg_dir_}/stage/lib"
        DOC "Path to jpeg library."
)
FIND_LIBRARY(
        JPEG_LIBRARY_DEB NAMES jpegd libjpegd.lib
        PATHS /usr/local/lib /usr/lib
        HINTS "${jpeg_dir_}/stage/lib"
        DOC "Path to jpeg library."
)

IF (JPEG_LIBRARY_DEB)
    SET(JPEG_LIBRARY optimized ${JPEG_LIBRARY_REL} debug ${JPEG_LIBRARY_DEB})
ELSE ()
    SET(JPEG_LIBRARY ${JPEG_LIBRARY_REL})
ENDIF ()

IF (NOT JPEG_LIBRARY)
    MESSAGE(
            FATAL_ERROR
            "Could not find jpeg library.\n"
    )
ENDIF ()

#=================================== ZLIB ===================================#

FIND_PATH(
        ZLIB_INCLUDE_DIR zlib.h
        PATHS /usr/local/include /usr/include
        HINTS ${zlib_dir_}
        DOC "Path to zlib headers."
)
IF (NOT ZLIB_INCLUDE_DIR)
    MESSAGE(
            FATAL_ERROR
            "Could not find zlib headers.\n"
    )
ENDIF ()

INCLUDE_DIRECTORIES("${ZLIB_INCLUDE_DIR}")

FIND_LIBRARY(
        ZLIB_LIBRARY_REL NAMES z zdll.lib
        PATHS /usr/local/lib /usr/lib
        HINTS "${zlib_dir_}/stage/lib"
        DOC "Path to zlib library."
)
FIND_LIBRARY(
        ZLIB_LIBRARY_DEB NAMES zd zdlld.lib
        PATHS /usr/local/lib /usr/lib
        HINTS "${zlib_dir_}/stage/lib"
        DOC "Path to jpeg library."
)

IF (ZLIB_LIBRARY_DEB)
    SET(ZLIB_LIBRARY optimized ${ZLIB_LIBRARY_REL} debug ${ZLIB_LIBRARY_DEB})
ELSE ()
    SET(ZLIB_LIBRARY ${ZLIB_LIBRARY_REL})
ENDIF ()

IF (NOT ZLIB_LIBRARY)
    MESSAGE(
            FATAL_ERROR
            "Could not find zlib library.\n"
    )
ENDIF ()

#================================== LIBPNG ==================================#

FIND_PATH(
        PNG_INCLUDE_DIR png.h
        PATHS /usr/local/include /usr/include
        HINTS ${png_dir_1} ${png_dir_2}
        DOC "Path to libpng headers."
)
IF (NOT PNG_INCLUDE_DIR)
    MESSAGE(
            FATAL_ERROR
            "Could not find libpng headers.\n"
    )
ENDIF ()

INCLUDE_DIRECTORIES("${PNG_INCLUDE_DIR}")

FIND_LIBRARY(
        PNG_LIBRARY_REL NAMES png libpng.lib
        PATHS /usr/local/lib /usr/lib
        HINTS "${png_dir_1}/stage/lib" "${png_dir_2}/stage/lib"
        DOC "Path to png library."
)
FIND_LIBRARY(
        PNG_LIBRARY_DEB NAMES pngd libpngd.lib
        PATHS /usr/local/lib /usr/lib
        HINTS "${png_dir_1}/stage/lib" "${png_dir_2}/stage/lib"
        DOC "Path to png library."
)

IF (PNG_LIBRARY_DEB)
    SET(PNG_LIBRARY optimized ${PNG_LIBRARY_REL} debug ${PNG_LIBRARY_DEB})
ELSE ()
    SET(PNG_LIBRARY ${PNG_LIBRARY_REL})
ENDIF ()

IF (NOT PNG_LIBRARY)
    MESSAGE(
            FATAL_ERROR
            "Could not find libpng library.\n"
    )
ENDIF ()

#=================================== TIFF ===================================#

FIND_PATH(
        TIFF_INCLUDE_DIR tiff.h
        PATHS /usr/local/include /usr/include
        HINTS ${tiff_dir_}/libtiff
        PATH_SUFFIXES libtiff
        DOC "Path to libtiff headers."
)
IF (NOT TIFF_INCLUDE_DIR)
    MESSAGE(
            FATAL_ERROR
            "Could not find libtiff headers.\n"
    )
ENDIF ()

INCLUDE_DIRECTORIES("${TIFF_INCLUDE_DIR}")

FIND_LIBRARY(
        TIFF_LIBRARY_REL tiff libtiff.lib
        PATHS /usr/local/lib /usr/lib
        HINTS "${tiff_dir_}/stage/lib"
        PATH_SUFFIXES libtiff
        DOC "Path to tiff library."
)
FIND_LIBRARY(
        TIFF_LIBRARY_DEB tiffd libtiffd.lib
        PATHS /usr/local/lib /usr/lib
        HINTS "${tiff_dir_}/stage/lib"
        PATH_SUFFIXES libtiff
        DOC "Path to tiff library."
)

IF (TIFF_LIBRARY_DEB)
    SET(TIFF_LIBRARY optimized ${TIFF_LIBRARY_REL} debug ${TIFF_LIBRARY_DEB})
ELSE ()
    SET(TIFF_LIBRARY ${TIFF_LIBRARY_REL})
ENDIF ()

IF (NOT TIFF_LIBRARY)
    MESSAGE(
            FATAL_ERROR
            "Could not find libtiff library.\n"
    )
ENDIF ()

IF (WIN32)
    ADD_DEFINITIONS(-DUSE_LIBTIFF_DLL)
ENDIF ()

#================================= Boost ================================#

IF (WIN32)
    FIND_PATH(
            BOOST_ROOT boost-build.jam PATHS ${boost_dir_}
            DOC "Path to top-level Boost source directory."
    )
    SET(Boost_USE_STATIC_LIBS ON)
ELSE (WIN32)
    ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
ENDIF (WIN32)
SET(Boost_USE_MULTITHREADED ON)

FIND_PACKAGE(Boost 1.60 COMPONENTS unit_test_framework prg_exec_monitor REQUIRED)
IF (NOT Boost_FOUND)
    MESSAGE(
            FATAL_ERROR
            "Could not find boost headers or libraries.\n"
    )
ENDIF (NOT Boost_FOUND)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

#=================================== Qt ===================================#

IF (WIN32)
    SET(Qt5Core_DIR "${qt_dir_}/qtbase/lib/cmake/Qt5Core")
    SET(Qt5Gui_DIR "${qt_dir_}/qtbase/lib/cmake/Qt5Gui")
    SET(Qt5Widgets_DIR "${qt_dir_}/qtbase/lib/cmake/Qt5Widgets")
    SET(Qt5Xml_DIR "${qt_dir_}/qtbase/lib/cmake/Qt5Xml")
    SET(Qt5Network_DIR "${qt_dir_}/qtbase/lib/cmake/Qt5Network")
    SET(Qt5OpenGL_DIR "${qt_dir_}/qtbase/lib/cmake/Qt5OpenGL")
    SET(Qt5LinguistTools_DIR "${qt_dir_}/qttools/lib/cmake/Qt5LinguistTools")
    IF (WIN_XP)
        SET(Qt5LinguistTools_DIR "${qt_dir_}/qtbase/lib/cmake/Qt5LinguistTools")
    ENDIF (WIN_XP)
ENDIF (WIN32)

SET(qt_min_version 5.6)
FIND_PACKAGE(Qt5Core ${qt_min_version} REQUIRED)
FIND_PACKAGE(Qt5Gui ${qt_min_version} REQUIRED)
FIND_PACKAGE(Qt5Widgets ${qt_min_version} REQUIRED)
FIND_PACKAGE(Qt5Xml ${qt_min_version} REQUIRED)
FIND_PACKAGE(Qt5Network ${qt_min_version} REQUIRED)
FIND_PACKAGE(Qt5LinguistTools ${qt_min_version} REQUIRED)
FIND_PACKAGE(Qt5OpenGL ${qt_min_version} REQUIRED)

INCLUDE_DIRECTORIES(${Qt5Core_INCLUDE_DIRS})
LINK_DIRECTORIES(${Qt5Core_LIBRARIES})
INCLUDE_DIRECTORIES(${Qt5GUI_INCLUDE_DIRS})
LINK_DIRECTORIES(${Qt5GUI_LIBRARIES})
INCLUDE_DIRECTORIES(${Qt5Widgets_INCLUDE_DIRS})
ADD_DEFINITIONS(${Qt5Widgets_DEFINITIONS})
LINK_DIRECTORIES(${Qt5Widgets_LIBRARIES})
INCLUDE_DIRECTORIES(${Qt5Xml_INCLUDE_DIRS})
LINK_DIRECTORIES(${Qt5Xml_LIBRARIES})
INCLUDE_DIRECTORIES(${Qt5Network_INCLUDE_DIRS})
LINK_DIRECTORIES(${Qt5Network_LIBRARIES})
INCLUDE_DIRECTORIES(${Qt5OpenGL_INCLUDE_DIRS})
LINK_DIRECTORIES(${Qt5OpenGL_LIBRARIES})
INCLUDE_DIRECTORIES(${Qt5LinguistTools_INCLUDE_DIRS})
LINK_DIRECTORIES(${Qt5LinguistTools_LIBRARIES})

#=================================== OpenCV ===================================#


#   find_package(OpenCV PATHS opencv_dir_ REQUIRED)
#   IF (NOT OpenCV_FOUND)
#       MESSAGE(
#               FATAL_ERROR
#               "Could not find OpenCV headers or libraries.\n"
#       )
#   ENDIF ()
#
#   INCLUDE_DIRECTORIES(${OpenCV_INCLUDE_DIRS})
#   LINK_DIRECTORIES(${OpenCV_LIBRARIES})


#=================================== Main ===================================#

SET(EXTRA_LIBS "")

IF (UNIX)
    FindPthreads()
    IF (PTHREADS_FOUND)
        ADD_DEFINITIONS(${PTHREADS_CFLAGS})
        LINK_LIBRARIES(${PTHREADS_LIBS})
    ELSE (PTHREADS_FOUND)
        MESSAGE(
                FATAL_ERROR
                "Could not detect threading flags.\n"
                "Try specifying them manually in PTHREADS_CFLAGS and PTHREADS_LIBS."
        )
    ENDIF (PTHREADS_FOUND)
ELSEIF (WIN32 AND MSVC)
    ADD_DEFINITIONS(-DNOMINMAX)
ENDIF (UNIX)

ADD_DEFINITIONS(-DBOOST_MULTI_INDEX_DISABLE_SERIALIZATION)

IF (WIN32)
    LIST(APPEND EXTRA_LIBS winmm imm32 ws2_32 ole32 oleaut32 uuid gdi32 comdlg32 winspool)
ENDIF ()

LIST(APPEND EXTRA_LIBS ${TIFF_LIBRARY} ${PNG_LIBRARY} ${ZLIB_LIBRARY} ${JPEG_LIBRARY})

# Prepare config.h
IF (WIN32)
    SET(TRANSLATION_DIRS ".:translations")
    SET(PLUGIN_DIRS ".")
ELSE ()
    SET(TRANSLATION_DIRS ".:${CMAKE_INSTALL_PREFIX}/share/scantailor/translations")
    SET(PLUGIN_DIRS ".:${CMAKE_INSTALL_PREFIX}/lib/scantailor")
ENDIF ()

CONFIGURE_FILE(config.h.in ${CMAKE_BINARY_DIR}/config.h @ONLY)

ADD_SUBDIRECTORY(dewarping)
ADD_SUBDIRECTORY(foundation)
ADD_SUBDIRECTORY(math)
ADD_SUBDIRECTORY(imageproc)
ADD_SUBDIRECTORY(interaction)
ADD_SUBDIRECTORY(zones)
ADD_SUBDIRECTORY(tests)

FILE(GLOB common_ui_files ui/ErrorWidget.ui)
FILE(GLOB gui_only_ui_files "ui/*.ui")
FOREACH (ui_file ${common_ui_files})
    LIST(REMOVE_ITEM gui_only_ui_files "${ui_file}")
ENDFOREACH ()

SOURCE_GROUP("UI Files" FILES ${common_ui_files} ${gui_only_ui_files})
QT5_WRAP_UI(common_ui_sources ${common_ui_files})
QT5_WRAP_UI(gui_only_ui_sources ${gui_only_ui_files})
SET_SOURCE_FILES_PROPERTIES(${common_ui_sources} ${gui_only_ui_files} PROPERTIES GENERATED TRUE)
ADD_SUBDIRECTORY(ui)
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}") # for ui files

ADD_CUSTOM_TARGET(toplevel_ui_sources DEPENDS ${common_ui_sources} ${gui_only_ui_sources})

ADD_SUBDIRECTORY(filters/fix_orientation)
ADD_SUBDIRECTORY(filters/page_split)
ADD_SUBDIRECTORY(filters/deskew)
ADD_SUBDIRECTORY(filters/select_content)
ADD_SUBDIRECTORY(filters/page_layout)
ADD_SUBDIRECTORY(filters/output)

SET(resource_files "resources/resources.qrc" "resources/DarkScheme.qrc" "resources/LightScheme.qrc")
SET(resource_sources)
FOREACH (resource_file ${resource_files})
    QT5_ADD_RESOURCES(resource_sources ${resource_file})
ENDFOREACH ()
SET_SOURCE_FILES_PROPERTIES(${resource_sources} PROPERTIES GENERATED TRUE)
SOURCE_GROUP("Generated" FILES ${common_ui_sources} ${gui_only_ui_sources} ${resource_sources})
SOURCE_GROUP("Resources" FILES ${resource_files})
IF (WIN32)
    SOURCE_GROUP("Resources" FILES resources/win32/resources.rc)
ENDIF ()

SET(
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
        ImageSettings.cpp ImageSettings.h
        version.h
        config.h.in
        ${common_ui_files})

SET(
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

SET(
        cli_only_sources
        ConsoleBatch.cpp ConsoleBatch.h
        main-cli.cpp
)

SOURCE_GROUP("Sources" FILES ${common_sources} ${gui_only_sources} ${cli_only_sources})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

IF (POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
ENDIF (POLICY CMP0071)

SOURCE_GROUP("Special Headers" FILES version.h config.h.in)

SET(win32_resource_file)
IF (WIN32)
    SET(rc_file "${CMAKE_SOURCE_DIR}/resources/win32/resources.rc")
    FILE(GLOB win32_resources resources/win32/*.ico)
    SET_SOURCE_FILES_PROPERTIES(
            "${rc_file}" PROPERTIES
            OBJECT_DEPENDS ${win32_resources}
    )
    IF (MINGW)
        # CMake doesn't know how to process .rc files with MinGW.
        SET(win32_resource_file "${CMAKE_BINARY_DIR}/win32_resources.o")
        ADD_CUSTOM_COMMAND(
                OUTPUT "${win32_resource_file}"
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/resources/win32"
                COMMAND windres -i "${rc_file}" -o "${win32_resource_file}"
                MAIN_DEPENDENCY "${rc_file}"
                DEPENDS ${win32_resources}
        )
    ELSE (MINGW)
        SET(win32_resource_file "${rc_file}")
    ENDIF (MINGW)
ENDIF (WIN32)

ADD_LIBRARY(stcore STATIC ${common_sources} ${common_ui_sources})

ADD_EXECUTABLE(
        scantailor WIN32 ${gui_only_sources} ${common_ui_sources} ${gui_only_ui_sources}
        ${resource_sources} ${win32_resource_file} resources/icons/COPYING
)

#ADD_EXECUTABLE(scantailor-cli ${cli_only_sources} ${common_ui_sources})

TARGET_LINK_LIBRARIES(
        scantailor
        fix_orientation page_split deskew select_content page_layout output stcore
        dewarping zones interaction imageproc math foundation
        ${Qt5Core_LIBRARIES} ${Qt5GUI_LIBRARIES} ${Qt5Widgets_LIBRARIES} ${Qt5Xml_LIBRARIES} ${Qt5Network_LIBRARIES}
        ${Qt5OpenGL_LIBRARIES} ${Qt5LinguistTools_LIBRARIES} ${EXTRA_LIBS}
        # ${OpenCV_LIBRARIES}
)
#TARGET_LINK_LIBRARIES(
#        scantailor-cli
#        fix_orientation page_split deskew select_content page_layout output
#        stcore dewarping zones interaction imageproc math foundation
#        ${Qt5Core_LIBRARIES} ${Qt5Widgets_LIBRARIES} ${Qt5Xml_LIBRARIES} ${Qt5Network_LIBRARIES}
#        ${Qt5LinguistTools_LIBRARIES} ${EXTRA_LIBS}
#        # ${OpenCV_LIBRARIES}
#)

#INSTALL(TARGETS scantailor scantailor-cli RUNTIME DESTINATION bin)
INSTALL(TARGETS scantailor RUNTIME DESTINATION bin)

# Translations
TRANSLATION_SOURCES(
        scantailor
        ${common_sources} ${gui_only_sources} ${cli_only_sources}
)
FILE(GLOB TRANSLATION_FILES translations/scantailor_*.ts)

FINALIZE_TRANSLATION_SET(scantailor ${TRANSLATION_FILES})
UPDATE_TRANSLATIONS_TARGET(update_translations scantailor)

SET(ts_files ${TRANSLATION_FILES})

# Don't build *.qm files from *untranslated.ts
SET(FILTERED_TRANSLATION_FILES)
FOREACH (ts_file ${ts_files})
    IF ("${ts_file}" MATCHES ".*untranslated.ts")
        # Just skip it.
    ELSE ("${ts_file}" MATCHES ".*untranslated.ts")
        LIST(APPEND FILTERED_TRANSLATION_FILES "${ts_file}")
    ENDIF ("${ts_file}" MATCHES ".*untranslated.ts")
ENDFOREACH (ts_file)

QT5_ADD_TRANSLATION(QM_FILES ${FILTERED_TRANSLATION_FILES})
ADD_CUSTOM_TARGET(compile_translations ALL DEPENDS ${QM_FILES})

INSTALL(FILES ${QM_FILES} DESTINATION "share/scantailor/translations")

IF (WIN32)
    # Copy some DLLs to the staging dir.

    SET(
            qt5_libs
            ${Qt5Widgets_LIBRARIES} ${Qt5Gui_LIBRARIES}
            ${Qt5Core_LIBRARIES} ${Qt5Xml_LIBRARIES}
            ${Qt5Network_LIBRARIES} ${Qt5OpenGL_LIBRARIES}
    )

    FOREACH (target ${qt5_libs})
        GET_TARGET_PROPERTY(debug_loc "${target}" LOCATION_DEBUG)
        GET_TARGET_PROPERTY(release_loc "${target}" LOCATION_RELEASE)
        COPY_TO_BUILD_DIR("${debug_loc}" CONFIGURATIONS Debug)
        COPY_TO_BUILD_DIR("${release_loc}" CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
    ENDFOREACH ()

    # Qt's plugins.
    SET(PLUGINS_DIR "${qt_dir_}/qtbase/plugins")
    COPY_TO_BUILD_DIR(
            "${PLUGINS_DIR}/platforms/qwindows.dll" SUBDIR platforms
            CONFIGURATIONS Release MinSizeRel RelWithDebInfo
    )
    COPY_TO_BUILD_DIR(
            "${PLUGINS_DIR}/platforms/qwindowsd.dll" SUBDIR platforms
            CONFIGURATIONS Debug
    )

    COPY_TO_BUILD_DIR(
            "${PLUGINS_DIR}/imageformats/qjpeg.dll" SUBDIR imageformats
            CONFIGURATIONS Release MinSizeRel RelWithDebInfo
    )
    COPY_TO_BUILD_DIR(
            "${PLUGINS_DIR}/imageformats/qjpegd.dll" SUBDIR imageformats
            CONFIGURATIONS Debug
    )

    IF (EXISTS "${PLUGINS_DIR}/accessible/qtaccessiblewidgets.dll")
        COPY_TO_BUILD_DIR(
                "${PLUGINS_DIR}/accessible/qtaccessiblewidgets.dll" SUBDIR accessible
                CONFIGURATIONS Release MinSizeRel RelWithDebInfo
        )
    ENDIF ()
    IF (EXISTS "${PLUGINS_DIR}/accessible/qtaccessiblewidgetsd.dll")
        COPY_TO_BUILD_DIR(
                "${PLUGINS_DIR}/accessible/qtaccessiblewidgetsd.dll" SUBDIR accessible
                CONFIGURATIONS Debug
        )
    ENDIF ()

    FIND_FILE(
            JPEG_LIBRARY_RT_REL NAMES jpeg libjpeg libjpeg.dll
            HINTS "${jpeg_dir_}/bin"
            DOC "Path to jpeg runtime library."
    )
    FIND_FILE(
            JPEG_LIBRARY_RT_DEB NAMES jpegd libjpegd libjpegd.dll
            HINTS "${jpeg_dir_}/bin"
            DOC "Path to jpeg runtime library."
    )
    COPY_TO_BUILD_DIR("${JPEG_LIBRARY_RT_REL}" CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
    COPY_TO_BUILD_DIR("${JPEG_LIBRARY_RT_DEB}" CONFIGURATIONS Debug)

    FIND_FILE(
            ZLIB_LIBRARY_RT_REL NAMES z libz libz.dll zdll.dll
            HINTS "${zlib_dir_}/bin"
            DOC "Path to zlib runtime library."
    )
    FIND_FILE(
            ZLIB_LIBRARY_RT_DEB NAMES zd libzd libzd.dll zdlld.dll
            HINTS "${zlib_dir_}/bin"
            DOC "Path to zlib runtime library."
    )
    COPY_TO_BUILD_DIR("${ZLIB_LIBRARY_RT_REL}" CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
    COPY_TO_BUILD_DIR("${ZLIB_LIBRARY_RT_DEB}" CONFIGURATIONS Debug)

    FIND_FILE(
            PNG_LIBRARY_RT_REL NAMES png libpng libpng.dll
            HINTS "${png_dir_1}/bin" "${png_dir_2}/bin"
            DOC "Path to png runtime library."
    )
    FIND_FILE(
            PNG_LIBRARY_RT_DEB NAMES pngd libpngd libpngd.dll
            HINTS "${png_dir_1}/bin" "${png_dir_2}/bin"
            DOC "Path to png runtime library."
    )
    COPY_TO_BUILD_DIR("${PNG_LIBRARY_RT_REL}" CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
    COPY_TO_BUILD_DIR("${PNG_LIBRARY_RT_DEB}" CONFIGURATIONS Debug)

    FIND_FILE(
            TIFF_LIBRARY_RT_REL NAMES tiff libtiff libtiff.dll
            HINTS "${tiff_dir_}/bin"
            DOC "Path to tiff runtime library."
    )
    FIND_FILE(
            TIFF_LIBRARY_RT_DEB NAMES tiffd libtiffd libtiffd.dll
            HINTS "${tiff_dir_}/bin"
            DOC "Path to tiff runtime library."
    )
    COPY_TO_BUILD_DIR("${TIFF_LIBRARY_RT_REL}" CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
    COPY_TO_BUILD_DIR("${TIFF_LIBRARY_RT_DEB}" CONFIGURATIONS Debug)

    IF (MINGW)
        GET_FILENAME_COMPONENT(mingw_path_ ${CMAKE_CXX_COMPILER} PATH)
        COPY_TO_BUILD_DIR("${mingw_path_}/libgcc_s_seh-1.dll" "${mingw_path_}/libgcc_s_sjlj-1.dll" "${mingw_path_}/libgcc_s_dw2-1.dll"
                "${mingw_path_}/libstdc++-6.dll" "${mingw_path_}/libwinpthread-1.dll")
    ENDIF (MINGW)

    # Generate the target that will actually do the copying.
    GENERATE_COPY_TO_BUILD_DIR_TARGET(copy_to_build_dir)
ENDIF (WIN32)
