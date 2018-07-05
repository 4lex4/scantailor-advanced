# Reset variables.
foreach (conf_ Debug Release MinSizeRel RelWithDebInfo)
  set(
      "COPY_TO_BUILD_DIR_${conf_}" "" CACHE INTERNAL
      "Files to copy to ${conf_} build directory" FORCE
  )
endforeach()

# Usage:
# copy_to_build_dir(one or more files [SUBDIR subdir] [CONFIGURATIONS] conf1 conf2 ...)
macro (copy_to_build_dir)
  set(files_ "")
  set(confs_ "Debug;Release;MinSizeRel;RelWithDebInfo")
  set(subdir_ "")
  set(out_list_ "files_")
  foreach (arg_ ${ARGV})
    if ("${arg_}" STREQUAL "SUBDIR")
      set(out_list_ "subdir_")
    elseif ("${arg_}" STREQUAL "CONFIGURATIONS")
      set(out_list_ "confs_")
      set(confs_ "")
    else()
      list(APPEND ${out_list_} "${arg_}")
    endif()
  endforeach()

  list(LENGTH subdir_ num_subdirs_)
  if ("${num_subdirs_}" GREATER 1)
    message(FATAL_ERROR "Multiple sub-directories aren't allowed!")
  endif()

  foreach (conf_ ${confs_})
    foreach (file_ ${files_})
      if (EXISTS "${file_}")
        if ("${subdir_}" STREQUAL "")
          list(APPEND "COPY_TO_BUILD_DIR_${conf_}" "${file_}")
        else()
          list(APPEND "COPY_TO_BUILD_DIR_${conf_}" "${file_}=>${subdir_}")
        endif()
      endif()
    endforeach()

    # Force the new value to be written to the cache.
    set(
        "COPY_TO_BUILD_DIR_${conf_}" ${COPY_TO_BUILD_DIR_${conf_}}
        CACHE INTERNAL "Files to copy to ${conf_} build directory" FORCE
    )
  endforeach()
endmacro()


macro (generate_copy_to_build_dir_target target_name_)
  set(script_ "${CMAKE_BINARY_DIR}/copy_to_build_dir.cmake")
  configure_file("cmake/copy_to_build_dir.cmake.in" "${script_}" @ONLY)

  set(
      src_files_
      ${COPY_TO_BUILD_DIR_Debug} ${COPY_TO_BUILD_DIR_Release}
      ${COPY_TO_BUILD_DIR_MinSizeRel} ${COPY_TO_BUILD_DIR_RelWithDebInfo}
  )
  set(deps_ "")
  foreach (src_file_ ${src_files_})
    string(REGEX REPLACE "(.*)=>.*" "\\1" src_file_ "${src_file_}")
    list(APPEND deps_ "${src_file_}")
  endforeach()

  # Copy DLLs and other stuff to ${CMAKE_BINARY_DIR}/<configuration>
  add_custom_target(
      "${target_name_}" ALL
      COMMAND "${CMAKE_COMMAND}" "-DTARGET_DIR=$<TARGET_FILE_DIR:scantailor>"
      "-DCFG=$<CONFIG>" -P "${script_}"
      DEPENDS "${script_}" ${deps_}
  )
endmacro()