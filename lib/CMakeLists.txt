# SPDX-FileCopyrightText: 2021 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
#
# SPDX-License-Identifier: BSD-3-Clause

set(allocatorlib_SOURCES
    kguillotineallocator.cpp
)

ecm_generate_headers(allocatorlib_HEADERS
    HEADER_NAMES
        KGuillotineAllocator
    REQUIRED_HEADERS allocatorlib_HEADERS
)

add_library(kguillotineallocator STATIC ${allocatorlib_SOURCES})
add_library(KGuillotineAllocator::KGuillotineAllocator ALIAS kguillotineallocator)

set_target_properties(kguillotineallocator PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_include_directories(kguillotineallocator PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KGuillotineAllocator>"
)

target_link_libraries(kguillotineallocator
    PUBLIC
        Qt5::Core
        Qt5::Gui
)
