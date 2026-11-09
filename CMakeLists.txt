cmake_minimum_required(VERSION 3.22)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")  # static CRT: no VC++ redistributable needed
project(LALeveler VERSION 2.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
  add_subdirectory(JUCE)
else()
  include(FetchContent)
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
                       GIT_TAG 8.0.9 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(LALeveler
  COMPANY_NAME "Loops"
  COMPANY_WEBSITE "https://loops.co.ke"
  BUNDLE_ID co.ke.loops.compla
  PLUGIN_MANUFACTURER_CODE Jcal
  PLUGIN_CODE Lalv
  FORMATS VST3 Standalone
  PRODUCT_NAME "Comp LA"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  VST3_CATEGORIES Fx Dynamics
  COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(LALeveler PRIVATE
  Source/PluginProcessor.cpp Source/PluginProcessor.h
  Source/PluginEditor.cpp Source/PluginEditor.h
  Source/LevelerLookAndFeel.h)

target_compile_definitions(LALeveler PUBLIC
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0)

target_link_libraries(LALeveler
  PRIVATE juce::juce_audio_utils juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

set(LAL_TEST_MAIN Tests/Snapshot.cpp CACHE STRING "test main")
option(LAL_BUILD_TESTS "Build offscreen snapshot / DSP test tool" OFF)
if(LAL_BUILD_TESTS)
  juce_add_console_app(LALSnapshot PRODUCT_NAME "LALSnapshot")
  target_sources(LALSnapshot PRIVATE ${LAL_TEST_MAIN} Source/PluginProcessor.cpp Source/PluginEditor.cpp)
  target_compile_definitions(LALSnapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1)
  target_link_libraries(LALSnapshot PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()
