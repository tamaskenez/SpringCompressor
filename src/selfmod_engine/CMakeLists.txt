file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_library(SelfModEngine STATIC ${sources})

target_include_directories(SelfModEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})