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

#include "android/skin/qt/emulator-qt-no-window.h"

#include "android/base/memory/LazyInstance.h"
#include "android/base/async/ThreadLooper.h"
#include "android/globals.h"
#include "android/skin/qt/QtLooper.h"

#include <QObject>
#include <QThread>
#include <QtCore>
#include <QCoreApplication>

#define DEBUG 0

#if DEBUG
#define D(...) qDebug(__VA_ARGS__)
#else
#define D(...) ((void)0)
#endif

static android::base::LazyInstance<EmulatorQtNoWindow::Ptr> sNoWindowInstance =
        LAZY_INSTANCE_INIT;

Task::Task(std::function<void()> f) : fptr(f) {}

void Task::run() {
    fptr();
    D("Task finished, notify connected slots");
    emit finished();
}

/******************************************************************************/

void EmulatorQtNoWindow::create() {
    sNoWindowInstance.get() = Ptr(new EmulatorQtNoWindow());
}

EmulatorQtNoWindow::EmulatorQtNoWindow(QObject* parent)
    : mLooper(android::qt::createLooper()),
      mAdbInterface(android::emulation::AdbInterface::create(mLooper)) {
    android::base::ThreadLooper::setLooper(mLooper, true);
    QObject::connect(QCoreApplication::instance(),
                     &QCoreApplication::aboutToQuit, this,
                     &EmulatorQtNoWindow::slot_clearInstance);
    QObject::connect(this, &EmulatorQtNoWindow::requestClose, this,
                     &EmulatorQtNoWindow::slot_requestClose);

}

EmulatorQtNoWindow::Ptr EmulatorQtNoWindow::getInstancePtr() {
    return sNoWindowInstance.get();
}

EmulatorQtNoWindow* EmulatorQtNoWindow::getInstance() {
    return getInstancePtr().get();
}

void EmulatorQtNoWindow::startThread(std::function<void()> f) {
    auto thread = new QThread();
    auto task = new Task(f);

    // pass task object to thread and start task when thread starts
    task->moveToThread(thread);
    connect(thread, SIGNAL(started()), task, SLOT(run()));
    // when the task is finished, signal the thread to quit
    connect(task, SIGNAL(finished()), thread, SLOT(quit()));
    // queue up task object for deletion when the thread is finished
    connect(thread, SIGNAL(finished()), task, SLOT(deleteLater()));
    // queue up thread object for deletion when the thread is finished
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    // when the thread is finished, quit this GUI-less window too
    connect(thread, SIGNAL(finished()), getInstance(), SLOT(slot_finished()));

    thread->start();
}

extern "C" void qemu_system_shutdown_request(void);

void EmulatorQtNoWindow::slot_requestClose() {
    if (mRunning) {
        mRunning = false;
        if (savevm_on_exit) {
            qemu_system_shutdown_request();
        } else {
            mAdbInterface->runAdbCommand(
                    {"shell", "reboot", "-p"},
                    [](const android::emulation::OptionalAdbCommandResult&) {
                        qemu_system_shutdown_request();
                    },
                    5000, false);
        }
    }
}

void EmulatorQtNoWindow::slot_clearInstance() {
    sNoWindowInstance.get().reset();
}

void EmulatorQtNoWindow::slot_finished() {
    D("Closing GUI-less window, quiting application");
    QCoreApplication::instance()->quit();
}
