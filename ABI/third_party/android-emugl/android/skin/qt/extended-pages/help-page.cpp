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

#include "android/skin/qt/extended-pages/help-page.h"

#include "android/android.h"
#include "android/base/system/System.h"
#include "android/globals.h"
#include "android/update-check/UpdateChecker.h"
#include "android/update-check/VersionExtractor.h"

#include <QDesktopServices>
#include <QThread>
#include <QUrl>

#include <cassert>

const char DOCS_URL[] =
    "http://developer.android.com/r/studio-ui/emulator.html";
const char SEND_FEEDBACK_URL[] =
    "https://issuetracker.google.com/issues/new?component=192727&template=872501";

HelpPage::HelpPage(QWidget* parent) : QWidget(parent), mUi(new Ui::HelpPage) {
    mUi->setupUi(this);

    // Get the version of this code
    android::update_check::VersionExtractor vEx;

    android::base::Version curVersion = vEx.getCurrentVersion();
    auto verStr = curVersion.isValid()
            ? QString(curVersion.toString().c_str()) : "Unknown";

    mUi->help_versionBox->setPlainText(verStr);

    int apiLevel = avdInfo_getApiLevel(android_avdInfo);
    char versionString[128];
    avdInfo_getFullApiName(apiLevel, versionString, 128);
    mUi->help_androidVersionBox->setPlainText(versionString);

    // launch the latest version loader in a separate thread
    auto latestVersionThread = new QThread();
    auto latestVersionTask = new LatestVersionLoadTask();
    latestVersionTask->moveToThread(latestVersionThread);
    connect(latestVersionThread, SIGNAL(started()), latestVersionTask, SLOT(run()));
    connect(latestVersionTask, SIGNAL(finished(QString)),
            mUi->help_latestVersionBox, SLOT(setPlainText(QString)));
    connect(latestVersionTask, SIGNAL(finished(QString)), latestVersionThread, SLOT(quit()));
    connect(latestVersionThread, SIGNAL(finished()), latestVersionTask, SLOT(deleteLater()));
    connect(latestVersionThread, SIGNAL(finished()), latestVersionThread, SLOT(deleteLater()));
    mUi->help_latestVersionBox->setPlainText(tr("Loading..."));
    latestVersionThread->start();

    QObject::connect(this, &HelpPage::adbPortUpdateRequired,
                     this, &HelpPage::updateAdbPortNumber);
}

void HelpPage::initialize(const ShortcutKeyStore<QtUICommand>* key_store,
                          EmulatorQtWindow* eW) {
    initializeLicenseText();
    initializeKeyboardShortcutList(key_store);
    mEmulatorWindow = eW;
}

void HelpPage::initializeLicenseText() {
    // Read the license text into the display box
    // The file is <SDK path>/tools/NOTICE.txt

    QString lFileName = android::base::System::get()->getLauncherDirectory().c_str();
    lFileName += "/NOTICE.txt";

    QFile licenseFile(lFileName);
    if (licenseFile.open(QIODevice::ReadOnly)) {
        // Read the file into the text box
        QTextStream lText(&licenseFile);
        mUi->help_licenseText->setPlainText(lText.readAll());
    } else {
        // Can't read the file. Give a backup notice.
        mUi->help_licenseText->setPlainText(
                tr("Find Android Emulator License NOTICE files here:") +
                "\n\nhttps://android.googlesource.com/platform/external/"
                          "qemu/+/emu-master-dev/");
    }
}

static void addShortcutsTableRow(QTableWidget* table_widget,
                                 const QString& key_sequence,
                                 const QString& description) {
    int table_row = table_widget->rowCount();
    table_widget->insertRow(table_row);
    table_widget->setItem(table_row, 0, new QTableWidgetItem(description));
    table_widget->setItem(table_row, 1, new QTableWidgetItem(key_sequence));
}

void HelpPage::initializeKeyboardShortcutList(const ShortcutKeyStore<QtUICommand>* key_store)
{
    QTableWidget* table_widget = mUi->shortcutsTableWidget;
    if (key_store) {
        for (auto key_sequence_and_command = key_store->begin();
             key_sequence_and_command != key_store->end();
             ++key_sequence_and_command) {
            QString key_combo;

            // Unfortunately, QKeySequence doesn't handle modifier-only key
            // sequences very well. In this case, "multitouch" is Ctrl and
            // QKeySequence::toString sometimes produces strings with weird
            // characters. To mitigate this problem, we simply hardcode the
            // string for the "multitouch" key combo.
            if (key_sequence_and_command.value() ==
                       QtUICommand::SHOW_MULTITOUCH) {
#ifdef Q_OS_MAC
                key_combo = "\u2318";  // Cmd
#else
                key_combo = "Ctrl";
#endif
            } else {
                key_combo  = key_sequence_and_command.key().toString(QKeySequence::NativeText);
            }

            addShortcutsTableRow(table_widget,
                                 key_combo,
                                 getQtUICommandDescription(key_sequence_and_command.value()));
        }
    }

    table_widget->sortItems(0);
}

void HelpPage::setAdbPort() {
    // The UI can only be updated from the proper thread.
    // Emit a signal so that happens now, but in the right
    // context.
    emit adbPortUpdateRequired();
}

void HelpPage::updateAdbPortNumber() {
    // Show the "serial number" that can be used to connect ADB
    // to this device
    mUi->help_adbSerialNumberBox->setPlainText(
            "emulator-" + QString::number(android_serial_number_port) );
}

BugReportWindow* HelpPage::getBugreportWindow() {
    return mBugReportWindow;
}

void HelpPage::on_help_docs_clicked() {
    QDesktopServices::openUrl(QUrl::fromEncoded(DOCS_URL));
}

void HelpPage::createBugReportWindow() {
    mBugReportWindow = new BugReportWindow(mEmulatorWindow, this);
}

void HelpPage::on_help_fileBug_clicked() {
    if (mBugReportWindow == nullptr) {
        createBugReportWindow();
    }
    assert(mBugReportWindow);
    mBugReportWindow->show();
    mBugReportWindow->raise();
    mBugReportWindow->activateWindow();
}

void HelpPage::on_help_sendFeedback_clicked() {
    QDesktopServices::openUrl(QUrl::fromEncoded(SEND_FEEDBACK_URL));
}

static const char* updateChannelName(android::studio::UpdateChannel channel) {
    switch (channel) {
        case android::studio::UpdateChannel::Stable:
            return "Stable";
        case android::studio::UpdateChannel::Dev:
            return "Dev";
        case android::studio::UpdateChannel::Beta:
            return "Beta";
        case android::studio::UpdateChannel::Canary:
            return "Canary";
        default:
            return nullptr;
    }
}

void LatestVersionLoadTask::run() {
    // Get latest version that is available online
    android::update_check::UpdateChecker upCheck;
    const auto latestVersion = upCheck.getLatestVersion();
    QString latestVerString;
    if (!latestVersion) {
        latestVerString = tr("Unavailable");
    } else {
        latestVerString =
                QString::fromStdString(latestVersion->first.toString());
        if (const auto channelName = updateChannelName(latestVersion->second)) {
            latestVerString += tr(" (%1 update channel)").arg(channelName);
        }
    }
    emit finished(latestVerString);
}
