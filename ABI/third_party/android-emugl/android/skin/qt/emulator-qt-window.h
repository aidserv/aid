/* Copyright (C) 2015-2016 The Android Open Source Project
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
#include "android/base/StringView.h"
#include "android/base/synchronization/Lock.h"
#include "android/emulation/control/ApkInstaller.h"
#include "android/emulation/control/FilePusher.h"
#include "android/emulation/control/ScreenCapturer.h"
#include "android/globals.h"
#include "android/metrics/PeriodicReporter.h"
#include "android/skin/event.h"
#include "android/skin/qt/emulator-container.h"
#include "android/skin/qt/emulator-overlay.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/tool-window.h"
#include "android/skin/qt/ui-event-recorder.h"
#include "android/skin/qt/user-actions-counter.h"
#include "android/skin/surface.h"
#include "android/skin/winsys.h"

#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFrame>
#include <QImage>
#include <QMessageBox>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QObject>
#include <QPainter>
#include <QProgressDialog>
#include <QResizeEvent>
#include <QWidget>
#include <QtCore>

#include <memory>
#include <string>
#include <vector>

namespace Ui {
class EmulatorWindow;
}

typedef struct SkinSurface SkinSurface;

class MainLoopThread : public QThread {
    Q_OBJECT

public:
    MainLoopThread(StartFunction f, int argc, char** argv)
        : start_function(f), argc(argc), argv(argv) {}
    void run() Q_DECL_OVERRIDE {
        if (start_function)
            start_function(argc, argv);
    }

private:
    StartFunction start_function;
    int argc;
    char** argv;
};

class EmulatorQtWindow final : public QFrame {
    Q_OBJECT

public:
    using Ptr = std::shared_ptr<EmulatorQtWindow>;

private:
    explicit EmulatorQtWindow(QWidget* parent = 0);

public:
    static void create();
    static EmulatorQtWindow* getInstance();
    static Ptr getInstancePtr();

    virtual ~EmulatorQtWindow();

    void queueQuitEvent();
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void startThread(StartFunction f, int argc, char** argv);

    /*
     In Qt, signals are normally events of interest that a class can emit, which
     can be hooked up to arbitrary slots. Here
     we use this mechanism for a different purpose: it's to allow the QEMU
     thread
     to request an operation be performed on
     the Qt thread; Qt allows signals to be emitted from any thread. When used
     in
     this fashion, the signal is queued and
     handled asyncrhonously. Since we sometimes will call these signals from
     Qt's
     thread as well, we can't use
     BlockingQueuedConnections for these signals, since this connection type
     will
     deadlock if called from the same thread.
     For that reason, we use a normal non-blocking connection type, and allow
     all
     of the signals to pass an optional semaphore
     that will be released by the slot when it is done processing. If you want
     to
     block on the completion of the signal, simply
     pass in the semaphore to the signal and acquire it after the call returns.
     If
     you're passing in pointers to data structures
     that could change or go away, you will need to make sure you block to
     maintain the integrity of the data while the signal runs.

     TODO: allow nonblocking calls to these signals by having the signal take
     ownership of object pointers. This would allow QEMU
     to do things like update the screen without blocking, which would make it
     run
     faster.
     */
signals:
    void blit(QImage* src,
              QRect srcRect,
              QImage* dst,
              QPoint dstPos,
              QPainter::CompositionMode op,
              QSemaphore* semaphore = NULL);
    void createBitmap(SkinSurface* s,
                      int w,
                      int h,
                      QSemaphore* semaphore = NULL);
    void fill(SkinSurface* s,
              QRect rect,
              QColor color,
              QSemaphore* semaphore = NULL);
    void getBitmapInfo(SkinSurface* s,
                       SkinSurfacePixels* pix,
                       QSemaphore* semaphore = NULL);
    void getDevicePixelRatio(double* out_dpr, QSemaphore* semaphore = NULL);
    void getScreenDimensions(QRect* out_rect, QSemaphore* semaphore = NULL);
    void getFramePos(int* x, int* y, QSemaphore* semaphore = NULL);
    void getWindowPos(int* x, int* y, QSemaphore* semaphore = NULL);
    void isWindowFullyVisible(bool* out_value, QSemaphore* semaphore = NULL);
    void releaseBitmap(SkinSurface* s, QSemaphore* sempahore = NULL);
    void requestClose(QSemaphore* semaphore = NULL);
    void requestUpdate(QRect rect, QSemaphore* semaphore = NULL);
    void setDeviceGeometry(QRect rect, QSemaphore* sempahore = NULL);
    void setWindowIcon(const unsigned char* data,
                       int size,
                       QSemaphore* semaphore = NULL);
    void setWindowPos(int x, int y, QSemaphore* semaphore = NULL);
    void setTitle(QString title, QSemaphore* semaphore = NULL);
    void showWindow(SkinSurface* surface,
                    QRect rect,
                    QSemaphore* semaphore = NULL);

    // Qt doesn't support function pointers in signals/slots natively, but
    // pointer to function pointer works fine
    void runOnUiThread(SkinGenericFunction* f,
                       void* data,
                       QSemaphore* semaphore = NULL);
    void updateRotation(SkinRotation rotation);
    void layoutChanged(SkinRotation rot);

