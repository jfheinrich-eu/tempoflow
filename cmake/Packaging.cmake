if(NOT WIN32)
    return()
endif()

include(CPackComponent)

set(
    TEMPOFLOW_WINDOWS_VST3_BUNDLE
    "${CMAKE_CURRENT_BINARY_DIR}/TempoFlowPlugin_artefacts/$<CONFIG>/VST3/TempoFlow.vst3"
)

install(
    DIRECTORY "${TEMPOFLOW_WINDOWS_VST3_BUNDLE}"
    DESTINATION "."
    COMPONENT VST3
)

install(
    FILES ${TEMPOFLOW_FACTORY_PRESET_OUTPUTS}
    DESTINATION "."
    COMPONENT FactoryPresets
)

install(
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
        "${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
    DESTINATION "."
    COMPONENT Documentation
)

install(
    FILES "${juce_SOURCE_DIR}/LICENSE.md"
    DESTINATION "."
    RENAME "JUCE-LICENSE.md"
    COMPONENT Documentation
)

install(
    FILES "${juce_SOURCE_DIR}/JUCE.spdx.json"
    DESTINATION "."
    RENAME "JUCE-SPDX.json"
    COMPONENT Documentation
)

install(
    FILES "${TEMPOFLOW_VST3_SDK_DIR}/LICENSE.txt"
    DESTINATION "."
    RENAME "VST3-SDK-LICENSE.txt"
    COMPONENT Documentation
)

cpack_add_component(
    VST3
    DISPLAY_NAME "TempoFlow VST3"
    DESCRIPTION "TempoFlow 64-bit VST3 plug-in"
    REQUIRED
)

cpack_add_component(
    FactoryPresets
    DISPLAY_NAME "TempoFlow factory presets"
    DESCRIPTION "Seven native TempoFlow VST3 factory presets"
    REQUIRED
)

cpack_add_component(
    Documentation
    DISPLAY_NAME "Licenses and notices"
    DESCRIPTION "TempoFlow and third-party license information"
    REQUIRED
)

set(CPACK_GENERATOR "NSIS64")
set(CPACK_PACKAGE_NAME "TempoFlow")
set(CPACK_PACKAGE_VENDOR "jfheinrich")
set(CPACK_PACKAGE_CONTACT "contact@jfheinrich.eu")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Sample-accurate VST3 metronome")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://jfheinrich-eu.github.io/tempoflow/")
set(TEMPOFLOW_WINDOWS_PACKAGE_BASENAME "TempoFlow-${PROJECT_VERSION}-windows-x64")
set(CPACK_PACKAGE_FILE_NAME "${TEMPOFLOW_WINDOWS_PACKAGE_BASENAME}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "TempoFlow")
set(CPACK_PACKAGE_CHECKSUM "SHA256")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_COMPONENTS_ALL VST3 FactoryPresets Documentation)

set(CPACK_NSIS_BRANDING_TEXT "TempoFlow by ${CPACK_PACKAGE_VENDOR}")
set(CPACK_NSIS_BRANDING_TEXT_TRIM_POSITION RIGHT)
set(
    CPACK_NSIS_INSTALLER_MUI_ICON_CODE
    "VIProductVersion ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.0
VIFileVersion ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.0
VIAddVersionKey /LANG=0 \\\"ProductName\\\" \\\"${CPACK_PACKAGE_NAME}\\\"
VIAddVersionKey /LANG=0 \\\"ProductVersion\\\" \\\"${PROJECT_VERSION}\\\"
VIAddVersionKey /LANG=0 \\\"CompanyName\\\" \\\"${CPACK_PACKAGE_VENDOR}\\\"
VIAddVersionKey /LANG=0 \\\"FileDescription\\\" \\\"${CPACK_PACKAGE_NAME} Windows Installer\\\"
VIAddVersionKey /LANG=0 \\\"FileVersion\\\" \\\"${PROJECT_VERSION}\\\"
VIAddVersionKey /LANG=0 \\\"OriginalFilename\\\" \\\"${CPACK_PACKAGE_FILE_NAME}.exe\\\""
)

set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
set(CPACK_NSIS_DISPLAY_NAME "TempoFlow ${PROJECT_VERSION}")
set(CPACK_NSIS_PACKAGE_NAME "TempoFlow ${PROJECT_VERSION}")
set(CPACK_NSIS_HELP_LINK "https://github.com/jfheinrich-eu/tempoflow/issues")
set(CPACK_NSIS_URL_INFO_ABOUT "https://jfheinrich-eu.github.io/tempoflow/")
set(CPACK_NSIS_CONTACT "contact@jfheinrich.eu")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
set(CPACK_NSIS_VST3_INSTALL_DIRECTORY "$COMMONFILES64\\\\VST3")
set(
    CPACK_NSIS_FactoryPresets_INSTALL_DIRECTORY
    "$COMMONPROGRAMDATA\\\\VST3 Presets\\\\jfheinrich\\\\TempoFlow"
)

file(
    GENERATE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/TempoFlowPackageName.txt"
    CONTENT "${TEMPOFLOW_WINDOWS_PACKAGE_BASENAME}"
)

include(CPack)
