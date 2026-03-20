set(cli_sources
    main.cpp
    CliConfig.cpp CliConfig.h
    CliPageSelectionProvider.cpp CliPageSelectionProvider.h
    NoOpFilterUiInterface.cpp NoOpFilterUiInterface.h
    ProjectGeneratorFromDir.cpp ProjectGeneratorFromDir.h)

set(cli_resource_files
    "${SCANTAILOR_RESOURCES_DIR}/resources.qrc"
    "${SCANTAILOR_RESOURCES_DIR}/light_scheme/light_scheme.qrc")

add_executable(scantailor-cli ${cli_sources} ${cli_resource_files})
set_target_properties(scantailor-cli PROPERTIES AUTORCC ON)
target_link_libraries(scantailor-cli PRIVATE core)
target_include_directories(scantailor-cli PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/src/core" "${CMAKE_BINARY_DIR}")
install(TARGETS scantailor-cli RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
