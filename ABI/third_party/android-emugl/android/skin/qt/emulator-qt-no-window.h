/* Copyright (C) 2015 The Android Open Source Project
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

#pragma once

#include "android/base/async/Looper.h"
#include "android/emulation/control/AdbInterface.h"

#include <QObject>

#include <memory>

// Task is a simple wrapper class, used to move the execution
// of function f to a separate QT Thread

class Task : public QObject {
    Q_OBJECT

public:
    Task(std::function<void()> f);

public slots:
    void run();

signals:
    void finished();

private:
    std::function<void()> fptr;
};

// The EmulatorQTNoWindow class is used to build a QT-Widget-less
// event loop when the parameter -no-window is passed to the android
// emulator. Much like EmulatorQtWindow, startThread(f) will spawn
// a new QT Thread to execute function f (qemu main loop).

class EmulatorQtNoWindow final : public QObject {
    Q_OBJECT

public:
    using Ptr = std::shared_ptr<EmulatorQtNoWindow>;

    static void create();
    static EmulatorQtNoWindow* getInstance();
    static Ptr getInstancePtr();

    void startThread(std::function<void()> f);

signals:
    void requestClose();

private:
    explicit EmulatorQtNoWindow(QObject* parent = 0);

private slots:
    void slot_clearInstance();
    void slot_finished();
    void slot_requestClose();

private:
    android::base::Looper* mLooper;
    std::unique_ptr<android::emulation::AdbInterface> mAdbInterface;
    bool mRunning = true;
};
