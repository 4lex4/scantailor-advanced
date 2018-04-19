macro (FindPthreads)
    set(PTHREADS_FOUND FALSE)

    # This won't overwrite values already in cache.
    set(PTHREADS_CFLAGS "" CACHE STRING "Compiler flags for pthreads")
    set(PTHREADS_LIBS "" CACHE STRING "Linker flags for pthreads")
    mark_as_advanced(CLEAR PTHREADS_CFLAGS PTHREADS_LIBS)

    set(_available_flags "")

    if (PTHREADS_CFLAGS OR PTHREADS_LIBS)
        # First try user specified flags.
        list(APPEND _available_flags "${PTHREADS_CFLAGS}:${PTHREADS_LIBS}")
    endif()

    # -pthreads for gcc, -lpthread for Sun's compiler.
    # Note that there are non-functional stubs of pthread functions
    # in Solaris' libc, so these checks must be done before others.
    set(_solaris_flags "-pthreads:-pthreads" "-D_REENTRANT:-lpthread")

    # No flags required.  This means this check has to be the first
    # on Darwin / Mac OS X, because the compiler will accept almost
    # any flag.
    set(_darwin_flags ":")

    # Must be checked before -lpthread on AIX.
    set(_aix_flags "-D_THREAD_SAFE:-lpthreads")

    # gcc on various OSes
    set(_other_flags "-pthread:-pthread")

    if (CMAKE_SYSTEM_NAME MATCHES "AIX.*")
        list(APPEND _available_flags ${_aix_flags})
        set(_aix_flags "")
    elseif (CMAKE_SYSTEM_NAME MATCHES "Solaris.*")
        list(APPEND _available_flags ${_solaris_flags})
        set(_solaris_flags "")
    elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin.*")
        list(APPEND _available_flags ${_darwin_flags})
        set(_darwin_flags "")
    else()
        list(APPEND _available_flags ${_other_flags})
        set(_other_flags "")
    endif()

    list(
            APPEND _available_flags
            ${_darwin_flags} ${_aix_flags} ${_solaris_flags} ${_other_flags}
    )

    list(LENGTH _available_flags _num_available_flags)
    set(_flags_idx 0)
    while (_flags_idx LESS _num_available_flags AND NOT PTHREADS_FOUND)
        list(GET _available_flags ${_flags_idx} _flag)
        math(EXPR _flags_idx "${_flags_idx} + 1")

        string(REGEX REPLACE ":.*" "" _cflags "${_flag}")
        string(REGEX REPLACE ".*:" "" _libs "${_flag}")

        file(WRITE ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/TestPthreads.c
                "#include <pthread.h>\n"
                "int main()\n"
                "{\n"
                "    pthread_t th;\n"
                "    pthread_create(&th, 0, 0, 0);\n"
                "    pthread_join(th, 0);\n"
                "    pthread_attr_init(0);\n"
                "    pthread_cleanup_push(0, 0);\n"
                "    pthread_cleanup_pop(0);\n"
                "   return 0;\n"
                "}\n"
                )

        try_compile(
                PTHREADS_FOUND "${CMAKE_BINARY_DIR}"
                ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/TestPthreads.c
                CMAKE_FLAGS "-DLINK_LIBRARIES:STRING=${_libs}"
                COMPILE_DEFINITIONS "${_cflags}"
                OUTPUT_VARIABLE _out
        )
        if (PTHREADS_FOUND)
            message(STATUS "Checking pthreads with CFLAGS=\"${_cflags}\" and LIBS=\"${_libs}\" -- yes")
            set(PTHREADS_CFLAGS ${_cflags} CACHE STRING "Compiler flags for pthreads" FORCE)
            set(PTHREADS_LIBS ${_libs} CACHE STRING "Linker flags for pthreads" FORCE)
            mark_as_advanced(FORCE PTHREADS_CFLAGS PTHREADS_LIBS)
        else()
            message(STATUS "Checking pthreads with CFLAGS=\"${_cflags}\" and LIBS=\"${_libs}\" -- no")
            file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
                    "Pthreads don't work with CFLAGS=\"${_cflags}\" and LIBS=\"${_libs}\"\n"
                    "Build output follows:\n"
                    "==========================================\n"
                    "${_out}\n"
                    "==========================================\n"
                    )
        endif()
    endwhile()

endmacro()
