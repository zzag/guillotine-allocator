# SPDX-FileCopyrightText: 2021 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
#
# SPDX-License-Identifier: BSD-3-Clause

add_executable(demo demo.cpp)
target_link_libraries(demo Qt5::Core Qt5::Gui Qt5::Widgets KGuillotineAllocator::KGuillotineAllocator)
