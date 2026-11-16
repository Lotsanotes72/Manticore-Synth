cmake_minimum_required(VERSION 3.22)
project(Manticore VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Static MSVC runtime: the plugin works without installing the Visual C++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(MC_BUILD_TESTS "Build the offline test / preset render / screenshot app" OFF)

# JUCE: use a local checkout if JUCE_PATH is given, otherwise download it.
if(DEFINED JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.10
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_binary_data(ManticoreData SOURCES
    Resources/Fonts/Cinzel-Bold.ttf
    Resources/Fonts/Rajdhani-Medium.ttf
    Resources/Fonts/Rajdhani-SemiBold.ttf
    Resources/Fonts/Rajdhani-Bold.ttf
    Resources/factory_presets.json.gz)

set(MC_SOURCES
    Source/Params.cpp
    Source/Presets.cpp
    Source/dsp/Wavetables.cpp
    Source/dsp/VoiceModules.cpp
    Source/dsp/Voice.cpp
    Source/dsp/Effects.cpp
    Source/dsp/Engine.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/ui/LookAndFeel.cpp
    Source/ui/Widgets.cpp
    Source/ui/Displays.cpp
    Source/ui/SynthPage.cpp
    Source/ui/ModRack.cpp
    Source/ui/FxPage.cpp
    Source/ui/Browser.cpp)

set(MC_DEFS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_REPORT_APP_USAGE=0)

juce_add_plugin(Manticore
    COMPANY_NAME "Fellowship of the String"
    COMPANY_WEBSITE ""
    BUNDLE_ID com.fellowshipofthestring.manticore
    PLUGIN_MANUFACTURER_CODE Fots
    PLUGIN_CODE Mntc
    FORMATS VST3 Standalone
    PRODUCT_NAME "Manticore"
    DESCRIPTION "Modular grid synthesizer"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Instrument Synth
    ICON_BIG Resources/icon.png
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(Manticore)
target_sources(Manticore PRIVATE ${MC_SOURCES})
target_include_directories(Manticore PRIVATE Source)
target_compile_definitions(Manticore PUBLIC ${MC_DEFS})
target_link_libraries(Manticore
    PRIVATE ManticoreData juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(MC_BUILD_TESTS)
    juce_add_console_app(ManticoreTests PRODUCT_NAME "ManticoreTests")
    juce_generate_juce_header(ManticoreTests)
    target_sources(ManticoreTests PRIVATE tests/Tests.cpp ${MC_SOURCES})
    target_include_directories(ManticoreTests PRIVATE Source)
    target_compile_definitions(ManticoreTests PRIVATE ${MC_DEFS} JUCE_STANDALONE_APPLICATION=1 MC_TESTS=1)
    target_link_libraries(ManticoreTests PRIVATE ManticoreData juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra
        juce::juce_recommended_config_flags)
endif()