public:
    void pollEvent(SkinEvent* event,
                   bool* hasEvent);

    WId getWindowId();

    android::emulation::AdbInterface* getAdbInterface() const;
    android::emulation::ScreenCapturer* getScreenCapturer();
    bool isInZoomMode() const;
    ToolWindow* toolWindow() const;
    void showZoomIfNotUserHidden();
    QSize containerSize() const;
    QRect deviceGeometry() const;

    void doResize(const QSize& size,
                  bool isKbdShortcut = false);
    void handleMouseEvent(SkinEventType type,
                          SkinMouseButtonType button,
                          const QPoint& pos,
                          bool skipSync = false);
    void panHorizontal(bool left);
    void panVertical(bool up);
    void queueSkinEvent(SkinEvent* event);
    void recenterFocusPoint();
    void saveZoomPoints(const QPoint& focus, const QPoint& viewportFocus);
    void scaleDown();
    void scaleUp();
    void screenshot();
    void setFrameAlways(bool showFrame);
    void setOnTop(bool onTop);
    void simulateKeyPress(int keyCode, int modifiers);
    void simulateScrollBarChanged(int x, int y);
    void simulateSetScale(double scale);
    void simulateSetZoom(double zoom);
    void simulateWindowMoved(const QPoint& pos);
    void simulateZoomedWindowResized(const QSize& size);
    void toggleZoomMode();
    void zoomIn();
    void zoomIn(const QPoint& focus, const QPoint& viewportFocus);
    void zoomOut();
    void zoomOut(const QPoint& focus, const QPoint& viewportFocus);
    void zoomReset();
    void zoomTo(const QPoint& focus, const QSize& rectSize);

public slots:
    void rotateSkin(SkinRotation rot);

private slots:
    void slot_adbWarningMessageAccepted();
    void slot_blit(QImage* src,
                   QRect srcRect,
                   QImage* dst,
                   QPoint dstPos,
                   QPainter::CompositionMode op,
                   QSemaphore* semaphore = NULL);
    void slot_clearInstance();
    void slot_createBitmap(SkinSurface* s,
                           int w,
                           int h,
                           QSemaphore* semaphore = NULL);
    void slot_fill(SkinSurface* s,
                   QRect rect,
                   QColor color,
                   QSemaphore* semaphore = NULL);
    void slot_getBitmapInfo(SkinSurface* s,
                            SkinSurfacePixels* pix,
                            QSemaphore* semaphore = NULL);
    void slot_getDevicePixelRatio(double* out_dpr,
                                  QSemaphore* semaphore = NULL);
    void slot_getScreenDimensions(QRect* out_rect,
                                  QSemaphore* semaphore = NULL);
    void slot_getFramePos(int* x, int* y, QSemaphore* semaphore = NULL);
    void slot_getWindowPos(int* x, int* y, QSemaphore* semaphore = NULL);
    void slot_isWindowFullyVisible(bool* out_value,
                                   QSemaphore* semaphore = NULL);
    void slot_releaseBitmap(SkinSurface* s, QSemaphore* sempahore = NULL);
    void slot_requestClose(QSemaphore* semaphore = NULL);
    void slot_requestUpdate(QRect rect, QSemaphore* semaphore = NULL);
    void slot_setDeviceGeometry(QRect rect,
                                QSemaphore* semaphore = NULL);
    void slot_setWindowIcon(const unsigned char* data,
                            int size,
                            QSemaphore* semaphore = NULL);
    void slot_setWindowPos(int x, int y, QSemaphore* semaphore = NULL);
    void slot_setWindowTitle(QString title,
                             QSemaphore* semaphore = NULL);
    void slot_showWindow(SkinSurface* surface,
                         QRect rect,
                         QSemaphore* semaphore = NULL);
    void slot_runOnUiThread(SkinGenericFunction* f,
                            void* data,
                            QSemaphore* semaphore = NULL);
    void slot_updateRotation(SkinRotation rotation);

    void slot_horizontalScrollChanged(int value);
    void slot_verticalScrollChanged(int value);

    void slot_scrollRangeChanged(int min, int max);

    void slot_startupTick();

    void slot_avdArchWarningMessageAccepted();
    void slot_gpuWarningMessageAccepted();

    void slot_installCanceled();
    void slot_adbPushCanceled();

    /*
     Here are conventional slots that perform interesting high-level functions
     in
     the emulator. These can be hooked up to signals
     from UI elements or called independently.
     */
