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

#include "android/base/StringView.h"
#include "android/emulation/control/GooglePlayServices.h"
#include "ui_google-play-page.h"

#include <QTimer>
#include <QWidget>
#include <memory>

class GooglePlayPage : public QWidget {
    Q_OBJECT
public:
    enum class PlayPages {
        ServicesDetailsPage,
    };

    enum class PlayApps {
        PlayServices,
    };

    explicit GooglePlayPage(QWidget* parent = 0);
    ~GooglePlayPage();

    void initialize(android::emulation::AdbInterface* adb);

private:
    void showPlayServicesPage();
    void playPageDone(android::emulation::GooglePlayServices::Result result,
                      PlayPages page);
    static QString getPlayAppDescription(PlayApps app);
    static QString getPlayPageDescription(PlayPages page);
    void bootCompletionPropertyDone(
            android::emulation::GooglePlayServices::Result result,
            android::base::StringView outString);
    void getPlayServicesVersion();
    void playVersionDone(android::emulation::GooglePlayServices::Result result,
                         PlayApps app,
                         android::base::StringView outString);

private slots:
    void on_goog_updateServicesButton_clicked();

    void getBootCompletionProperty();
    void queryPlayVersions();

private:
    static const std::pair<PlayApps, const char*> PlayAppToDesc[];
    static const std::pair<PlayPages, const char*> PlayPageToDesc[];

    QTimer mTimer;
    std::unique_ptr<android::emulation::GooglePlayServices> mGooglePlayServices;
    std::unique_ptr<Ui::GooglePlayPage> mUi;
};
