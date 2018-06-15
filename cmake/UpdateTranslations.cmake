# translation_sources(<translation set> <source files>)
#
# Associates the specified source files with a translation set.  A translation
# set corresponds to a family of *.ts files with the same prefix, one for each
# translation.  Usually translation sets also correspond to build targets,
# though they don't have to.  You may use the target name as a translation set
# name if you want to.  Translation set names can only contain characters
# allowed in CMake variable names.
# This macro may be called multiple times, possibly from different directories.
# The typical usage will be like this:
#
# translation_sources(myapp MainWindow.cpp MainWindow.h MainWindow.ui ...)
# finalize_translation_set(myapp myapp_de.ts myapp_ru.ts myapp_ja.ts ...)
# update_translations_target(update_translations myapp)
#
macro (translation_sources _set) #, _sources
  file(GLOB _sources ABSOLUTE ${ARGN})
  list(APPEND ${_set}_SOURCES ${_sources})

  get_directory_property(_inc_dirs INCLUDE_DIRECTORIES)
  file(GLOB _inc_dirs ${_inc_dirs} .)
  list(APPEND ${_set}_INC_DIRS ${_inc_dirs})

  # If there is a parent scope, set these variables there as well.
  get_directory_property(_parent_dir PARENT_DIRECTORY)
  if (_parent_dir)
    set(${_set}_SOURCES ${${_set}_SOURCES} PARENT_SCOPE)
    set(${_set}_INC_DIRS ${${_set}_INC_DIRS} PARENT_SCOPE)
  endif()
endmacro()


# finalize_translation_set(<translation set>, <*.ts files>)
#
# Associates *.ts files with a translation set.
# May be called multiple times for different translation sets.
# To be followed by update_translations_target()
#
macro (finalize_translation_set _set) #, _ts_files
  set(_sources_str "")
  foreach (_file ${${_set}_SOURCES})
    set(_sources_str "${_sources_str} \"${_file}\"")
  endforeach()

  set(_inc_dirs ${${_set}_INC_DIRS})
  list(REMOVE_DUPLICATES _inc_dirs)

  set(_filtered_inc_dirs "")
  foreach (_dir ${_inc_dirs})
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
      WRITE "${CMAKE_BINARY_DIR}/update_translations_${_set}.pro"
      "SOURCES = ${_sources_str}\nTRANSLATIONS = ${_translations_str}\nINCLUDEPATH = ${_inc_dirs_str}"
  )

  # Note that we can't create a custom target with *.ts files as output, because:
  # 1. CMake would pollute our source tree with *.rule fules.
  # 2. "make clean" would remove them.
endmacro()


# update_translations_target(<target> <translation sets>)
#
# Creates a target that updates *.ts files assiciated with the specified
# translation sets by finalize_translation_set()
#
macro (update_translations_target _target) #, _sets
  set(_commands "")
  foreach (_set ${ARGN})
    list(
        APPEND _commands COMMAND Qt5::lupdate -locations absolute
        -pro "${CMAKE_BINARY_DIR}/update_translations_${_set}.pro"
    )
  endforeach()

  add_custom_target(${_target} ${_commands} VERBATIM)
endmacro()
