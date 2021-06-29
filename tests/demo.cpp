/*
    SPDX-FileCopyrightText: 2021 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QRandomGenerator>
#include <QWidget>

#include "kguillotineallocator.h"

class Window : public QWidget
{
    Q_OBJECT

    struct AllocationData {
        KGuillotineAllocator::Allocation allocation;
        QColor color;
    };

public:
    Window(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        resize(800, 800);
        reload();
    }

protected:
    void resizeEvent(QResizeEvent *) override
    {
        reload();
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), QColor(200, 200, 200));

        for (const AllocationData &d : qAsConst(m_allocations)) {
            painter.fillRect(d.allocation.rect, d.color);
            painter.drawText(d.allocation.rect, QString("%1x%2")
                    .arg(d.allocation.rect.width()).arg(d.allocation.rect.height()));
        }
    }

private:
    void reload()
    {
        struct Box {
            const QSize size;
            const QColor color;
        };

        static QVector<Box> boxes;
        if (boxes.isEmpty()) {
            const QVector<QSize> desiredSizes {
                QSize(100, 40),
                QSize(60, 300),
                QSize(250, 270),
                QSize(300, 20),
            };

            QRandomGenerator *rng = QRandomGenerator::global();
            for (const QSize &size : desiredSizes) {
                const QColor color(rng->generate() % 256,
                                   rng->generate() % 256,
                                   rng->generate() % 256);
                boxes.append(Box{size, color});
            }
        }

        m_allocator.reset(new KGuillotineAllocator::Allocator(size()));
        m_allocations.clear();

        for (const Box &box : boxes) {
            const KGuillotineAllocator::Allocation allocation = m_allocator->allocate(box.size);
            if (allocation.isNull())
                qDebug() << "failed to allocate space for" << box.size;
            else
                m_allocations.append({allocation, box.color});
        }
    }

    QScopedPointer<KGuillotineAllocator::Allocator> m_allocator;
    QVector<AllocationData> m_allocations;
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    Window w;
    w.show();
    return app.exec();
}

#include "demo.moc"
