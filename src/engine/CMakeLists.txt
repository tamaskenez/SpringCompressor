add_library(SpringCompressorEngine STATIC
    engine.cpp
    engine.h
)

target_include_directories(SpringCompressorEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
