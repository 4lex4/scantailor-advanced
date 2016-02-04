CMAKE_MINIMUM_REQUIRED(VERSION 2.8.4)

PROJECT("ScanTailor")

IF (DEBUG_CLI)
    ADD_DEFINITIONS(-DDEBUG_CLI)
ENDIF (DEBUG_CLI)

ENABLE_TESTING()

# This has to go quite early on, as otherwise we risk picking
# up an identically named header from a system include path.
INCLUDE_DIRECTORIES(. foundation math interaction zones)

INCLUDE(cmake/FindPthreads.cmake)
INCLUDE(cmake/SetDefaultBuildType.cmake)
INCLUDE(cmake/UpdateTranslations.cmake)
INCLUDE(cmake/CopyToBuildDir.cmake)
INCLUDE(cmake/LibToDLL.cmake)

IF (CMAKE_COMPILER_IS_GNUCC AND CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "-static-libgcc -static-libstdc++")
ENDIF ()

ST_SET_DEFAULT_BUILD_TYPE(Release)

IF (WIN32)
    FIND_PATH(
            LIB_DIR LIBS-BUILD/CMakeLists.txt
            HINTS "${PROJECT_SOURCE_DIR}/../libs"
            DOC "Libs directory"
    )
    IF (NOT LIB_DIR)
        MESSAGE(
                FATAL_ERROR "Libs directory could not be found! "
                "You can specify it manually in LIB_DIR variable. Make sure you build the dependencies first!"
        )
    ENDIF ()
ENDIF ()

FILE(GLOB jpeg_dir_ "${LIB_DIR}/jpeg-[0-9]*")
FILE(GLOB zlib_dir_ "${LIB_DIR}/zlib-[0-9]*.[0-9]*.[0-9]*")
FILE(GLOB png_dir_1 "${LIB_DIR}/libpng-[0-9]*.[0-9]*.[0-9]*")
FILE(GLOB png_dir_2 "${LIB_DIR}/lpng[0-9]*")
FILE(GLOB tiff_dir_ "${LIB_DIR}/tiff-[0-9]*.[0-9]*.[0-9]*")
FILE(GLOB qt_dir_ "${LIB_DIR}/qt-*-*-[0-9]*.[0-9]*")
FILE(GLOB boost_dir_ "${LIB_DIR}/boost_[0-9]*_[0-9]*_[0-9]*")
FILE(GLOB opencv_dir_ "${LIB_DIR}/opencv*")

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
        JPEG_LIBRARY_REL NAMES jpeg libjpeg
        PATHS /usr/local/lib /usr/lib
        HINTS "${jpeg_dir_}/stage/lib"
        DOC "Path to jpeg library."
)
FIND_LIBRARY(
        JPEG_LIBRARY_DEB NAMES jpegd libjpegd
        PATHS /usr/local/lib /usr/lib
        HINTS "${jpeg_dir_}/stage/lib"
        DOC "Path to jpeg library."
)
SET(JPEG_LIBRARY optimized ${JPEG_LIBRARY_REL} debug ${JPEG_LIBRARY_DEB})
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
        ZLIB_LIBRARY_REL NAMES z zdll
        PATHS /usr/local/lib /usr/lib
        HINTS "${zlib_dir_}/stage/lib"
        DOC "Path to zlib library."
)
FIND_LIBRARY(
        ZLIB_LIBRARY_DEB NAMES zd zdlld
        PATHS /usr/local/lib /usr/lib
        HINTS "${zlib_dir_}/stage/lib"
        DOC "Path to jpeg library."
)
SET(ZLIB_LIBRARY optimized ${ZLIB_LIBRARY_REL} debug ${ZLIB_LIBRARY_DEB})
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
        PNG_LIBRARY_REL NAMES png libpng
        PATHS /usr/local/lib /usr/lib
        HINTS "${png_dir_1}/stage/lib" "${png_dir_2}/stage/lib"
        DOC "Path to png library."
)
FIND_LIBRARY(
        PNG_LIBRARY_DEB NAMES pngd libpngd
        PATHS /usr/local/lib /usr/lib
        HINTS "${png_dir_1}/stage/lib" "${png_dir_2}/stage/lib"
        DOC "Path to png library."
)
SET(PNG_LIBRARY optimized ${PNG_LIBRARY_REL} debug ${PNG_LIBRARY_DEB})
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
        TIFF_LIBRARY_REL tiff libtiff
        PATHS /usr/local/lib /usr/lib
        HINTS "${tiff_dir_}/stage/lib"
        PATH_SUFFIXES libtiff
        DOC "Path to tiff library."
)
FIND_LIBRARY(
        TIFF_LIBRARY_DEB tiffd libtiffd
        PATHS /usr/local/lib /usr/lib
        HINTS "${tiff_dir_}/stage/lib"
        PATH_SUFFIXES libtiff
        DOC "Path to tiff library."
)
SET(TIFF_LIBRARY optimized ${TIFF_LIBRARY_REL} debug ${TIFF_LIBRARY_DEB})
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

