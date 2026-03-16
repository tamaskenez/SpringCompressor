file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_library(SpringCompressorEngine STATIC ${sources})

target_include_directories(SpringCompressorEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(SpringCompressorEngine
    PRIVATE
        meadow::meadow
        readerwriterqueue::readerwriterqueue
)

target_precompile_headers(SpringCompressorEngine PRIVATE pch.h)