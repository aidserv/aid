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

#include "android/base/containers/CircularBuffer.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/skin/event.h"
#include "android/skin/qt/extended-window-styles.h"
#include "android/skin/qt/extended-window.h"
#include "android/skin/qt/qt-ui-commands.h"
#include "android/skin/qt/set-ui-emu-agent.h"
#include "android/skin/qt/shortcut-key-store.h"
#include "android/skin/qt/size-tweaker.h"
#include "android/skin/qt/ui-event-recorder.h"
#include "android/skin/qt/user-actions-counter.h"
#include "android/utils/compiler.h"

#include "ui_tools.h"

#include <QDir>
#include <QErrorMessage>
#include <QFrame>
#include <QFrame>
#include <QGridLayout>
#include <QHash>
#include <QKeyEvent>
#include <QMap>
#include <QMessageBox>
#include <QPair>
#include <QProcess>
#include <QQueue>
#include <QToolButton>
#include <QUrl>

#include <memory>

#define REMOTE_SCREENSHOT_FILE "/data/local/tmp/screen.png"

class EmulatorQtWindow;
class ExtendedWindow;

typedef void (EmulatorQtWindow::*EmulatorQtWindowSlot)();

class ToolWindow : public QFrame {
    Q_OBJECT
    using UIEventRecorderPtr =
            std::weak_ptr<UIEventRecorder<android::base::CircularBuffer>>;
    using UserActionsCounterPtr =
            std::weak_ptr<android::qt::UserActionsCounter>;

public:
    ToolWindow(EmulatorQtWindow* emulatorWindow,
               QWidget* parent,
               UIEventRecorderPtr event_recorder,
               UserActionsCounterPtr user_actions_counter);
    ~ToolWindow();

    void hide();
    void show();
    void dockMainWindow();
    void raiseMainWindow();
    void setLastExtendedWindowPane(ExtendedWindowPane pane);

    void setToolEmuAgent(const UiEmuAgent* agPtr);
    const UiEmuAgent* getUiEmuAgent() const { return mUiEmuAgent; }

    bool handleQtKeyEvent(QKeyEvent* event);

    void closeExtendedWindow();

    // Observed only on Windows:
    // Whenever we set the window flags for the EmulatorContainer,
    // we receive a focusInEvent, which raises the tool window. We
    // need to raise the extended window after the tool window is
    // raised, which is what this function is for.
    void notifySwitchOnTop();

    // The designers want a gap between the main emulator
    // window and the tool bar. This is how big that gap is.
    static const int toolGap = 10;

signals:
    void guestClipboardChanged(QString text);
    void haveClipboardSharingKnown(bool have);

private:
    void createExtendedWindow();
    void handleUICommand(QtUICommand cmd, bool down);
    void forwardKeyToEmulator(uint32_t keycode, bool down);

    // Helper method, calls handleUICommand with
    // down equal to true and down equal to false.
    void handleUICommand(QtUICommand cmd) {
        handleUICommand(cmd, true);
        handleUICommand(cmd, false);
    }

    QToolButton* addButton(QGridLayout* layout,
                           int row,
                           int col,
                           const char* iconPath,
                           QString tip,
                           EmulatorQtWindowSlot slot);
    void showOrRaiseExtendedWindow(ExtendedWindowPane pane);

    virtual void closeEvent(QCloseEvent* ce) override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void paintEvent(QPaintEvent*) override;
    virtual void hideEvent(QHideEvent* event) override;

    EmulatorQtWindow* mEmulatorWindow;
    // ExtendedWindow has slots with subscribers, so use deleteLater() instead
    // of regular delete for it.
    android::base::ScopedCustomPtr<ExtendedWindow, void (*)(QObject*)>
            mExtendedWindow;
    const UiEmuAgent* mUiEmuAgent;
    std::unique_ptr<Ui::ToolControls> mToolsUi;
    bool mStartedAdbStopProcess = false;
    ShortcutKeyStore<QtUICommand> mShortcutKeyStore;
    bool mIsExtendedWindowVisibleOnShow = false;
    QString mDetectedAdbPath;
    UIEventRecorderPtr mUIEventRecorder;
    UserActionsCounterPtr mUserActionsCounter;
    SizeTweaker mSizeTweaker;
    bool mTopSwitched = false;

public slots:
    void raise();
    void switchClipboardSharing(bool enabled);

private slots:
    void on_back_button_pressed();
    void on_back_button_released();
    void on_close_button_clicked();
    void on_home_button_pressed();
    void on_home_button_released();
    void on_minimize_button_clicked();
    void on_more_button_clicked();
    void on_power_button_pressed();
    void on_power_button_released();
    void on_overview_button_pressed();
    void on_overview_button_released();
    void on_prev_layout_button_clicked();
    void on_next_layout_button_clicked();
    void on_scrShot_button_clicked();
    void on_volume_down_button_pressed();
    void on_volume_down_button_released();
    void on_volume_up_button_pressed();
    void on_volume_up_button_released();
    void on_zoom_button_clicked();

    void onGuestClipboardChanged(QString text);
    void onHostClipboardChanged();
};

typedef void (ToolWindow::*ToolWindowSlot)();
