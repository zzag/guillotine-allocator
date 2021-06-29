add_library(guillotine STATIC allocator.cpp)
target_link_libraries(guillotine
    Qt5::Core
    Qt5::Gui
)
target_include_directories(guillotine PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)
