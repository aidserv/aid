// Copyright 2016 The Android Open Source Project
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

#include <QGraphicsDropShadowEffect>
#include <QPushButton>

// It's just a QPushButton with a drop shadow.
class RaisedMaterialButton : public QPushButton {
Q_OBJECT
public:
    explicit RaisedMaterialButton(QWidget* parent = 0) :
        QPushButton(parent),
        mShadowEffect(new QGraphicsDropShadowEffect()) {
        mShadowEffect->setBlurRadius(4.5);
        mShadowEffect->setOffset(1.0, 1.0);
        setGraphicsEffect(mShadowEffect);
        connect(this, SIGNAL(pressed()), this, SLOT(onPressed()));
        connect(this, SIGNAL(released()), this, SLOT(onReleased()));
    }

    QGraphicsDropShadowEffect* shadowEffect() { return mShadowEffect; }
    const QGraphicsDropShadowEffect* shadowEffect() const { return mShadowEffect; }

private slots:
    void onPressed() {
        mShadowEffect->setBlurRadius(3.0);
    }

    void onReleased() {
        mShadowEffect->setBlurRadius(4.5);
    }

private:
    QGraphicsDropShadowEffect* mShadowEffect;
};