public slots:
    void raise();
    void setForwardShortcutsToDevice(int index);
    void show();
    void showMinimized();
    void wheelScrollTimeout();

    void slot_screenChanged();

private:
    static const android::base::StringView kRemoteDownloadsDir;
    static const android::base::StringView kRemoteDownloadsDirApi10;

    // When the main window appears, close the "Starting..."
    // pop-up, if it was displayed.
    void showEvent(QShowEvent* event) {
        mStartupTimer.stop();
        mStartupDialog.close();
    }

    // These are all called on first window show and
    // depend on AVD info, GPU info, etc.,
    // so they must be collected before the window shows up.
    void checkAdbVersionAndWarn();
    void showAvdArchWarning();
    void checkShouldShowGpuWarning();
    void showGpuWarning();

    bool mouseInside();
    SkinMouseButtonType getSkinMouseButton(QMouseEvent* event) const;

    SkinEvent* createSkinEvent(SkinEventType type);
    void forwardKeyEventToEmulator(SkinEventType type, QKeyEvent* event);
    void handleKeyEvent(SkinEventType type, QKeyEvent* event);
    void maskWindowFrame();

    void screenshotDone(android::emulation::ScreenCapturer::Result result);

    void runAdbInstall(const QString& path);
    void installDone(android::emulation::ApkInstaller::Result result,
                     android::base::StringView errorString);

    static const int kPushProgressBarMax;
    void runAdbPush(const QList<QUrl>& urls);
    void adbPushProgress(double progress, bool done);
    void adbPushDone(android::base::StringView filePath,
                     android::emulation::FilePusher::Result result);

    void runAdbShellPowerDownAndQuit();

    android::base::Looper* mLooper;
    QTimer mStartupTimer;
    QProgressDialog mStartupDialog;

    SkinSurface* mBackingSurface;
    QPixmap mScaledBackingBitmap;
    bool mBackingBitmapChanged = true;

    QQueue<SkinEvent*> mSkinEventQueue;
    android::base::Lock mSkinEventQueueLock;

    ToolWindow* mToolWindow;
    EmulatorContainer mContainer;
    EmulatorOverlay mOverlay;
    QRect mDeviceGeometry;

    // Window flags to use for frameless and framed appearance

    static constexpr Qt::WindowFlags FRAMELESS_WINDOW_FLAGS  = (Qt::Window |
                                                                Qt::FramelessWindowHint);
    static constexpr Qt::WindowFlags FRAMED_WINDOW_FLAGS     = (Qt::Window               |
                                                                Qt::WindowTitleHint      |
                                                                Qt::CustomizeWindowHint    );
    static constexpr Qt::WindowFlags FRAME_WINDOW_FLAGS_MASK = (FRAMELESS_WINDOW_FLAGS |
                                                                FRAMED_WINDOW_FLAGS     );

    QPointF mFocus;
    QPoint mViewportFocus;
    double mZoomFactor;
    bool mInZoomMode;
    bool mNextIsZoom;
    bool mForwardShortcutsToDevice;
    QPoint mPrevMousePosition;

    MainLoopThread* mMainLoopThread;

    QMessageBox mAvdWarningBox;

    // First-show related warning messages state
    bool mGpuBlacklisted = false;
    bool mHasForcedRenderer = false;
    bool mShouldShowGpuWarning = false;
    QMessageBox mGpuWarningBox;
    QMessageBox mAdbWarningBox;
    bool mFirstShowEvent;

    EventCapturer mEventCapturer;
    std::shared_ptr<UIEventRecorder<android::base::CircularBuffer>>
            mEventLogger;

    std::shared_ptr<android::qt::UserActionsCounter> mUserActionsCounter;
    std::unique_ptr<android::emulation::AdbInterface> mAdbInterface;
    android::emulation::AdbCommandPtr mApkInstallCommand;
    android::emulation::ApkInstaller mApkInstaller;
    android::emulation::FilePusher mFilePusher;
    android::emulation::ScreenCapturer mScreenCapturer;
    QProgressDialog mInstallDialog;
    QProgressDialog mPushDialog;

    QTimer mWheelScrollTimer;
    QPoint mWheelScrollPos;
    bool mStartedAdbStopProcess;

    bool         mFrameAlways;       // true = no floating emulator
    bool         mHaveBeenFrameless;
    SkinRotation mOrientation;       // Rotation of the main window

    android::metrics::PeriodicReporter::TaskToken mMetricsReportingToken;
};

struct SkinSurface {
    int refcount;
    int id;
    QImage* bitmap;
    int w, h, original_w, original_h;
    EmulatorQtWindow::Ptr window;
};
