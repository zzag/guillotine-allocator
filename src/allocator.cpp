/*
    SPDX-FileCopyrightText: 2021 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-3.0-or-later
*/

#include "allocator.h"

#include <QDebug>
#include <QVector>

#include <limits>

namespace KGuillotineAllocator
{

struct AllocationNode
{
    enum class Kind {
        Fork,
        Leaf,
    };

    enum class Status {
        Free,
        Occupied,
        Deleted,
    };

    AllocationId prevSibling = AllocationId::null();
    AllocationId nextSibling = AllocationId::null();
    AllocationId parent = AllocationId::null();

    Qt::Orientation orientation;
    QRect rect;
    Kind kind;
    Status status;
};

class AllocatorPrivate
{
public:
    AllocationId selectFreeNode(const QSize &size) const;

    AllocationId allocateNode();
    void releaseNode(AllocationId nodeId);

    QVector<AllocationNode> nodes;
    QSize size;
};

AllocationId AllocatorPrivate::allocateNode()
{
    for (int i = 0; i < nodes.count(); ++i) {
        if (nodes[i].status == AllocationNode::Status::Deleted) {
            return AllocationId(i);
        }
    }

    nodes.append(AllocationNode{});
    return nodes.count() - 1;
}

void AllocatorPrivate::releaseNode(AllocationId nodeId)
{
    nodes[nodeId].status = AllocationNode::Status::Deleted;
}

Allocator::Allocator(const QSize &size)
    : d(new AllocatorPrivate)
{
    d->size = size;

    d->nodes.append(AllocationNode{
        .prevSibling = AllocationId::null(),
        .nextSibling = AllocationId::null(),
        .parent = AllocationId::null(),
        .orientation = Qt::Horizontal,
        .rect = QRect(QPoint(0, 0), size),
        .kind = AllocationNode::Kind::Leaf,
        .status = AllocationNode::Status::Free,
    });
}

Allocator::~Allocator()
{
}

QSize Allocator::size() const
{
    return d->size;
}

AllocationId AllocatorPrivate::selectFreeNode(const QSize &size) const
{
    int bestCandidate = AllocationId::null();
    int bestScore = std::numeric_limits<int>::max();

    for (int nodeId = 0; nodeId < nodes.count(); ++nodeId) {
        if (nodes[nodeId].status != AllocationNode::Status::Free ||
                nodes[nodeId].kind != AllocationNode::Kind::Leaf) {
            continue;
        }

        const QSize availableSize = nodes[nodeId].rect.size();
        const int xDelta = availableSize.width() - size.width();
        const int yDelta = availableSize.height() - size.height();

        if (xDelta < 0 || yDelta < 0) {
            continue;
        }

        const int score = std::min(xDelta, yDelta);
        if (score < bestScore) {
            bestCandidate = nodeId;
            bestScore = score;
        }
    }
    return bestCandidate;
}

static Qt::Orientation flipOrientation(Qt::Orientation orientation)
{
    return orientation == Qt::Horizontal ? Qt::Vertical : Qt::Horizontal;
}

static std::tuple<QRect, QRect, QRect> guillotine(const QRect &bounds, const QSize &size, Qt::Orientation axis)
{
    const QRect allocatedRect(bounds.topLeft(), size);

    QRect leftoverRect, splitRect;
    if (axis == Qt::Vertical) {
        leftoverRect = QRect(bounds.x(),
                             bounds.y() + size.height(),
                             size.width(),
                             bounds.height() - size.height());

        splitRect = QRect(bounds.x() + size.width(),
                          bounds.y(),
                          bounds.width() - size.width(),
                          bounds.height());
    } else {
        leftoverRect = QRect(bounds.x() + size.width(),
                             bounds.y(),
                             bounds.width() - size.width(),
                             size.height());

        splitRect = QRect(bounds.x(),
                          bounds.y() + size.height(),
                          bounds.width(),
                          bounds.height() - size.height());
    }

    return {allocatedRect, leftoverRect, splitRect};
}

Allocation Allocator::allocate(const QSize &requestedSize)
{
    if (Q_UNLIKELY(requestedSize.isEmpty())) {
        return Allocation{};
    }

    const AllocationId selectedId = d->selectFreeNode(requestedSize);
    if (selectedId == AllocationId::null()) {
        return Allocation{};
    }

    if (d->nodes[selectedId].rect.size() == requestedSize) {
        d->nodes[selectedId].status = AllocationNode::Status::Occupied;
        return Allocation{.rect = d->nodes[selectedId].rect, .id = selectedId,};
    }

    const auto [allocatedRect, leftoverRect, splitRect] =
            guillotine(d->nodes[selectedId].rect, requestedSize, d->nodes[selectedId].orientation);

    // Note that some rectangles can be empty, avoid creating nodes for these rects.
    const AllocationId allocatedId = d->allocateNode();
    const AllocationId leftoverId =
            !leftoverRect.isEmpty() ? d->allocateNode() : AllocationId::null();
    const AllocationId splitId =
            !splitRect.isEmpty() ? d->allocateNode() : AllocationId::null();

    d->nodes[selectedId].kind = AllocationNode::Kind::Fork;

    const Qt::Orientation childOrientation = flipOrientation(d->nodes[selectedId].orientation);
    d->nodes[allocatedId] = AllocationNode{
        .prevSibling = AllocationId::null(),
        .nextSibling = leftoverId,
        .parent = selectedId,

        .orientation = childOrientation,
        .rect = allocatedRect,
        .kind = AllocationNode::Kind::Leaf,
        .status = AllocationNode::Status::Occupied,
    };

    // If the requested rectangle perfectly fits the bin, i.e. there is no leftover,
    // avoid creating the leftover node.
    if (leftoverId != AllocationId::null()) {
        d->nodes[leftoverId] = AllocationNode{
            .prevSibling = allocatedId,
            .nextSibling = AllocationId::null(),
            .parent = selectedId,

            .orientation = childOrientation,
            .rect = leftoverRect,
            .kind = AllocationNode::Kind::Leaf,
            .status = AllocationNode::Status::Free,
        };
    }

    // Avoid creating the split node if its area is empty, the leftover rect can still
    // be valid though. Note that the split node is a sibling of the parent node.
    if (splitId != AllocationId::null()) {
        d->nodes[splitId] = AllocationNode{
            .prevSibling = selectedId,
            .nextSibling = d->nodes[selectedId].nextSibling,
            .parent = d->nodes[selectedId].parent,

            .orientation = d->nodes[selectedId].orientation,
            .rect = splitRect,
            .kind = AllocationNode::Kind::Leaf,
            .status = AllocationNode::Status::Free,
        };
        d->nodes[selectedId].nextSibling = splitId;
    }

    return Allocation{.rect = allocatedRect, .id = allocatedId,};
}

void Allocator::deallocate(AllocationId nodeId)
{
    d->nodes[nodeId].status = AllocationNode::Status::Free;

    while (true) {
        // Merge the node with the next (free) sibling nodes. Note that the sibling nodes are
        // sorted along the axis where they had been split.
        while (d->nodes[nodeId].nextSibling != AllocationId::null()) {
            const AllocationId nextSibling = d->nodes[nodeId].nextSibling;
            if (d->nodes[nextSibling].kind != AllocationNode::Kind::Leaf ||
                    d->nodes[nextSibling].status != AllocationNode::Status::Free) {
                break;
            }

            d->nodes[nodeId].rect |= d->nodes[nextSibling].rect;

            const AllocationId grandNextSibling = d->nodes[nextSibling].nextSibling;
            if (grandNextSibling != AllocationId::null()) {
                d->nodes[grandNextSibling].prevSibling = nodeId;
                d->nodes[nodeId].nextSibling = grandNextSibling;
            }

            d->releaseNode(nextSibling);
        }

        // Merge the node with the previous (free) sibling nodes. Note that sibling nodes are
        // sorted along the axis where they had been split.
        while (d->nodes[nodeId].prevSibling != AllocationId::null()) {
            const AllocationId prevSibling = d->nodes[nodeId].prevSibling;
            if (d->nodes[prevSibling].kind != AllocationNode::Kind::Leaf ||
                    d->nodes[prevSibling].status != AllocationNode::Status::Free) {
                break;
            }

            d->nodes[nodeId].rect |= d->nodes[prevSibling].rect;

            const AllocationId grandPrevSibling = d->nodes[prevSibling].prevSibling;
            if (grandPrevSibling != AllocationId::null()) {
                d->nodes[grandPrevSibling].nextSibling = nodeId;
                d->nodes[nodeId].prevSibling = grandPrevSibling;
            }

            d->releaseNode(prevSibling);
        }

        // Stop if it's either a root node or one of the siblings is still occcupied.
        if (d->nodes[nodeId].parent == AllocationId::null() ||
                d->nodes[nodeId].prevSibling != AllocationId::null() ||
                d->nodes[nodeId].nextSibling != AllocationId::null()) {
            break;
        }

        // If the parent node has only one child, merge those two and check whether the parent
        // node can be merged with one of its siblings.
        const AllocationId parentId = d->nodes[nodeId].parent;
        d->nodes[parentId].rect = d->nodes[nodeId].rect;
        d->nodes[parentId].kind = AllocationNode::Kind::Leaf;
        d->nodes[parentId].status = AllocationNode::Status::Free;

        d->releaseNode(nodeId);
        nodeId = parentId;
    }
}

} // namespace KGuillotineAllocator
