cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(
        FATAL_ERROR
        "In-source builds are not supported. Use a checked-in CMake preset so generated files stay below build/."
    )
endif()

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
    TempoFlowClickEngine
    STATIC
        src/audio/ClickEngine.cpp
        src/audio/ClickEngine.h
)

target_include_directories(
    TempoFlowClickEngine
    PUBLIC
        src
)

target_compile_features(
    TempoFlowClickEngine
    PUBLIC
        cxx_std_17
)

target_link_libraries(
    TempoFlowClickEngine
    PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_library(
    TempoFlowHostTiming
    STATIC
        src/timing/BeatScheduler.cpp
        src/timing/BeatScheduler.h
)

target_include_directories(
    TempoFlowHostTiming
    PUBLIC
        src
)

target_compile_features(
    TempoFlowHostTiming
    PUBLIC
        cxx_std_17
)

target_link_libraries(
    TempoFlowHostTiming
    PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

juce_add_plugin(
    TempoFlowPlugin
    COMPANY_NAME "jfheinrich"
    BUNDLE_ID "eu.jfheinrich.tempoflow"
    PLUGIN_MANUFACTURER_CODE Jfhe
    PLUGIN_CODE Tflo
    FORMATS VST3
    PRODUCT_NAME "TempoFlow"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE
)

target_sources(
    TempoFlowPlugin
    PRIVATE
        src/plugin/PluginProcessor.cpp
        src/plugin/PluginProcessor.h
)

target_include_directories(
    TempoFlowPlugin
    PUBLIC
        src
)

target_compile_features(
    TempoFlowPlugin
    PUBLIC
        cxx_std_17
)

target_compile_definitions(
    TempoFlowPlugin
    PUBLIC
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
)

target_link_libraries(
    TempoFlowPlugin
    PUBLIC
        juce::juce_audio_processors_headless
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    PRIVATE
        TempoFlowClickEngine
        TempoFlowHostTiming
)

add_library(
    TempoFlowPresetValidation
    STATIC
        src/preset/PresetFileIO.cpp
        src/preset/PresetFileIO.h
        src/preset/PresetRuntimeModel.cpp
        src/preset/PresetRuntimeModel.h
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

juce_add_console_app(
    TempoFlowPluginTests
    PRODUCT_NAME "TempoFlow Plugin Tests"
)

target_sources(
    TempoFlowPluginTests
    PRIVATE
        tests/plugin_processor_tests.cpp
)

target_link_libraries(
    TempoFlowPluginTests
    PRIVATE
        TempoFlowPlugin
)

add_executable(
    TempoFlowHostTimingTests
    tests/host_timing_tests.cpp
)

target_link_libraries(
    TempoFlowHostTimingTests
    PRIVATE
        TempoFlowHostTiming
)

add_executable(
    TempoFlowClickEngineTests
    tests/click_engine_tests.cpp
)

target_link_libraries(
    TempoFlowClickEngineTests
    PRIVATE
        TempoFlowClickEngine
)

add_executable(
    TempoFlowPresetRuntimeModelTests
    tests/preset_runtime_model_tests.cpp
)

target_link_libraries(
    TempoFlowPresetRuntimeModelTests
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

    target_link_options(
        TempoFlowPluginTests
        PRIVATE
            $<$<CONFIG:Debug>:/PROFILE>
            $<$<CONFIG:Debug>:/INCREMENTAL:NO>
    )

    target_link_options(
        TempoFlowHostTimingTests
        PRIVATE
            $<$<CONFIG:Debug>:/PROFILE>
            $<$<CONFIG:Debug>:/INCREMENTAL:NO>
    )

    target_link_options(
        TempoFlowClickEngineTests
        PRIVATE
            $<$<CONFIG:Debug>:/PROFILE>
            $<$<CONFIG:Debug>:/INCREMENTAL:NO>
    )

    target_link_options(
        TempoFlowPresetRuntimeModelTests
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

    add_test(
        NAME plugin-processor-unit-tests
        COMMAND TempoFlowPluginTests
    )

    add_test(
        NAME host-timing-unit-tests
        COMMAND TempoFlowHostTimingTests
    )

    add_test(
        NAME click-engine-unit-tests
        COMMAND TempoFlowClickEngineTests
    )

    add_test(
        NAME preset-runtime-model-unit-tests
        COMMAND TempoFlowPresetRuntimeModelTests
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
