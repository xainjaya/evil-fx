cmake_minimum_required(VERSION 3.22)
project(EvilFX VERSION 1.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

option(EVIL_BUILD_TESTS "Build the headless DSP test + UI snapshot tool" ON)
option(EVIL_BUILD_CLAP "Also build a CLAP plugin (via clap-juce-extensions)" ON)
option(EVIL_MINIMAL_LINUX_DEPS "Linux: build without ALSA/Xrandr/Xinerama/Xcursor headers (CI/containers)" OFF)

set(EVIL_LINUX_MINIMAL_FLAGS -DJUCE_ALSA=0 -DJUCE_JACK=0 -DJUCE_USE_XRANDR=0 -DJUCE_USE_XINERAMA=0
                             -DJUCE_USE_XCURSOR=0 -DJUCE_USE_XRENDER=0)
if(EVIL_MINIMAL_LINUX_DEPS)
    # JUCE's helper tool (juceaide) is configured in a nested CMake run that inherits the environment
    list(JOIN EVIL_LINUX_MINIMAL_FLAGS " " _evil_flags)
    set(ENV{CXXFLAGS} "$ENV{CXXFLAGS} ${_evil_flags}")
endif()

# ---- JUCE: use ./JUCE if present (git submodule), otherwise download it -------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(EVIL_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND EVIL_FORMATS AU)
endif()
if(UNIX AND NOT APPLE)
    list(APPEND EVIL_FORMATS LV2)
endif()

juce_add_plugin(EvilFX
    COMPANY_NAME              "Xain Jaya"
    COMPANY_WEBSITE           "https://xainjaya.com"
    BUNDLE_ID                 com.xainjaya.evilfx
    PRODUCT_NAME              "Evil FX"
    ICON_BIG                  "${CMAKE_CURRENT_SOURCE_DIR}/docs/icon.png"
    PLUGIN_MANUFACTURER_CODE  Xain
    PLUGIN_CODE               EvFx
    FORMATS                   ${EVIL_FORMATS}
    LV2URI                    "https://xainjaya.com/plugins/evilfx"
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    VST3_CATEGORIES           Fx
    AU_MAIN_TYPE              kAudioUnitType_Effect)

# ---- CLAP: same plugin, wrapped by clap-juce-extensions ---------------------------
if(EVIL_BUILD_CLAP)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/clap-juce-extensions/CMakeLists.txt")
        add_subdirectory(clap-juce-extensions EXCLUDE_FROM_ALL)
    else()
        include(FetchContent)
        FetchContent_Declare(clap-juce-extensions
            GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
            GIT_TAG        7adee3a1bd4684d4caa5601100e364abccff4b4d
            GIT_SUBMODULES_RECURSE TRUE)
        FetchContent_GetProperties(clap-juce-extensions)
        if(NOT clap-juce-extensions_POPULATED)
            FetchContent_Populate(clap-juce-extensions)
            add_subdirectory(${clap-juce-extensions_SOURCE_DIR} ${clap-juce-extensions_BINARY_DIR} EXCLUDE_FROM_ALL)
        endif()
    endif()
    clap_juce_extensions_plugin(TARGET EvilFX
        CLAP_ID "com.xainjaya.evilfx"
        CLAP_FEATURES audio-effect multi-effects distortion delay reverb stereo)
endif()

set(EVIL_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_sources(EvilFX PRIVATE ${EVIL_SOURCES})

set(EVIL_DEFS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
if(EVIL_MINIMAL_LINUX_DEPS)
    list(APPEND EVIL_DEFS JUCE_ALSA=0 JUCE_JACK=0 JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0
                          JUCE_USE_XCURSOR=0 JUCE_USE_XRENDER=0)
endif()

target_compile_definitions(EvilFX PUBLIC ${EVIL_DEFS} EVIL_VERSION="${PROJECT_VERSION}")

target_link_libraries(EvilFX
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---- headless tests: DSP stress test + renders the UI to a PNG ---------------------
if(EVIL_BUILD_TESTS)
    juce_add_console_app(EvilFXTests PRODUCT_NAME "EvilFXTests")
    target_sources(EvilFXTests PRIVATE Tests/HeadlessTests.cpp ${EVIL_SOURCES})
    target_include_directories(EvilFXTests PRIVATE Source)
    target_compile_definitions(EvilFXTests PRIVATE ${EVIL_DEFS} EVIL_VERSION="${PROJECT_VERSION}"
        JUCE_STANDALONE_APPLICATION=1 JucePlugin_Name="Evil FX")
    target_link_libraries(EvilFXTests PRIVATE
        juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
