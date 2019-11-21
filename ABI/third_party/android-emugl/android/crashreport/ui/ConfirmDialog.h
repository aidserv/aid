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

#include "android/skin/qt/qt-settings.h"
#include "android/crashreport/CrashService.h"

#include <memory>

#include <QDialog>

namespace Ui { class ConfirmDialog; }

// QT Component that displays the send crash dump confirmation
class ConfirmDialog : public QDialog {
    Q_OBJECT

public:
    ConfirmDialog(QWidget* parent,
                  android::crashreport::CrashService* crashservice,
                  Ui::Settings::CRASHREPORT_PREFERENCE_VALUE reportPreference,
                  const char* reportingDir);
    ~ConfirmDialog();

public slots:
    void sendReport();
    void dontSendReport();
    void toggleDetails();

private:
    void enableInput(bool enable);
    void showProgressBar(const QString& msg);
    void hideProgressBar();
    void getDetails();
    bool uploadCrash();
    void addSuggestion(const QString& str);
    void hideDetails(bool firstTime = false);
    void showDetails();
    void setSwGpu();
    QString constructDumpMessage() const;

    std::unique_ptr<Ui::ConfirmDialog> mUi;
    android::crashreport::CrashService* mCrashService;
    const char* mReportingDir;  // Directory containing the temporary files
    int mHeightWithDetails = 0;
    Ui::Settings::CRASHREPORT_PREFERENCE_VALUE mReportPreference;
    bool mDidGetSysInfo = false;
    bool mDidUpdateDetails = false;
};
