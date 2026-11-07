cmake_minimum_required(VERSION 3.22)

project(Vocalyst VERSION 1.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(VOCALYST_BUILD_AUTOKEY     "Build the Vocalyst Auto Key companion plugin" ON)
option(VOCALYST_BUILD_TESTS       "Build the offline DSP/preset test runner"  ON)
option(VOCALYST_BUILD_SCREENSHOT  "Build the headless editor screenshot tool"  OFF)
option(VOCALYST_COPY_PLUGINS      "Copy built plugins into the user plugin folders" OFF)
set(VOCALYST_JUCE_TAG "8.0.15" CACHE STRING "JUCE git tag to fetch when JUCE_PATH is not given")
set(JUCE_PATH "" CACHE PATH "Optional path to a local JUCE checkout (skips the download)")
set(VOCALYST_AAX_SDK_PATH "" CACHE PATH "Optional path to the AAX SDK (enables the AAX format)")

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
        GIT_TAG        ${VOCALYST_JUCE_TAG}
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------------------
# Formats
# ---------------------------------------------------------------------------
set(VOCALYST_FORMATS VST3 Standalone)
if (APPLE)
    list(APPEND VOCALYST_FORMATS AU)
endif()
if (VOCALYST_AAX_SDK_PATH)
    juce_set_aax_sdk_path("${VOCALYST_AAX_SDK_PATH}")
    list(APPEND VOCALYST_FORMATS AAX)
endif()

# ---------------------------------------------------------------------------
# Shared core (compiled into every target)
# ---------------------------------------------------------------------------
set(VOCALYST_CORE_SOURCES
    Source/Core/ModuleRegistry.cpp
    Source/Core/Scales.cpp
    Source/Core/KeyDetector.cpp
    Source/Core/VocalAnalyzer.cpp
    Source/Core/SharedKey.cpp
    Source/Core/MeterEngine.cpp
    Source/Presets/Presets.cpp
    Source/Presets/PresetData.cpp
    Source/DSP/PitchDetector.cpp
    Source/DSP/PitchShifter.cpp
    Source/DSP/Modules/ModuleFactory.cpp
)

# Embedded assets (Inter typeface, SIL OFL)
juce_add_binary_data(VocalystAssets SOURCES
    Resources/fonts/Inter-Regular.ttf
    Resources/fonts/Inter-Medium.ttf
    Resources/fonts/Inter-SemiBold.ttf
    Resources/fonts/Inter-Bold.ttf)

add_library(VocalystCore INTERFACE)
target_sources(VocalystCore INTERFACE ${VOCALYST_CORE_SOURCES})
target_include_directories(VocalystCore INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
target_compile_definitions(VocalystCore INTERFACE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)
target_link_libraries(VocalystCore INTERFACE
    VocalystAssets
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Vocalyst (main vocal plugin)
# ---------------------------------------------------------------------------
juce_add_plugin(Vocalyst
    PRODUCT_NAME                "Vocalyst"
    COMPANY_NAME                "AnyMeans"
    BUNDLE_ID                   "ai.anymeans.vocalyst"
    PLUGIN_MANUFACTURER_CODE    Anym
    PLUGIN_CODE                 Vcl1
    FORMATS                     ${VOCALYST_FORMATS}
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     ${VOCALYST_COPY_PLUGINS}
    VST3_CATEGORIES             Fx Dynamics EQ PitchShift Reverb Delay
    AU_MAIN_TYPE                kAudioUnitType_Effect
    AAX_CATEGORY                AAX_ePlugInCategory_Effect)

target_sources(Vocalyst PRIVATE
    Source/Plugin/VocalystProcessor.cpp
    Source/Plugin/VocalystEditor.cpp
    Source/Plugin/UI/VocalystLookAndFeel.cpp
    Source/Plugin/UI/HeaderBar.cpp
    Source/Plugin/UI/FooterBar.cpp
    Source/Plugin/UI/PresetBrowserPanel.cpp
    Source/Plugin/UI/ChainStrip.cpp
    Source/Plugin/UI/ModulePanel.cpp
    Source/Plugin/UI/MacroPanel.cpp
    Source/Plugin/UI/MixView.cpp
    Source/Plugin/UI/CommunityPanel.cpp
    Source/Plugin/UI/AnalyzePanel.cpp
    Source/Plugin/UI/MeterStrip.cpp
    Source/Plugin/UI/TuneView.cpp)

target_link_libraries(Vocalyst PRIVATE VocalystCore juce::juce_recommended_lto_flags)

# ---------------------------------------------------------------------------
# Vocalyst Auto Key (companion key finder)
# ---------------------------------------------------------------------------
if (VOCALYST_BUILD_AUTOKEY)
    juce_add_plugin(VocalystAutoKey
        PRODUCT_NAME                "Vocalyst Auto Key"
        COMPANY_NAME                "AnyMeans"
        BUNDLE_ID                   "ai.anymeans.vocalystautokey"
        PLUGIN_MANUFACTURER_CODE    Anym
        PLUGIN_CODE                 Vcak
        FORMATS                     ${VOCALYST_FORMATS}
        IS_SYNTH                    FALSE
        NEEDS_MIDI_INPUT            FALSE
        NEEDS_MIDI_OUTPUT           FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD     ${VOCALYST_COPY_PLUGINS}
        VST3_CATEGORIES             Fx Analyzer Tools
        AU_MAIN_TYPE                kAudioUnitType_Effect
        AAX_CATEGORY                AAX_ePlugInCategory_Effect)

    target_sources(VocalystAutoKey PRIVATE
        Source/AutoKey/AutoKeyProcessor.cpp
        Source/AutoKey/AutoKeyEditor.cpp
        Source/Plugin/UI/VocalystLookAndFeel.cpp)

    target_link_libraries(VocalystAutoKey PRIVATE VocalystCore juce::juce_recommended_lto_flags)
endif()

# ---------------------------------------------------------------------------
# Offline tests
# ---------------------------------------------------------------------------
if (VOCALYST_BUILD_TESTS)
    juce_add_console_app(VocalystTests PRODUCT_NAME "VocalystTests")
    target_sources(VocalystTests PRIVATE
        Tests/VocalystTests.cpp
        Source/Plugin/VocalystProcessor.cpp
        Source/Plugin/VocalystEditor.cpp
        Source/Plugin/UI/VocalystLookAndFeel.cpp
        Source/Plugin/UI/HeaderBar.cpp
        Source/Plugin/UI/FooterBar.cpp
        Source/Plugin/UI/PresetBrowserPanel.cpp
        Source/Plugin/UI/ChainStrip.cpp
        Source/Plugin/UI/ModulePanel.cpp
        Source/Plugin/UI/MacroPanel.cpp
        Source/Plugin/UI/MixView.cpp
        Source/Plugin/UI/CommunityPanel.cpp
        Source/Plugin/UI/AnalyzePanel.cpp
    Source/Plugin/UI/MeterStrip.cpp
    Source/Plugin/UI/TuneView.cpp)
    target_compile_definitions(VocalystTests PRIVATE
        VOCALYST_STANDALONE_TEST=1
        JucePlugin_Name="Vocalyst"
        JucePlugin_WantsMidiInput=0
        JucePlugin_ProducesMidiOutput=0
        JucePlugin_IsMidiEffect=0
        JucePlugin_IsSynth=0
        JucePlugin_EditorRequiresKeyboardFocus=0
        JucePlugin_VersionString="1.1.0")
    target_link_libraries(VocalystTests PRIVATE VocalystCore)
    enable_testing()
    add_test(NAME VocalystTests COMMAND VocalystTests)
endif()

# ---------------------------------------------------------------------------
# Headless screenshot tool (renders the editor to PNG, used for UI checks)
# ---------------------------------------------------------------------------
if (VOCALYST_BUILD_SCREENSHOT)
    juce_add_gui_app(VocalystScreenshot PRODUCT_NAME "VocalystScreenshot")
    target_sources(VocalystScreenshot PRIVATE
        Tests/Screenshot.cpp
        Source/Plugin/VocalystProcessor.cpp
        Source/Plugin/VocalystEditor.cpp
        Source/Plugin/UI/VocalystLookAndFeel.cpp
        Source/Plugin/UI/HeaderBar.cpp
        Source/Plugin/UI/FooterBar.cpp
        Source/Plugin/UI/PresetBrowserPanel.cpp
        Source/Plugin/UI/ChainStrip.cpp
        Source/Plugin/UI/ModulePanel.cpp
        Source/Plugin/UI/MacroPanel.cpp
        Source/Plugin/UI/MixView.cpp
        Source/Plugin/UI/CommunityPanel.cpp
        Source/Plugin/UI/AnalyzePanel.cpp
        Source/Plugin/UI/MeterStrip.cpp
        Source/Plugin/UI/TuneView.cpp
        Source/AutoKey/AutoKeyProcessor.cpp
        Source/AutoKey/AutoKeyEditor.cpp)
    target_compile_definitions(VocalystScreenshot PRIVATE
        VOCALYST_STANDALONE_TEST=1
        JucePlugin_Name="Vocalyst"
        JucePlugin_WantsMidiInput=0
        JucePlugin_ProducesMidiOutput=0
        JucePlugin_IsMidiEffect=0
        JucePlugin_IsSynth=0
        JucePlugin_EditorRequiresKeyboardFocus=0
        JucePlugin_VersionString="1.1.0")
    target_link_libraries(VocalystScreenshot PRIVATE VocalystCore)
endif()