find_package(Boost REQUIRED)
IF (NOT Boost_FOUND)
    MESSAGE(
            FATAL_ERROR
            "Could not find boost headers or libraries.\n"
    )
ENDIF (NOT Boost_FOUND)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARIES})

#=================================== Qt ===================================#

cmake_policy(SET CMP0020 OLD)

find_package(Qt5Core PATHS ${qt_dir_}/qtbase REQUIRED)
find_package(Qt5GUI PATHS ${qt_dir_}/qtbase REQUIRED)
find_package(Qt5Widgets PATHS ${qt_dir_}/qtbase REQUIRED)
find_package(Qt5Xml PATHS ${qt_dir_}/qtbase REQUIRED)
find_package(Qt5Network PATHS ${qt_dir_}/qtbase REQUIRED)
find_package(Qt5OpenGL PATHS ${qt_dir_}/qtbase REQUIRED)
find_package(Qt5LinguistTools PATHS ${qt_dir_}/qtbase REQUIRED)
IF (NOT (Qt5Widgets_FOUND AND Qt5GUI_FOUND AND Qt5Core_FOUND AND Qt5LinguistTools_FOUND
        AND Qt5Xml_FOUND AND Qt5Network_FOUND AND Qt5OpenGL_FOUND))
    MESSAGE(
            FATAL_ERROR "Qt could not be found. "
            "If it's installed in a non-standard location, specify the path to qmake in QT_QMAKE_EXECUTABLE. "
            "You can do it in interactive mode (ccmake instead of cmake) or using -DVAR=VAL syntax."
    )
ENDIF ()

INCLUDE_DIRECTORIES(${Qt5Core_INCLUDE_DIRS})
LINK_DIRECTORIES(${Qt5Core_LIBRARIES})
INCLUDE_DIRECTORIES(${Qt5GUI_INCLUDE_DIRS})
LINK_DIRECTORIES(${Qt5GUI_LIBRARIES})
INCLUDE_DIRECTORIES(${Qt5Widgets_INCLUDE_DIRS})
add_definitions(${Qt5Widgets_DEFINITIONS})
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

find_package(OpenCV PATHS opencv_dir_ REQUIRED)
IF (NOT OpenCV_FOUND)
    MESSAGE(
            FATAL_ERROR
            "Could not find OpenCV headers or libraries.\n"
    )
ENDIF()

INCLUDE_DIRECTORIES(${OpenCV_INCLUDE_DIRS})
LINK_DIRECTORIES(${OpenCV_LIBRARIES})

# Note that ${JPEG_LIBRARY} must go after qjpeg plugin, because otherwise
# the GNU linker won't resolve symbols qjpeg needs from it.
SET(EXTRA_LIBS ${TIFF_LIBRARY} ${PNG_LIBRARY} ${ZLIB_LIBRARY} ${JPEG_LIBRARY})

# Prepare config.h
IF (WIN32)
    SET(TRANSLATIONS_DIR_REL "translations")
ELSE (WIN32)
    SET(TRANSLATIONS_DIR_REL "share/scantailor/translations")
