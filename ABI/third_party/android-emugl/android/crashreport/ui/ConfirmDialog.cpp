/* Copyright (C) 2015-2017 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "android/crashreport/ui/ConfirmDialog.h"

#include "android/android.h"
#include "android/base/files/IniFile.h"
#include "android/base/files/PathUtils.h"
#include "android/crashreport/CrashReporter.h"
#include "android/globals.h"

#include "ui_ConfirmDialog.h"

#include <QEventLoop>
#include <QFutureWatcher>
#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QtConcurrent/QtConcurrentRun>

static const char kIconFile[] = "emulator_icon_128.png";

extern "C" const unsigned char* android_emulator_icon_find(const char* name,
                                                           size_t* psize);

using android::base::IniFile;
using android::base::PathUtils;
using android::crashreport::CrashService;
using Ui::Settings::CRASHREPORT_PREFERENCE_VALUE;

// Layout doesn't have show()/hide() methods, one has to manually enumerate all
// its widgets for that.
static void makeVisible(QLayout* layout, bool visible = true) {
    for (int i = 0; i < layout->count(); ++i) {
        const auto widget = layout->itemAt(i)->widget();
        visible ? widget->show() : widget->hide();
    }
}

ConfirmDialog::ConfirmDialog(QWidget* parent,
                             CrashService* crashservice,
                             CRASHREPORT_PREFERENCE_VALUE reportPreference,
                             const char* reportingDir)
    : QDialog(parent),
      mUi(new Ui::ConfirmDialog()),
      mCrashService(crashservice),
      mReportingDir(reportingDir),
      mReportPreference(reportPreference) {
    mUi->setupUi(this);

    mUi->yesNoButtonBox->button(QDialogButtonBox::Ok)->setText(
                tr("Send report"));
    mUi->yesNoButtonBox->button(QDialogButtonBox::Cancel)->setText(
                tr("Don't send"));

    mUi->label->setText(constructDumpMessage());

    QSettings settings;
    bool save_preference_checked =
            settings.value(Ui::Settings::CRASHREPORT_SAVEPREFERENCE_CHECKED, 1)
                    .toInt();
    mUi->savePreference->setChecked(
            save_preference_checked ||
            mReportPreference == Ui::Settings::CRASHREPORT_PREFERENCE_ALWAYS);

    size_t icon_size;
    QPixmap icon;
    const unsigned char* icon_data =
            android_emulator_icon_find(kIconFile, &icon_size);

    icon.loadFromData(icon_data, icon_size);

    mUi->icon->setPixmap(icon);

    crashservice->processCrash();
    auto suggestions = crashservice->getSuggestions().suggestions;
    bool haveGfxFailure = false;
    if (!suggestions.empty()) {
        if (suggestions.find(
                    android::crashreport::Suggestion::UpdateGfxDrivers) !=
            suggestions.end()) {
            haveGfxFailure = true;
            addSuggestion(
                    tr("It appears that your computer's OpenGL graphics driver "
                       "crashed. This was probably caused by a bug in the "
                       "driver or by a bug in your app's OpenGL code.\n\n"
                       "You should check your manufacturer's website for an "
                       "updated graphics driver.\n\n"
                       "You can also tell the Emulator to use software "
                       "rendering for this device. This could avoid driver "
                       "problems and make it easier to debug the OpenGL code "
                       "in your app."
#ifdef __APPLE__
                       " (You may need to reduce your screen resolution to run "
                       "this way.)"
#endif
                       ));
        }
    } else {
        mUi->suggestions->hide();
    }

    if (!haveGfxFailure) {
        mUi->softwareGpu->hide();
    }

    setWindowIcon(icon);
    hideDetails(true);
    hideProgressBar();
}

ConfirmDialog::~ConfirmDialog() = default;

void ConfirmDialog::enableInput(bool enable) {
    mUi->yesNoButtonBox->setEnabled(enable);
    mUi->detailsButton->setEnabled(enable);
    mUi->commentsText->setEnabled(enable);
    mUi->savePreference->setEnabled(enable);
}

void ConfirmDialog::hideDetails(bool firstTime) {
    makeVisible(mUi->detailsLayout, false);
    mUi->detailsButton->setText(tr("Show details"));
    if (!firstTime) {
        mHeightWithDetails = height();
    }
}

void ConfirmDialog::getDetails() {
    if (!mDidGetSysInfo) {
        enableInput(false);

        showProgressBar(tr("Collecting crash info... this may take a minute."));
        QEventLoop eventloop;

        QFutureWatcher<bool> watcher;
        connect(&watcher, SIGNAL(finished()), &eventloop, SLOT(quit()));

        // Start the computation.
        QFuture<bool> future = QtConcurrent::run(
                mCrashService, &CrashService::collectSysInfo);
        watcher.setFuture(future);

        eventloop.exec();

        hideProgressBar();
        enableInput(true);
    }
    mDidGetSysInfo = true;
}

void ConfirmDialog::showDetails() {
    getDetails();
    if (!mDidUpdateDetails) {
        QString details = QString::fromStdString(mCrashService->getReport());
        details += QString::fromStdString(mCrashService->getSysInfo());

        mUi->detailsText->document()->setPlainText(details);
        mDidUpdateDetails = true;
    }

    mUi->detailsButton->setText(tr("Hide details"));
    makeVisible(mUi->detailsLayout);
    mUi->detailsText->verticalScrollBar()->setValue(
            mUi->detailsText->verticalScrollBar()->minimum());
    if (mHeightWithDetails) {
        resize(width(), mHeightWithDetails);
    }
}

void ConfirmDialog::addSuggestion(const QString& str) {
    QString next_text = mUi->suggestions->text().append(str).append('\n');
    mUi->suggestions->setText(next_text);
    mUi->suggestions->adjustSize();
    mUi->suggestions->setFixedHeight(mUi->suggestions->height());
}

void ConfirmDialog::showProgressBar(const QString& msg) {
    mUi->progressLabel->setText(msg);
    makeVisible(mUi->progressLayout);
}

void ConfirmDialog::hideProgressBar() {
    makeVisible(mUi->progressLayout, false);
}

bool ConfirmDialog::uploadCrash() {
    enableInput(false);
    showProgressBar(tr("Sending crash report..."));
    QEventLoop eventloop;

    QFutureWatcher<bool> watcher;
    connect(&watcher, SIGNAL(finished()), &eventloop, SLOT(quit()));

    // Start the computation.
    QFuture<bool> future = QtConcurrent::run(
            mCrashService, &CrashService::uploadCrash);
    watcher.setFuture(future);

    eventloop.exec();

    hideProgressBar();

    return watcher.result();
}

static void savePref(bool checked, CRASHREPORT_PREFERENCE_VALUE v) {
    QSettings settings;
    settings.setValue(Ui::Settings::CRASHREPORT_PREFERENCE,
            checked ? v : Ui::Settings::CRASHREPORT_PREFERENCE_ASK);
    settings.setValue(Ui::Settings::CRASHREPORT_SAVEPREFERENCE_CHECKED,
                      checked);
}

void ConfirmDialog::sendReport() {
    getDetails();
    mCrashService->addUserComments(
                mUi->commentsText->toPlainText().toStdString());
    bool upload_success = uploadCrash();

    if (upload_success &&
        (mReportPreference == Ui::Settings::CRASHREPORT_PREFERENCE_ASK)) {
        QMessageBox msgbox(this);
        msgbox.setWindowTitle(tr("Crash Report Submitted"));
        msgbox.setText(tr("<p>Thank you for submitting a crash report!</p>"
                          "<p>If you would like to contact us for further "
                          "information, "
                          "use the following Crash Report ID:</p>"
                          "<p></p>"
                          "<p>%1</p>")
                               .arg(QString::fromStdString(
                                       mCrashService->getReportId())));
        msgbox.setTextInteractionFlags(Qt::TextSelectableByMouse |
                                       Qt::TextSelectableByKeyboard);
        msgbox.exec();
    }

    setSwGpu();

    savePref(mUi->savePreference->isChecked(),
             Ui::Settings::CRASHREPORT_PREFERENCE_ALWAYS);
    accept();
}

void ConfirmDialog::dontSendReport() {
    setSwGpu();
    reject();
}

// If the user requests a switch to software GPU, modify
// config.ini
void ConfirmDialog::setSwGpu() {
    if (!mUi->softwareGpu->isChecked() || !mReportingDir) {
        return;
    }

    // The user wants the switch and we have the path to the avd_info.txt file
    std::string avdInfoPath =
            PathUtils::join(mReportingDir, CRASH_AVD_HARDWARE_INFO);

    IniFile iniF(avdInfoPath);
    iniF.read();

    // Get the path to config.ini
    std::string diskPartDir = iniF.getString("disk.dataPartition.path", "");
    if ( !diskPartDir.empty() ) {
        // Keep the path; discard the file name
        android::base::StringView outputDir;
        bool isOK = PathUtils::split(diskPartDir, &outputDir, nullptr);
        if (isOK) {
            std::string hwQemuPath =
                    PathUtils::join(outputDir, CORE_CONFIG_INI);
            // We have the path to config.ini
            // Read that
            IniFile hwQemuIniF(hwQemuPath);
            hwQemuIniF.read();

            // Set hw.gpu.enabled=no and hw.gpu.mode=guest
            hwQemuIniF.setString("hw.gpu.enabled", "no");
            hwQemuIniF.setString("hw.gpu.mode", "guest");

            // Write the modified configuration back
            hwQemuIniF.write();
        }
    }
}

void ConfirmDialog::toggleDetails() {
    if (mUi->detailsText->isVisible()) {
        hideDetails();
    } else {
        showDetails();
    }
}

QString ConfirmDialog::constructDumpMessage() const {
    QString dumpMessage =
            QString::fromStdString(mCrashService->getCustomDumpMessage());
    if (dumpMessage.isEmpty()) {
        dumpMessage = tr("<p>Android Emulator closed unexpectedly.</p>");
    } else {
        dumpMessage = tr("<p>Android Emulator closed because of an internal "
                         "error:</p>") +
                      "<p>" + dumpMessage + "</p>";
    }
    dumpMessage +=
            tr("<p>Do you want to send a crash report about the problem?</p>");
    return dumpMessage;
}
