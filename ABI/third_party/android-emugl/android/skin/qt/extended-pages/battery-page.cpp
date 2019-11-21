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
#include "android/skin/qt/extended-pages/battery-page.h"

#include "android/emulation/control/battery_agent.h"
#include "ui_battery-page.h"
#include <vector>
#include <utility>

BatteryPage::BatteryPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::BatteryPage()), mBatteryAgent(nullptr) {
    mUi->setupUi(this);
    populateListBox(
            mUi->bat_chargerBox,
            {
                    {BATTERY_CHARGER_NONE, "None"},
                    {BATTERY_CHARGER_AC, "AC charger"},
                    // TODO: The UI only allows NONE and AC.
                    // Need to also allow and implement USB and WIRELESS.
                    // {BATTERY_CHARGER_USB, "USB charger"},
                    // {BATTERY_CHARGER_WIRELESS, "Wireless charger"},
            });
    populateListBox(mUi->bat_healthBox,
                    {
                            {BATTERY_HEALTH_GOOD, "Good"},
                            {BATTERY_HEALTH_FAILED, "Failed"},
                            {BATTERY_HEALTH_DEAD, "Dead"},
                            {BATTERY_HEALTH_OVERVOLTAGE, "Overvoltage"},
                            {BATTERY_HEALTH_OVERHEATED, "Overheated"},
                            {BATTERY_HEALTH_UNKNOWN, "Unknown"},
                    });
    populateListBox(mUi->bat_statusBox,
                    {
                            {BATTERY_STATUS_UNKNOWN, "Unknown"},
                            {BATTERY_STATUS_CHARGING, "Charging"},
                            {BATTERY_STATUS_DISCHARGING, "Discharging"},
                            {BATTERY_STATUS_NOT_CHARGING, "Not charging"},
                            {BATTERY_STATUS_FULL, "Full"},
                    });
}

void BatteryPage::setBatteryAgent(const QAndroidBatteryAgent* agent) {
    mBatteryAgent = agent;

    // Update the UI with values read from the battery agent.

    int chargeLevel = 50;
    if (mBatteryAgent && mBatteryAgent->chargeLevel) {
        chargeLevel = mBatteryAgent->chargeLevel();
    }
    mUi->bat_levelSlider->setValue(chargeLevel);

    BatteryCharger bCharger = BATTERY_CHARGER_AC;
    if (mBatteryAgent && mBatteryAgent->charger) {
        bCharger = mBatteryAgent->charger();
    }
    int chargeIdx = mUi->bat_chargerBox->findData(bCharger);
    if (chargeIdx < 0)
        chargeIdx = 0;  // In case the saved value wasn't found
    mUi->bat_chargerBox->setCurrentIndex(chargeIdx);

    BatteryHealth bHealth = BATTERY_HEALTH_UNKNOWN;
    if (mBatteryAgent && mBatteryAgent->health) {
        bHealth = mBatteryAgent->health();
    }
    int healthIdx = mUi->bat_healthBox->findData(bHealth);
    if (healthIdx < 0)
        healthIdx = 0;  // In case the saved value wasn't found
    mUi->bat_healthBox->setCurrentIndex(healthIdx);

    BatteryStatus bStatus = BATTERY_STATUS_UNKNOWN;
    if (mBatteryAgent && mBatteryAgent->status) {
        bStatus = mBatteryAgent->status();
    }
    int statusIdx = mUi->bat_statusBox->findData(bStatus);
    if (statusIdx < 0)
        statusIdx = 0;  // In case the saved value wasn't found
    mUi->bat_statusBox->setCurrentIndex(statusIdx);
}

void BatteryPage::populateListBox(
        QComboBox* list,
        std::initializer_list<std::pair<int, const char*>> associations) {
    list->clear();
    for (const auto& a : associations) {
        list->addItem(tr(a.second), a.first);
    }
}

void BatteryPage::on_bat_chargerBox_activated(int index) {
    BatteryCharger bCharger = static_cast<BatteryCharger>(
            mUi->bat_statusBox->itemData(index).toInt());

    if (bCharger >= 0 && bCharger < BATTERY_CHARGER_NUM_ENTRIES) {
        if (mBatteryAgent && mBatteryAgent->setCharger) {
            mBatteryAgent->setCharger(bCharger);
        }
    }
}

void BatteryPage::on_bat_levelSlider_valueChanged(int value) {
    // Update the text output
    mUi->bat_chargeLevelText->setText(QString::number(value) + "%");

    if (mBatteryAgent && mBatteryAgent->setChargeLevel) {
        mBatteryAgent->setChargeLevel(value);
    }
}

void BatteryPage::on_bat_healthBox_activated(int index) {
    BatteryHealth bHealth = static_cast<BatteryHealth>(
            mUi->bat_healthBox->itemData(index).toInt());

    if (bHealth >= 0 && bHealth < BATTERY_HEALTH_NUM_ENTRIES) {
        if (mBatteryAgent && mBatteryAgent->setHealth) {
            mBatteryAgent->setHealth(bHealth);
        }
    }
}

void BatteryPage::on_bat_statusBox_activated(int index) {
    BatteryStatus bStatus = static_cast<BatteryStatus>(
            mUi->bat_statusBox->itemData(index).toInt());

    if (bStatus >= 0 && bStatus < BATTERY_STATUS_NUM_ENTRIES) {
        if (mBatteryAgent && mBatteryAgent->setStatus) {
            mBatteryAgent->setStatus(bStatus);
        }
    }
}