ENDIF (WIN32)
SET(TRANSLATIONS_DIR_ABS "${CMAKE_INSTALL_PREFIX}/${TRANSLATIONS_DIR_REL}")

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

SET(resource_sources)
QT5_ADD_RESOURCES(resource_sources resources/resources.qrc)
SOURCE_GROUP("Generated" FILES ${common_ui_sources} ${gui_only_ui_sources} ${resource_sources})
SOURCE_GROUP("Resources" FILES resources/resources.qrc resources/icons/COPYING)
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
        PhysicalTransformation.cpp PhysicalTransformation.h
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
        OutputFileNameGenerator.cpp OutputFileNameGenerator.h
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
        version.h
        config.h.in
        ${common_ui_files}
)

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
        StartBatchProcessingDialog.cpp StartBatchProcessingDialog.h
        ProjectCreationContext.cpp ProjectCreationContext.h
        ProjectOpeningContext.cpp ProjectOpeningContext.h
        OutOfMemoryDialog.cpp OutOfMemoryDialog.h
        ThumbnailSequence.cpp ThumbnailSequence.h
        ProjectFilesDialog.cpp ProjectFilesDialog.h
        NewOpenProjectPanel.cpp NewOpenProjectPanel.h
        SystemLoadWidget.cpp SystemLoadWidget.h
        MainWindow.cpp MainWindow.h
        main.cpp
)

SET(
        cli_only_sources
        ConsoleBatch.cpp ConsoleBatch.h
        main-cli.cpp
)

SOURCE_GROUP("Sources" FILES ${common_sources} ${gui_only_sources} ${cli_only_sources})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

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

ADD_EXECUTABLE(scantailor-cli ${cli_only_sources} ${common_ui_sources})

TARGET_LINK_LIBRARIES(
        scantailor
        fix_orientation page_split deskew select_content page_layout output stcore
        dewarping zones interaction imageproc math foundation
        ${Qt5Core_LIBRARIES} ${Qt5GUI_LIBRARIES} ${Qt5Widgets_LIBRARIES} ${Qt5Xml_LIBRARIES} ${Qt5Network_LIBRARIES}
        ${Qt5OpenGL_LIBRARIES} ${OpenCV_LIBRARIES} ${Qt5LinguistTools_LIBRARIES} ${EXTRA_LIBS}
)
TARGET_LINK_LIBRARIES(
        scantailor-cli
        fix_orientation page_split deskew select_content page_layout output
        stcore dewarping zones interaction imageproc math foundation
        ${Qt5Core_LIBRARIES} ${Qt5Widgets_LIBRARIES} ${Qt5Xml_LIBRARIES} ${Qt5Network_LIBRARIES} ${OpenCV_LIBRARIES}
        ${Qt5LinguistTools_LIBRARIES} ${EXTRA_LIBS}
)

if (WIN32)
    TARGET_LINK_LIBRARIES(
            scantailor
            wsock32 ws2_32
    )
    TARGET_LINK_LIBRARIES(
            scantailor-cli
            wsock32 ws2_32
    )
endif ()

INSTALL(TARGETS scantailor scantailor-cli RUNTIME DESTINATION bin)

# Translations
TRANSLATION_SOURCES(scantailor ${sources} ${ui_files})
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

INSTALL(FILES ${QM_FILES} DESTINATION "${TRANSLATIONS_DIR_REL}/")

# Tests
ADD_TEST(ImageProcTests imageproc/tests/imageproc_tests --log_level=message)
ADD_TEST(ScanTaylorTests tests/tests --log_level=message)

