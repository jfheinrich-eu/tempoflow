cmake_minimum_required(VERSION 3.25)

project(
    TempoFlow
    VERSION 0.1.0
    LANGUAGES C CXX
)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 72782788ce18c2d4d760b28e0921d6ffc6431102 # 9.0.2
)

FetchContent_MakeAvailable(JUCE)

add_library(
    TempoFlowPresetValidation
    STATIC
        src/preset/PresetFileIO.cpp
        src/preset/PresetFileIO.h
        src/preset/PresetValidator.cpp
        src/preset/PresetValidator.h
)

target_include_directories(
    TempoFlowPresetValidation
    PUBLIC
        src
)

target_compile_features(
    TempoFlowPresetValidation
    PUBLIC
        cxx_std_17
)

target_compile_definitions(
    TempoFlowPresetValidation
    PUBLIC
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
)

target_link_libraries(
    TempoFlowPresetValidation
    PUBLIC
        juce::juce_core
)

juce_add_console_app(
    TempoFlowToolchainCheck
    PRODUCT_NAME "TempoFlow Toolchain Check"
)

target_sources(
    TempoFlowToolchainCheck
    PRIVATE
        tools/toolchain_check.cpp
)

target_compile_features(
    TempoFlowToolchainCheck
    PRIVATE
        cxx_std_17
)

target_compile_definitions(
    TempoFlowToolchainCheck
    PRIVATE
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
)

target_link_libraries(
    TempoFlowToolchainCheck
    PRIVATE
        juce::juce_core
)

juce_add_console_app(
    TempoFlowPresetValidator
    PRODUCT_NAME "TempoFlow Preset Validator"
)

target_sources(
    TempoFlowPresetValidator
    PRIVATE
        tools/preset_validator.cpp
)

target_link_libraries(
    TempoFlowPresetValidator
    PRIVATE
        TempoFlowPresetValidation
)

juce_add_console_app(
    TempoFlowPresetValidatorTests
    PRODUCT_NAME "TempoFlow Preset Validator Tests"
)

target_sources(
    TempoFlowPresetValidatorTests
    PRIVATE
        tests/preset_validator_tests.cpp
)

target_link_libraries(
    TempoFlowPresetValidatorTests
    PRIVATE
        TempoFlowPresetValidation
)

if(MSVC)
    target_link_options(
        TempoFlowPresetValidatorTests
        PRIVATE
            $<$<CONFIG:Debug>:/PROFILE>
            $<$<CONFIG:Debug>:/INCREMENTAL:NO>
    )
endif()

include(CTest)

if(BUILD_TESTING)
    add_test(
        NAME preset-validator-unit-tests
        COMMAND TempoFlowPresetValidatorTests
    )

    add_test(
        NAME preset-reference-files
        COMMAND
            TempoFlowPresetValidator
            "${CMAKE_CURRENT_SOURCE_DIR}/docs/Preset Format 1.0/presets"
    )

    if(WIN32)
        find_program(TEMPOFLOW_POWERSHELL_EXECUTABLE NAMES pwsh powershell REQUIRED)
        add_test(
            NAME preset-reparse-point
            COMMAND
                "${TEMPOFLOW_POWERSHELL_EXECUTABLE}"
                -NoLogo
                -NoProfile
                -ExecutionPolicy Bypass
                -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/preset-reparse-point.tests.ps1"
                -ValidatorPath "$<TARGET_FILE:TempoFlowPresetValidator>"
        )
    endif()
endif()
