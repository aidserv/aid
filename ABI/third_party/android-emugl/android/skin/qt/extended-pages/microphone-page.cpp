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

#include "android/skin/qt/extended-pages/microphone-page.h"

#include "android/hw-events.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/qt-settings.h"
#include <QSettings>


MicrophonePage::MicrophonePage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::MicrophonePage()),
    mUserEventAgent(nullptr)
{
    mUi->setupUi(this);

    // The Hook button is not functional yet.
    // Hide it for now.
    mUi->mic_hookButton->hide();
}

void MicrophonePage::on_mic_enableMic_toggled(bool checked) {
    // TODO
}

void MicrophonePage::on_mic_hasMic_toggled(bool checked) {
    if (mUserEventAgent && mUi->mic_inserted->isChecked()) {
        // The headset is inserted, give our new microphone
        // status to the device.
        mUserEventAgent->sendGenericEvent(EV_SW,
                                          SW_MICROPHONE_INSERT,
                                          checked ? 1 : 0);
        mUserEventAgent->sendGenericEvent(EV_SYN, 0, 0);
    }
}

void MicrophonePage::on_mic_hookButton_pressed() {
    if (mUserEventAgent) {
        mUserEventAgent->sendGenericEvent(EV_KEY, KEY_HEADSETHOOK, 1);
    }
}

void MicrophonePage::on_mic_hookButton_released() {
    if (mUserEventAgent) {
        mUserEventAgent->sendGenericEvent(EV_KEY, KEY_HEADSETHOOK, 0);
    }
}

void MicrophonePage::on_mic_inserted_toggled(bool checked) {
    // Enable or disable the subordinate controls
    mUi->mic_hasMic->setEnabled(checked);
    setButtonEnabled(mUi->mic_voiceAssistButton, getSelectedTheme(), checked);
    setButtonEnabled(mUi->mic_hookButton, getSelectedTheme(), checked);

    if (mUserEventAgent) {
        // Send the indication to the device
        int phonesInserted;
        int micInserted;

        if (checked) {
            // Headphones + optional microphone
            phonesInserted = 1;
            micInserted = mUi->mic_hasMic->isChecked() ? 1 : 0;
        } else {
            // No headphones, no microphone
            phonesInserted = 0;
            micInserted = 0;
        }

        mUserEventAgent->sendGenericEvent(EV_SW, SW_HEADPHONE_INSERT, phonesInserted);
        mUserEventAgent->sendGenericEvent(EV_SW, SW_MICROPHONE_INSERT, micInserted);
        mUserEventAgent->sendGenericEvent(EV_SYN, 0, 0);
    }
}

void MicrophonePage::on_mic_voiceAssistButton_pressed() {
    if (mUserEventAgent) {
        mUserEventAgent->sendGenericEvent(EV_KEY, KEY_SEND, 1);
    }
}

void MicrophonePage::on_mic_voiceAssistButton_released() {
    if (mUserEventAgent) {
        mUserEventAgent->sendGenericEvent(EV_KEY, KEY_SEND, 0);
    }
}


void MicrophonePage::setMicrophoneAgent(const QAndroidUserEventAgent* agent) {
    mUserEventAgent = agent;
}
