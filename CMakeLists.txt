cmake_minimum_required(VERSION 3.22)

project(
    TempoFlow
    VERSION 0.1.0
    LANGUAGES C CXX
)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.2
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(JUCE)

add_library(
    TempoFlowPresetValidation
    STATIC
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
endif()
