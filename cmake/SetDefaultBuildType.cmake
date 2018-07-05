macro (st_set_default_build_type TYPE_)
  if (NOT CMAKE_BUILD_TYPE AND NOT DEFAULT_BUILD_TYPE_SET)
    set(DEFAULT_BUILD_TYPE_SET TRUE CACHE INTERNAL "" FORCE)
    set(
        CMAKE_BUILD_TYPE "${TYPE_}" CACHE STRING
        "Build type (Release Debug RelWithDebInfo MinSizeRel)" FORCE
    )
  endif()
endmacro()
