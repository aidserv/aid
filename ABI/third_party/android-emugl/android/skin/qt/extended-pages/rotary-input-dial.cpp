// Copyright (C) 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "rotary-input-dial.h"

#include <QPainter>
#include <QSvgRenderer>

RotaryInputDial::RotaryInputDial(QWidget* parent)
: QDial(parent),
  mSvgRenderer(this),
  mImageAngleOffset(0)
{
    setRange(0, 359);
    setWrapping(true);
}

void RotaryInputDial::setImage(const QString& file) {
    mSvgRenderer.load(file);
    update();
}

void RotaryInputDial::setImageAngleOffset(int angle) {
    mImageAngleOffset = angle;
    update();
}

void RotaryInputDial::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setBackgroundMode(Qt::TransparentMode);
    painter.setRenderHint(QPainter::Antialiasing);
    // Get ratio between current value and maximum to calculate angle
    double ratio = static_cast<double>(QDial::value()) / QDial::maximum();
    double angle = ratio * 360;

    double centerX = QDial::width() / 2.0;
    double centerY = QDial::height() / 2.0;
    painter.translate(centerX, centerY);
    painter.rotate(angle + mImageAngleOffset);
    painter.translate(-centerX, -centerY);
    mSvgRenderer.render(&painter);
}
