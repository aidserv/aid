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

#include "ui_microphone-page.h"

#include <QWidget>
#include <memory>

#include "android/emulation/control/user_event_agent.h"


class MicrophonePage : public QWidget
{
    Q_OBJECT

public:
    explicit MicrophonePage(QWidget *parent = 0);

    void setMicrophoneAgent(const QAndroidUserEventAgent* agent);

private slots:
    void on_mic_enableMic_toggled(bool checked);
    void on_mic_hasMic_toggled(bool checked);
    void on_mic_hookButton_pressed();
    void on_mic_hookButton_released();
    void on_mic_inserted_toggled(bool checked);
    void on_mic_voiceAssistButton_pressed();
    void on_mic_voiceAssistButton_released();


private:
    std::unique_ptr<Ui::MicrophonePage> mUi;
    const QAndroidUserEventAgent* mUserEventAgent;
};
