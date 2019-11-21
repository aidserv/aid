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

#pragma once

#include "ui_rotary-input-page.h"

#include <QWidget>
#include <memory>

class EmulatorQtWindow;

class RotaryInputPage : public QWidget
{
    Q_OBJECT

public:
    explicit RotaryInputPage(QWidget *parent = 0);
    void setEmulatorWindow(EmulatorQtWindow* eW);
    void updateTheme();
private:
    void onValueChanged(const int value);
private:
    int dialDeltaToRotaryInputDelta(int newDialValue, int oldDialValue);
    std::unique_ptr<Ui::RotaryInputPage> mUi;
    EmulatorQtWindow* mEmulatorWindow;
    int mValue;
};
