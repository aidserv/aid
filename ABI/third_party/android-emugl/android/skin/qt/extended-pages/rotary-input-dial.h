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

#ifndef ROTARY_INPUT_DIAL_H
#define ROTARY_INPUT_DIAL_H

#include <QDial>
#include <QSvgRenderer>

// A QDial with a custom rotating SVG instead of the standard styling.
class RotaryInputDial : public QDial
{
    Q_OBJECT

public:
    RotaryInputDial(QWidget* parent = nullptr);
    void setImage(const QString& file);
    // Rotates the image set by setImage by 'angle' degrees clockwise.
    void setImageAngleOffset(int angle);

private:
    virtual void paintEvent(QPaintEvent*) override;
    QSvgRenderer mSvgRenderer;
    int mImageAngleOffset;
};
#endif // ROTARY_INPUT_DIAL_H
