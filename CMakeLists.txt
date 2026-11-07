cmake_minimum_required(VERSION 3.22)
project(PyanyiKon VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

# JUCE лежит в папке PyanyiKon/JUCE (git clone https://github.com/juce-framework/JUCE)
add_subdirectory(JUCE)

juce_add_plugin(PyanyiKon
    COMPANY_NAME "YouthAudio"
    PLUGIN_MANUFACTURER_CODE Yaud
    PLUGIN_CODE Pkon
    FORMATS VST3 Standalone
    PRODUCT_NAME "Pyanyi Kon"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(PyanyiKon)

juce_add_binary_data(KonAssets SOURCES Assets/horse.png)

target_sources(PyanyiKon PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h)

target_compile_definitions(PyanyiKon PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(PyanyiKon
    PRIVATE
        KonAssets
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(MSVC)
    target_compile_options(PyanyiKon PRIVATE /utf-8)
endif()
