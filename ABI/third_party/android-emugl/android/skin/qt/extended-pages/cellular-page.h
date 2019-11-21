// Copyright (C) 2015 The Android Open Source Project
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

#include "ui_cellular-page.h"
#include <QWidget>
#include <memory>

struct QAndroidCellularAgent;
class CellularPage : public QWidget
{
    Q_OBJECT

public:
    explicit CellularPage(QWidget *parent = nullptr);
    void setCellularAgent(const QAndroidCellularAgent* agent);

private slots:
    void on_cell_dataStatusBox_currentIndexChanged(int index);
    void on_cell_standardBox_currentIndexChanged(int index);
    void on_cell_voiceStatusBox_currentIndexChanged(int index);
    void on_cell_signalStatusBox_currentIndexChanged(int index);

    // TODO: Implement Network delay setting
    // http://developer.android.com/tools/devices/emulator.html#netdelay

private:
    std::unique_ptr<Ui::CellularPage> mUi;
    const QAndroidCellularAgent* mCellularAgent;
};
