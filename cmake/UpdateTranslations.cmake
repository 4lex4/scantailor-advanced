# translation_sources(<target> <source files>)
#
# Associates the specified source files to translate with a target.
# This macro may be called multiple times, possibly from different directories.
# The typical usage will be like this:
#
# translation_sources(target MainWindow.cpp MainWindow.h MainWindow.ui ...)
# finalize_translations(target myapp_de.ts myapp_ru.ts myapp_ja.ts ...)
# update_translations_target(update_translations target)
#
macro (translation_sources _target) #, _sources
  file(GLOB _sources ABSOLUTE ${ARGN})
  list(APPEND ${_target}_TRANSLATION_SOURCES ${_sources})
  list(APPEND ${_target}_TRANSLATION_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
  list(REMOVE_DUPLICATES ${_target}_TRANSLATION_SOURCES)
  list(REMOVE_DUPLICATES ${_target}_TRANSLATION_INCLUDE_DIRS)
  set(${_target}_TRANSLATION_SOURCES ${${_target}_TRANSLATION_SOURCES} CACHE STRING "" FORCE)
  set(${_target}_TRANSLATION_INCLUDE_DIRS ${${_target}_TRANSLATION_INCLUDE_DIRS} CACHE STRING "" FORCE)
endmacro()


# finalize_translations(<target>, <*.ts files>)
#
# Associates *.ts files with a target.
# May be called multiple times for different targets.
# To be followed by update_translations_target()
#
macro (finalize_translations _target) #, _ts_files
  set(_sources_str "")
  foreach (_file ${${_target}_TRANSLATION_SOURCES})
    set(_sources_str "${_sources_str} \"${_file}\"")
  endforeach()

  set(_filtered_inc_dirs "")
  foreach (_dir ${${_target}_TRANSLATION_INCLUDE_DIRS})
    # We are going to accept include directories within our
    # source and binary trees and reject all others.  Allowing lupdate
    # to parse things like boost headers leads to spurious warnings.
    file(RELATIVE_PATH _dir_rel_to_source "${CMAKE_SOURCE_DIR}" "${_dir}")
    file(RELATIVE_PATH _dir_rel_to_binary "${CMAKE_BINARY_DIR}" "${_dir}")
    if (NOT _dir_rel_to_source MATCHES "\\.\\..*")
      list(APPEND _filtered_inc_dirs "${_dir}")
    elseif (NOT _dir_rel_to_binary MATCHES "\\.\\..*")
      list(APPEND _filtered_inc_dirs "${_dir}")
    endif()
  endforeach()
  set(_inc_dirs_str "")
  foreach (_dir ${_filtered_inc_dirs})
    set(_inc_dirs_str "${_inc_dirs_str} \"${_dir}\"")
  endforeach()

  set(_translations_str "")
  foreach (_file ${ARGN})
    get_filename_component(_abs "${_file}" ABSOLUTE)
    set(_translations_str "${_translations_str} \"${_abs}\"")
  endforeach()

  file(
      WRITE "${CMAKE_BINARY_DIR}/update_translations_${_target}.pro"
      "SOURCES = ${_sources_str}\nTRANSLATIONS = ${_translations_str}\nINCLUDEPATH = ${_inc_dirs_str}")

  # Note that we can't create a custom target with *.ts files as output, because:
  # 1. CMake would pollute our source tree with *.rule fules.
  # 2. "make clean" would remove them.
endmacro()


# update_translations_target(<_update_target> <targets>)
#
# Creates a target that updates *.ts files assiciated with the specified
# targets to translate by finalize_translation_set()
#
macro (update_translations_target _update_target) #, _targets
  set(_commands "")
  foreach (_target ${ARGN})
    list(
        APPEND _commands COMMAND Qt5::lupdate -locations absolute -no-obsolete
        -pro "${CMAKE_BINARY_DIR}/update_translations_${_target}.pro")
  endforeach()

  add_custom_target(${_update_target} ${_commands} VERBATIM)
endmacro()
