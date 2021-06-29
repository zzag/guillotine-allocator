/*
    SPDX-FileCopyrightText: 2021 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-3.0-or-later
*/

#pragma once

#include <QRect>
#include <QScopedPointer>

namespace KGuillotineAllocator
{
class AllocatorPrivate;

/**
 * \internal
 */
struct Q_DECL_EXPORT AllocationId
{
    static constexpr AllocationId null() noexcept { return -1; }

    constexpr AllocationId(qsizetype value) noexcept
        : data(value)
    {}
    constexpr operator int() const noexcept
    {
        return data;
    }
    constexpr bool operator ==(const AllocationId &other) const noexcept
    {
        return data == other.data;
    }

    qsizetype data;
};

/**
 * The Allocation type represents a single 2D allocation. The isNull() function
 * returns \c true if the allocation is invalid.
 */
struct Q_DECL_EXPORT Allocation
{
    bool isNull() const { return id == AllocationId::null(); }

    QRect rect;
    AllocationId id = AllocationId::null();
    bool transposed;
};

/**
 * The AllocatorOptions provide a way to fine tune the behavior of the allocator.
 */
struct Q_DECL_EXPORT AllocatorOptions
{
    bool allowTranspose = true;
};

/**
 * The Allocator class represens a dynamic texture atlas allocator.
 *
 * If the allocator  has failed to find a suitable area for the specified texture size, a null
 * Allocation object will be returned.
 */
class Q_DECL_EXPORT Allocator
{
public:
    explicit Allocator(const QSize &size, const AllocatorOptions &options = {});
    ~Allocator();

    QSize size() const;

    /**
     * Allocates the space for a texture with the specified size \a requestedSize. If
     * the allocation has failed, a null Allocation object will be returned.
     */
    Allocation allocate(const QSize &requestedSize);

    /**
     * Release a rectangular area previously allocated by the allocate() function. Passing
     * an allocation id that was not returned by allocate() will lead to undefined behavior.
     */
    void deallocate(AllocationId allocationId);

private:
    QScopedPointer<AllocatorPrivate> d;
};

} // namespace KGuillotineAllocator
