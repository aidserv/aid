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

#include "android/skin/qt/extended-pages/rotary-input-page.h"

#include "android/globals.h"
#include "android/skin/event.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/stylesheet.h"
#include "android/utils/debug.h"

RotaryInputPage::RotaryInputPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::RotaryInputPage()),
    mEmulatorWindow(NULL)
{
    mUi->setupUi(this);
    // Start with highlighted dot on the left.
    mValue = 90;
    mUi->dial->setValue(mValue);
    // Source image is 90 degrees off expected angle.
    mUi->dial->setImageAngleOffset(-90);
    connect(mUi->dial, &QDial::valueChanged,
        [this](int value) { onValueChanged(value); });

    updateTheme();
}

void RotaryInputPage::setEmulatorWindow(EmulatorQtWindow* eW)
{
    mEmulatorWindow = eW;
}

void RotaryInputPage::updateTheme()
{
    mUi->dial->setImage(QString(
        ":/"
        + Ui::stylesheetValues(getSelectedTheme())[Ui::THEME_PATH_VAR]
        + "/rotary_input_control"));
}

void RotaryInputPage::onValueChanged(const int value) {
    int delta = dialDeltaToRotaryInputDelta(value, mValue);
    VERBOSE_PRINT(keys, "Rotary input value change: %d", delta);
    if (mEmulatorWindow) {
        SkinEvent* skin_event = new SkinEvent();
        skin_event->type = kEventRotaryInput;
        skin_event->u.rotary_input.delta = delta;
        mEmulatorWindow->queueSkinEvent(skin_event);
    }

    mValue = value;
}


// Returns the angle difference between two dial values, with a result that's
// always in [-180, 180).
int RotaryInputPage::dialDeltaToRotaryInputDelta(int newDialValue, int oldDialValue) {
    int delta = newDialValue - oldDialValue;
    if (delta > 180) {
        delta -= 360;
    } else if (delta <= -180) {
        delta += 360;
    }
    // Invert delta to fit rotation direction of rotary input.
    return -delta;
}
