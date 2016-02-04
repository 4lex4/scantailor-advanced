# This macro exists because FILE(TO_NATIVE_PATH ...) is broken on MinGW.
MACRO(TO_NATIVE_PATH PATH OUT)
    FILE(TO_NATIVE_PATH "${PATH}" "${OUT}")
    IF (MINGW)
        STRING(REPLACE "/" "\\" "${OUT}" "${${OUT}}")
    ENDIF (MINGW)
ENDMACRO(TO_NATIVE_PATH)
