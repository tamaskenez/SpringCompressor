file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_library(juce_util STATIC ${sources})
target_link_libraries(juce_util
    PUBLIC
        juce::juce_events
        meadow::meadow
    PRIVATE
        abseil::abseil
)
target_include_directories(juce_util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)