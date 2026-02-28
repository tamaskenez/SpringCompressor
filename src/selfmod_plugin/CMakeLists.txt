juce_add_plugin(SelfModPlugin
    COMPANY_NAME "SpringAudio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Spng
    PLUGIN_CODE Smod
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "SelfMod"
)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

target_sources(SelfModPlugin PRIVATE ${sources})

target_compile_definitions(SelfModPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(SelfModPlugin
    PRIVATE
        SelfModEngine
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)