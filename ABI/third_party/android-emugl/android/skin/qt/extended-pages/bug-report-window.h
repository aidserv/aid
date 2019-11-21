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

#include "android/emulation/control/AdbBugReportServices.h"
#include "android/skin/qt/emulator-qt-window.h"

#include "ui_bug-report-window.h"

#include <QFrame>
#include <QMessageBox>
#include <QThread>

#include <memory>
#include <string>

class BugReportWindow : public QFrame {
    Q_OBJECT

public:
    explicit BugReportWindow(EmulatorQtWindow* eW, QWidget* parent = 0);
    void showEvent(QShowEvent* event);
    void updateTheme();
    struct ReportingFields {
        std::string androidVer;
        std::string avdDetails;
        std::string cpuModel;
        std::string deviceName;
        std::string emulatorVer;
        std::string hypervisorVer;
        std::string hostOsName;
        std::string reproSteps;
        std::string sdkToolsVer;
    };

    struct SavingStates {
        std::string saveLocation;
        std::string adbBugreportFilePath;
        std::string screenshotFilePath;
        std::string bugreportFolderPath;
        bool adbBugreportSucceed;
        bool screenshotSucceed;
        bool bugreportSavedSucceed;
    };

private slots:
    void on_bug_saveButton_clicked();
    void on_bug_fileBugButton_clicked();
    void saveBugreportFolderFinished(bool success,
                                     QString folderPath,
                                     bool willOpenIssueTracker);
    void saveBugreportFolderStarted();
    void issueTrackerTaskFinished(bool success, QString error);

private:
    void loadAdbBugreport();
    void loadAdbLogcat();
    void loadAvdDetails();
    void loadCircularSpinner(SettingsTheme theme);
    void loadScreenshotImage();
    bool eventFilter(QObject* object, QEvent* event) override;
    void saveBugReportFolder(bool willOpenIssueTracker);
    void launchIssueTrackerThread();

    EmulatorQtWindow* mEmulatorWindow;
    android::emulation::AdbInterface* mAdb;
    android::emulation::AdbBugReportServices mBugReportServices;
    android::emulation::ScreenCapturer* mScreenCapturer;
    QMessageBox* mDeviceDetailsDialog;
    bool mFirstShowEvent = true;
    std::unique_ptr<Ui::BugReportWindow> mUi;
    ReportingFields mReportingFields;
    SavingStates mSavingStates;
};

class BugReportFolderSavingTask : public QObject {
    Q_OBJECT
public:
    BugReportFolderSavingTask(std::string savingPath,
                              std::string adbBugreportFilePath,
                              std::string screenshotFilePath,
                              std::string avdDetails,
                              std::string reproSteps,
                              bool openIssueTracker);
public slots:
    void run();

signals:
    void started();
    void finished(bool success, QString folderPath, bool openIssueTracker);

private:
    std::string mSavingPath;
    std::string mAdbBugreportFilePath;
    std::string mScreenshotFilePath;
    std::string mAvdDetails;
    std::string mReproSteps;
    bool mOpenIssueTracker;
};

class IssueTrackerTask : public QObject {
    Q_OBJECT
public:
    IssueTrackerTask(BugReportWindow::ReportingFields reportingField);

public slots:
    void run();
signals:
    void finished(bool success, QString error);

private:
    BugReportWindow::ReportingFields mReportingFields;
};
