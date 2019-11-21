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

#include "ui_battery-page.h"
#include <QWidget>
#include <initializer_list>
#include <memory>

struct QAndroidBatteryAgent;
class BatteryPage : public QWidget {
    Q_OBJECT

public:
    explicit BatteryPage(QWidget *parent = 0);
    void setBatteryAgent(const QAndroidBatteryAgent* agent);

private slots:
    void on_bat_chargerBox_activated(int value);
    void on_bat_healthBox_activated(int index);
    void on_bat_levelSlider_valueChanged(int value);
    void on_bat_statusBox_activated(int index);

private:
    void populateListBox(
            QComboBox* list,
            std::initializer_list<std::pair<int, const char*>> associations);

private:
    std::unique_ptr<Ui::BatteryPage> mUi;
    const QAndroidBatteryAgent* mBatteryAgent;
};
