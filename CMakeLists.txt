cmake_minimum_required(VERSION 3.22)

project(Muze VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(MUZE_BUILD_AUTOKEY     "Build the Muze Auto Key companion plugin" ON)
option(MUZE_BUILD_TESTS       "Build the offline DSP/preset test runner"  ON)
option(MUZE_BUILD_SCREENSHOT  "Build the headless editor screenshot tool"  OFF)
option(MUZE_COPY_PLUGINS      "Copy built plugins into the user plugin folders" OFF)
set(MUZE_JUCE_TAG "8.0.15" CACHE STRING "JUCE git tag to fetch when JUCE_PATH is not given")
set(JUCE_PATH "" CACHE PATH "Optional path to a local JUCE checkout (skips the download)")
set(MUZE_AAX_SDK_PATH "" CACHE PATH "Optional path to the AAX SDK (enables the AAX format)")

# ---------------------------------------------------------------------------
# JUCE
# ---------------------------------------------------------------------------
if (JUCE_PATH)
    add_subdirectory("${JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
elseif (DEFINED ENV{JUCE_PATH} AND EXISTS "$ENV{JUCE_PATH}/CMakeLists.txt")
    add_subdirectory("$ENV{JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        ${MUZE_JUCE_TAG}
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------------------
# Formats
# ---------------------------------------------------------------------------
set(MUZE_FORMATS VST3 Standalone)
if (APPLE)
    list(APPEND MUZE_FORMATS AU)
endif()
if (MUZE_AAX_SDK_PATH)
    juce_set_aax_sdk_path("${MUZE_AAX_SDK_PATH}")
    list(APPEND MUZE_FORMATS AAX)
endif()

# ---------------------------------------------------------------------------
# Shared core (compiled into every target)
# ---------------------------------------------------------------------------
set(MUZE_CORE_SOURCES
    Source/Core/ModuleRegistry.cpp
    Source/Core/Scales.cpp
    Source/Core/KeyDetector.cpp
    Source/Core/VocalAnalyzer.cpp
    Source/Core/SharedKey.cpp
    Source/Presets/Presets.cpp
    Source/Presets/PresetData.cpp
    Source/DSP/PitchDetector.cpp
    Source/DSP/PitchShifter.cpp
    Source/DSP/Modules/ModuleFactory.cpp
)

add_library(MuzeCore INTERFACE)
target_sources(MuzeCore INTERFACE ${MUZE_CORE_SOURCES})
target_include_directories(MuzeCore INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
target_compile_definitions(MuzeCore INTERFACE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)
target_link_libraries(MuzeCore INTERFACE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Muze (main vocal plugin)
# ---------------------------------------------------------------------------
juce_add_plugin(Muze
    PRODUCT_NAME                "Muze"
    COMPANY_NAME                "Muze Replica"
    BUNDLE_ID                   "fyi.muze.replica.muze"
    PLUGIN_MANUFACTURER_CODE    Mzrp
    PLUGIN_CODE                 Muz1
    FORMATS                     ${MUZE_FORMATS}
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     ${MUZE_COPY_PLUGINS}
    VST3_CATEGORIES             Fx Dynamics EQ PitchShift Reverb Delay
    AU_MAIN_TYPE                kAudioUnitType_Effect
    AAX_CATEGORY                AAX_ePlugInCategory_Effect)

target_sources(Muze PRIVATE
    Source/Plugin/MuzeProcessor.cpp
    Source/Plugin/MuzeEditor.cpp
    Source/Plugin/UI/MuzeLookAndFeel.cpp
    Source/Plugin/UI/HeaderBar.cpp
    Source/Plugin/UI/FooterBar.cpp
    Source/Plugin/UI/PresetBrowserPanel.cpp
    Source/Plugin/UI/ChainStrip.cpp
    Source/Plugin/UI/ModulePanel.cpp
    Source/Plugin/UI/MacroPanel.cpp
    Source/Plugin/UI/MixView.cpp
    Source/Plugin/UI/CommunityPanel.cpp
    Source/Plugin/UI/AnalyzePanel.cpp)

target_link_libraries(Muze PRIVATE MuzeCore juce::juce_recommended_lto_flags)

# ---------------------------------------------------------------------------
# Muze Auto Key (companion key finder)
# ---------------------------------------------------------------------------
if (MUZE_BUILD_AUTOKEY)
    juce_add_plugin(MuzeAutoKey
        PRODUCT_NAME                "Muze Auto Key"
        COMPANY_NAME                "Muze Replica"
        BUNDLE_ID                   "fyi.muze.replica.autokey"
        PLUGIN_MANUFACTURER_CODE    Mzrp
        PLUGIN_CODE                 Mzak
        FORMATS                     ${MUZE_FORMATS}
        IS_SYNTH                    FALSE
        NEEDS_MIDI_INPUT            FALSE
        NEEDS_MIDI_OUTPUT           FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD     ${MUZE_COPY_PLUGINS}
        VST3_CATEGORIES             Fx Analyzer Tools
        AU_MAIN_TYPE                kAudioUnitType_Effect
        AAX_CATEGORY                AAX_ePlugInCategory_Effect)

    target_sources(MuzeAutoKey PRIVATE
        Source/AutoKey/AutoKeyProcessor.cpp
        Source/AutoKey/AutoKeyEditor.cpp
        Source/Plugin/UI/MuzeLookAndFeel.cpp)

    target_link_libraries(MuzeAutoKey PRIVATE MuzeCore juce::juce_recommended_lto_flags)
endif()

# ---------------------------------------------------------------------------
# Offline tests
# ---------------------------------------------------------------------------
if (MUZE_BUILD_TESTS)
    juce_add_console_app(MuzeTests PRODUCT_NAME "MuzeTests")
    target_sources(MuzeTests PRIVATE
        Tests/MuzeTests.cpp
        Source/Plugin/MuzeProcessor.cpp
        Source/Plugin/MuzeEditor.cpp
        Source/Plugin/UI/MuzeLookAndFeel.cpp
        Source/Plugin/UI/HeaderBar.cpp
        Source/Plugin/UI/FooterBar.cpp
        Source/Plugin/UI/PresetBrowserPanel.cpp
        Source/Plugin/UI/ChainStrip.cpp
        Source/Plugin/UI/ModulePanel.cpp
        Source/Plugin/UI/MacroPanel.cpp
        Source/Plugin/UI/MixView.cpp
        Source/Plugin/UI/CommunityPanel.cpp
        Source/Plugin/UI/AnalyzePanel.cpp)
    target_compile_definitions(MuzeTests PRIVATE
        MUZE_STANDALONE_TEST=1
        JucePlugin_Name="Muze"
        JucePlugin_WantsMidiInput=0
        JucePlugin_ProducesMidiOutput=0
        JucePlugin_IsMidiEffect=0
        JucePlugin_IsSynth=0
        JucePlugin_EditorRequiresKeyboardFocus=0
        JucePlugin_VersionString="1.0.0")
    target_link_libraries(MuzeTests PRIVATE MuzeCore)
    enable_testing()
    add_test(NAME MuzeTests COMMAND MuzeTests)
endif()

# ---------------------------------------------------------------------------
# Headless screenshot tool (renders the editor to PNG, used for UI checks)
# ---------------------------------------------------------------------------
if (MUZE_BUILD_SCREENSHOT)
    juce_add_gui_app(MuzeScreenshot PRODUCT_NAME "MuzeScreenshot")
    target_sources(MuzeScreenshot PRIVATE
        Tests/Screenshot.cpp
        Source/Plugin/MuzeProcessor.cpp
        Source/Plugin/MuzeEditor.cpp
        Source/Plugin/UI/MuzeLookAndFeel.cpp
        Source/Plugin/UI/HeaderBar.cpp
        Source/Plugin/UI/FooterBar.cpp
        Source/Plugin/UI/PresetBrowserPanel.cpp
        Source/Plugin/UI/ChainStrip.cpp
        Source/Plugin/UI/ModulePanel.cpp
        Source/Plugin/UI/MacroPanel.cpp
        Source/Plugin/UI/MixView.cpp
        Source/Plugin/UI/CommunityPanel.cpp
        Source/Plugin/UI/AnalyzePanel.cpp
        Source/AutoKey/AutoKeyProcessor.cpp
        Source/AutoKey/AutoKeyEditor.cpp)
    target_compile_definitions(MuzeScreenshot PRIVATE
        MUZE_STANDALONE_TEST=1
        JucePlugin_Name="Muze"
        JucePlugin_WantsMidiInput=0
        JucePlugin_ProducesMidiOutput=0
        JucePlugin_IsMidiEffect=0
        JucePlugin_IsSynth=0
        JucePlugin_EditorRequiresKeyboardFocus=0
        JucePlugin_VersionString="1.0.0")
    target_link_libraries(MuzeScreenshot PRIVATE MuzeCore)
endif()