IF (WIN32)
    # Copy some DLLs to the staging dir.

    SET(
            libs_copy_debug
            "${QT_QTCORE_LIBRARY_DEBUG}"
            "${QT_QTGUI_LIBRARY_DEBUG}"
            "${QT_QTXML_LIBRARY_DEBUG}"
            "${QT_QTNETWORK_LIBRARY_DEBUG}"
    )
    LIB_TO_DLL(libs_copy_debug ${libs_copy_debug})
    COPY_TO_BUILD_DIR(${libs_copy_debug} CONFIGURATIONS Debug)

    SET(
            libs_copy_nondebug
            "${QT_QTCORE_LIBRARY_RELEASE}"
            "${QT_QTGUI_LIBRARY_RELEASE}"
            "${QT_QTXML_LIBRARY_RELEASE}"
            "${QT_QTNETWORK_LIBRARY_RELEASE}"
    )
    LIB_TO_DLL(libs_copy_nondebug ${libs_copy_nondebug})
    COPY_TO_BUILD_DIR(${libs_copy_nondebug} CONFIGURATIONS Release MinSizeRel RelWithDebInfo)

    # Qt's imageformat plugins. Note that png is built-in and for tiff we have our own code.
    COPY_TO_BUILD_DIR("${QT_QJPEG_PLUGIN_RELEASE}" SUBDIR imageformats CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
    COPY_TO_BUILD_DIR("${QT_QJPEG_PLUGIN_DEBUG}" SUBDIR imageformats CONFIGURATIONS Debug)

    FIND_LIBRARY(
            JPEG_LIBRARY_RT_REL NAMES jpeg libjpeg
            HINTS "${jpeg_dir_}/bin"
            DOC "Path to jpeg runtime library."
    )
    FIND_LIBRARY(
            JPEG_LIBRARY_RT_DEB NAMES jpegd libjpegd
            HINTS "${jpeg_dir_}/bin"
            DOC "Path to jpeg runtime library."
    )
    SET(JPEG_LIBRARY_RT optimized ${JPEG_LIBRARY_RT_REL} debug ${JPEG_LIBRARY_RT_DEB})
    FIND_LIBRARY(
            ZLIB_LIBRARY_RT_REL NAMES z zdll
            HINTS "${zlib_dir_}/bin"
            DOC "Path to zlib runtime library."
    )
    FIND_LIBRARY(
            ZLIB_LIBRARY_RT_DEB NAMES zd zdlld
            HINTS "${zlib_dir_}/bin"
            DOC "Path to zlib runtime library."
    )
    SET(ZLIB_LIBRARY_RT optimized ${ZLIB_LIBRARY_RT_REL} debug ${ZLIB_LIBRARY_RT_DEB})
    FIND_LIBRARY(
            PNG_LIBRARY_RT_REL NAMES png libpng
            HINTS "${png_dir_1}/bin" "${png_dir_2}/bin"
            DOC "Path to png runtime library."
    )
    FIND_LIBRARY(
            PNG_LIBRARY_RT_DEB NAMES pngd libpngd
            HINTS "${png_dir_1}/bin" "${png_dir_2}/bin"
            DOC "Path to png runtime library."
    )
    SET(PNG_LIBRARY_RT optimized ${PNG_LIBRARY_RT_REL} debug ${PNG_LIBRARY_RT_DEB})
    FIND_LIBRARY(
            TIFF_LIBRARY_RT_REL NAMES tiff libtiff
            HINTS "${tiff_dir_}/bin"
            DOC "Path to tiff runtime library."
    )
    FIND_LIBRARY(
            TIFF_LIBRARY_RT_DEB NAMES tiffd libtiffd
            HINTS "${tiff_dir_}/bin"
            DOC "Path to tiff runtime library."
    )
    SET(TIFF_LIBRARY_RT optimized ${TIFF_LIBRARY_RT_REL} debug ${TIFF_LIBRARY_RT_DEB})
    IF (NOT (JPEG_LIBRARY AND ZLIB_LIBRARY_RT AND PNG_LIBRARY_RT AND TIFF_LIBRARY_RT))
        MESSAGE(
                FATAL_ERROR
                "Could not find required RT libraries.\n"
        )
    ENDIF ()
    SET(
            libs_copy_all_confs
            "${JPEG_LIBRARY_RT}" "${ZLIB_LIBRARY_RT}" "${PNG_LIBRARY_RT}" "${TIFF_LIBRARY_RT}"
    )
    COPY_TO_BUILD_DIR(${libs_copy_all_confs})

    # Generate the target that will actually do the copying.
    GENERATE_COPY_TO_BUILD_DIR_TARGET(copy_to_build_dir)
ENDIF (WIN32)
