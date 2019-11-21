// Copyright (C) 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include <QObject>
#include <QWidget>
#include <QVector2D>
#include <QPointer>

// This is a special component that adjusts widget sizes for
// high-density displays. We don't need it on OS X, but we need
// it for Windows/Linux.
class SizeTweaker : public QObject {
Q_OBJECT
public:
    // The UI looks good at 96 DPI.
    static const qreal BaselineDpi;

    // Creates a new SizeTweaker that will adjust the appearance
    // of |widget| based on DPI, as it gets shown/moved between
    // displays.
    explicit SizeTweaker(QWidget* widget);

private:
    // Intercepts show and move events from the managed widget.
    bool eventFilter(QObject*, QEvent* event) override;

    // Adjusts the appearance of the managed widget according to
    // the DPI property of the screen that the widget is on.
    void adjustSizesAndPositions();

private:
    // Widget to operate on.
    QPointer<QWidget> mSubject;

    // Scale factors relative to baseline DPI.
    // For example, a value of "2" here would indicate that
    // the managed widget is currently adjusted for 192 DPI
    // (given that baseline DPI is 96)
    QVector2D mCurrentScaleFactor;
};

