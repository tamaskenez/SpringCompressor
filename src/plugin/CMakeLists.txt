juce_add_plugin(SpringCompressor
    COMPANY_NAME "SpringAudio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Spng
    PLUGIN_CODE Scmp
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Spring Compressor"
)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

target_sources(SpringCompressor PRIVATE ${sources})

target_compile_definitions(SpringCompressor
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(SpringCompressor
    PRIVATE
        SpringCompressorEngine
        juce::juce_audio_utils
        magic_enum::magic_enum
        meadow::meadow
        readerwriterqueue::readerwriterqueue
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_precompile_headers(SpringCompressor PRIVATE pch.h)

