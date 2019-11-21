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

#include "ui_record-screen-page.h"
#include <QTimer>
#include <QWidget>
#include <memory>

//struct QAndroidRecordScreenAgent;
class RecordScreenPage : public QWidget
{
    Q_OBJECT
public:
    enum class RecordState {
      Ready,
      Recording,
      Stopped
    };

    explicit RecordScreenPage(QWidget *parent = 0);
    ~RecordScreenPage();

//    void setRecordScreenAgent(const QAndroidRecordScreenAgent* agent);

signals:

private slots:
	void on_rec_recordButton_clicked();
	void updateElapsedTime();

public slots:

public:
	void setRecordState(RecordState r);

private:
    std::unique_ptr<Ui::RecordScreenPage> mUi;
//    const QAndroidRecordScreenAgent* mRecordScreenAgent;
    RecordState mState;
    QTimer mTimer;
    int mSec; // number of elapsed seconds
};
