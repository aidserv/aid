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

#include "android/skin/qt/extended-window.h"
#include "android/skin/qt/extended-window-styles.h"

#include "android/featurecontrol/FeatureControl.h"
#include "android/globals.h"
#include "android/main-common.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/extended-pages/help-page.h"
#include "android/skin/qt/qt-settings.h"
#include "android/skin/qt/stylesheet.h"
#include "android/skin/qt/tool-window.h"
#include "android/ui-emu-agent.h"

#include "ui_extended.h"

#include <QDesktopWidget>

ExtendedWindow::ExtendedWindow(
    EmulatorQtWindow *eW,
    ToolWindow *tW,
    const ShortcutKeyStore<QtUICommand>* shortcuts) :
    QFrame(nullptr),
    mEmulatorWindow(eW),
    mToolWindow(tW),
    mExtendedUi(new Ui::ExtendedControls),
    mSizeTweaker(this),
    mSidebarButtons(this)
{
    // "Tool" type windows live in another layer on top of everything in OSX, which is undesirable
    // because it means the extended window must be on top of the emulator window. However, on
    // Windows and Linux, "Tool" type windows are the only way to make a window that does not have
    // its own taskbar item.
#ifdef __APPLE__
    Qt::WindowFlags flag = Qt::Dialog;
#else
    Qt::WindowFlags flag = Qt::Tool;
#endif

    setWindowFlags(flag | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);

    QSettings settings;
    bool onTop = settings.value(Ui::Settings::ALWAYS_ON_TOP, false).toBool();
    setFrameOnTop(this, onTop);

    mExtendedUi->setupUi(this);
    mExtendedUi->helpPage->initialize(shortcuts, mEmulatorWindow);
    mExtendedUi->dpadPage->setEmulatorWindow(mEmulatorWindow);
    mExtendedUi->rotaryInputPage->setEmulatorWindow(mEmulatorWindow);
    mExtendedUi->settingsPage->setAdbInterface(
            mEmulatorWindow->getAdbInterface());
    mExtendedUi->virtualSensorsPage->setLayoutChangeNotifier(eW);

     connect(
        mExtendedUi->settingsPage, SIGNAL(frameAlwaysChanged(bool)),
        this, SLOT(switchFrameAlways(bool)));

    connect(
        mExtendedUi->settingsPage, SIGNAL(onTopChanged(bool)),
        this, SLOT(switchOnTop(bool)));

    connect(
        mExtendedUi->settingsPage, SIGNAL(onForwardShortcutsToDeviceChanged(int)),
        mEmulatorWindow, SLOT(setForwardShortcutsToDevice(int)));

    connect(
        mExtendedUi->settingsPage, SIGNAL(themeChanged(SettingsTheme)),
        this, SLOT(switchToTheme(SettingsTheme)));

    connect(
        mExtendedUi->settingsPage, SIGNAL(enableClipboardSharingChanged(bool)),
        mToolWindow, SLOT(switchClipboardSharing(bool)));
    connect(
        mToolWindow, SIGNAL(haveClipboardSharingKnown(bool)),
        mExtendedUi->settingsPage, SLOT(setHaveClipboardSharing(bool)));

    mPaneButtonMap = {
        {PANE_IDX_CAR,           mExtendedUi->carDataButton},
        {PANE_IDX_LOCATION,      mExtendedUi->locationButton},
        {PANE_IDX_CELLULAR,      mExtendedUi->cellularButton},
        {PANE_IDX_BATTERY,       mExtendedUi->batteryButton},
        {PANE_IDX_TELEPHONE,     mExtendedUi->telephoneButton},
        {PANE_IDX_DPAD,          mExtendedUi->dpadButton},
        {PANE_IDX_ROTARY,        mExtendedUi->rotaryInputButton},
        {PANE_IDX_MICROPHONE,    mExtendedUi->microphoneButton},
        {PANE_IDX_FINGER,        mExtendedUi->fingerButton},
        {PANE_IDX_VIRT_SENSORS,  mExtendedUi->virtSensorsButton},
        {PANE_IDX_SETTINGS,      mExtendedUi->settingsButton},
        {PANE_IDX_HELP,          mExtendedUi->helpButton},
        {PANE_IDX_RECORD_SCREEN, mExtendedUi->recordScreenButton},
        {PANE_IDX_GOOGLE_PLAY,   mExtendedUi->googlePlayButton},
    };

    setObjectName("ExtendedControls");

    mSidebarButtons.addButton(mExtendedUi->locationButton);
    mSidebarButtons.addButton(mExtendedUi->cellularButton);
    mSidebarButtons.addButton(mExtendedUi->batteryButton);
    mSidebarButtons.addButton(mExtendedUi->telephoneButton);
    mSidebarButtons.addButton(mExtendedUi->dpadButton);
    if (android_hw->hw_rotaryInput) {
        mSidebarButtons.addButton(mExtendedUi->rotaryInputButton);
    } else {
        mExtendedUi->rotaryInputButton->hide();
    }
    mSidebarButtons.addButton(mExtendedUi->microphoneButton);
    mSidebarButtons.addButton(mExtendedUi->fingerButton);
    mSidebarButtons.addButton(mExtendedUi->virtSensorsButton);
    mSidebarButtons.addButton(mExtendedUi->recordScreenButton);
    mSidebarButtons.addButton(mExtendedUi->settingsButton);
    mSidebarButtons.addButton(mExtendedUi->helpButton);

    if (android::featurecontrol::isEnabled(android::featurecontrol::PlayStoreImage) &&
        android_hw->PlayStore_enabled)
    {
        mSidebarButtons.addButton(mExtendedUi->googlePlayButton);
        mExtendedUi->googlePlayPage->initialize(
                mEmulatorWindow->getAdbInterface());
        mExtendedUi->googlePlayButton->setVisible(true);
    }

    if (avdInfo_getAvdFlavor(android_avdInfo) == AVD_ANDROID_AUTO) {
        mSidebarButtons.addButton(mExtendedUi->carDataButton);
        mExtendedUi->carDataButton->setVisible(true);
    }

#ifdef __APPLE__
    for (QWidget* w : findChildren<QWidget*>()) {
        w->setAttribute(Qt::WA_MacShowFocusRect, false);
    }
#endif

    connect(mExtendedUi->virtualSensorsPage,
            SIGNAL(coarseOrientationChanged(SkinRotation)),
            eW,
            SLOT(rotateSkin(SkinRotation)));
}

ExtendedWindow::~ExtendedWindow() {
    mExtendedUi->location_page->requestStopLoadingGeoData();
}

void ExtendedWindow::setAgent(const UiEmuAgent* agentPtr) {
    if (agentPtr) {
        mExtendedUi->cellular_page->setCellularAgent(agentPtr->cellular);
        mExtendedUi->batteryPage->setBatteryAgent(agentPtr->battery);
        mExtendedUi->telephonyPage->setTelephonyAgent(agentPtr->telephony);
        mExtendedUi->finger_page->setFingerAgent(agentPtr->finger);
        mExtendedUi->location_page->setLocationAgent(agentPtr->location);
        mExtendedUi->microphonePage->setMicrophoneAgent(agentPtr->userEvents);
        mExtendedUi->settingsPage->setHttpProxyAgent(agentPtr->proxy);
        mExtendedUi->virtualSensorsPage->setSensorsAgent(agentPtr->sensors);
        if (avdInfo_getAvdFlavor(android_avdInfo) == AVD_ANDROID_AUTO) {
            mExtendedUi->carDataPage->setCarDataAgent(agentPtr->car);
        }
//        mExtendedUi->recordScreenPage->setRecordScreenAgent(agentPtr->recordScreen);
    }
    // The ADB port is known now. Show it on the UI Help page.
    mExtendedUi->helpPage->setAdbPort();
}

void ExtendedWindow::show() {
    QFrame::show();

    // Verify that the extended pane is fully visible (otherwise it may be
    // impossible for the user to move it)
    QDesktopWidget *desktop = static_cast<QApplication*>(
                                     QApplication::instance() )->desktop();
    int screenNum = desktop->screenNumber(this); // Screen holding most of this

    QRect screenGeo = desktop->screenGeometry(screenNum);
    QRect myGeo = geometry();

    bool moved = false;
    // Leave some padding between the window and the edge of the screen.
    // This distance isn't precise--it's mainly to prevent the window from
    // looking like it's a little off screen.
    static const int gap = 10;
    if (myGeo.x() + myGeo.width() > screenGeo.x() + screenGeo.width() - gap) {
        // Right edge is off the screen
        myGeo.setX(screenGeo.x() + screenGeo.width() - myGeo.width() - gap);
        moved = true;
    }
    if (myGeo.y() + myGeo.height() > screenGeo.y() + screenGeo.height() - gap) {
        // Bottom edge is off the screen
        myGeo.setY(screenGeo.y() + screenGeo.height() - myGeo.height() - gap);
        moved = true;
    }
    if (myGeo.x() < screenGeo.x() + gap) {
        // Top edge is off the screen
        myGeo.setX(screenGeo.x() + gap);
        moved = true;
    }
    if (myGeo.y() < screenGeo.y() + gap) {
        // Left edge is off the screen
        myGeo.setY(screenGeo.y() + gap);
        moved = true;
    }
    if (moved) {
        setGeometry(myGeo);
    }
}

void ExtendedWindow::showPane(ExtendedWindowPane pane) {
    show();
    adjustTabs(pane);
}

void ExtendedWindow::closeEvent(QCloseEvent *e) {
    // Merely hide the window the widget is closed, do not destroy state.
    e->ignore();
    hide();
}

void ExtendedWindow::keyPressEvent(QKeyEvent* e) {
    mToolWindow->handleQtKeyEvent(e);
}

// Tab buttons. Each raises its stacked pane to the top.
void ExtendedWindow::on_batteryButton_clicked()      { adjustTabs(PANE_IDX_BATTERY); }
void ExtendedWindow::on_cellularButton_clicked()     { adjustTabs(PANE_IDX_CELLULAR); }
void ExtendedWindow::on_dpadButton_clicked()         { adjustTabs(PANE_IDX_DPAD); }
void ExtendedWindow::on_rotaryInputButton_clicked()  { adjustTabs(PANE_IDX_ROTARY); }
void ExtendedWindow::on_fingerButton_clicked()       { adjustTabs(PANE_IDX_FINGER); }
void ExtendedWindow::on_helpButton_clicked()         { adjustTabs(PANE_IDX_HELP); }
void ExtendedWindow::on_locationButton_clicked()     { adjustTabs(PANE_IDX_LOCATION); }
void ExtendedWindow::on_microphoneButton_clicked()   { adjustTabs(PANE_IDX_MICROPHONE); }
void ExtendedWindow::on_settingsButton_clicked()     { adjustTabs(PANE_IDX_SETTINGS); }
void ExtendedWindow::on_telephoneButton_clicked()    { adjustTabs(PANE_IDX_TELEPHONE); }
void ExtendedWindow::on_virtSensorsButton_clicked()  { adjustTabs(PANE_IDX_VIRT_SENSORS); }
void ExtendedWindow::on_recordScreenButton_clicked() { adjustTabs(PANE_IDX_RECORD_SCREEN); }
void ExtendedWindow::on_googlePlayButton_clicked() { adjustTabs(PANE_IDX_GOOGLE_PLAY); }
void ExtendedWindow::on_carDataButton_clicked()        { adjustTabs(PANE_IDX_CAR); }

void ExtendedWindow::adjustTabs(ExtendedWindowPane thisIndex) {
    auto it = mPaneButtonMap.find(thisIndex);
    if (it == mPaneButtonMap.end()) {
        return;
    }
    QPushButton* thisButton = it->second;
    thisButton->toggle();
    thisButton->clearFocus(); // It looks better when not highlighted
    mExtendedUi->stackedWidget->setCurrentIndex(static_cast<int>(thisIndex));
}

void ExtendedWindow::switchFrameAlways(bool showFrame)
{
    mEmulatorWindow->setFrameAlways(showFrame);
}

void ExtendedWindow::switchOnTop(bool isOnTop) {
    mEmulatorWindow->setOnTop(isOnTop);
    setFrameOnTop(this, isOnTop);
    mToolWindow->notifySwitchOnTop();
}

void ExtendedWindow::switchToTheme(SettingsTheme theme) {
    // Switch to the icon images that are appropriate for this theme.
    adjustAllButtonsForTheme(theme);

    // Set the Qt style.

    // The first part is based on the display's pixel density.
    // Most displays give 1.0; high density displays give 2.0.
    const double densityFactor = devicePixelRatio();
    QString styleString = Ui::fontStylesheet(densityFactor > 1.5);

    // The second part is based on the theme
    // Set the style for this theme
    styleString += Ui::stylesheetForTheme(theme);

    // Apply this style to the extended window (this),
    // and to the main tool-bar.
    this->setStyleSheet(styleString);
    mToolWindow->setStyleSheet(styleString);
    mExtendedUi->rotaryInputPage->updateTheme();
    BugReportWindow* bugreport = mExtendedUi->helpPage->getBugreportWindow();
    if (bugreport) {
        bugreport->updateTheme();
    }

    // Force a re-draw to make the new style take effect
    this->style()->unpolish(mExtendedUi->stackedWidget);
    this->style()->polish(mExtendedUi->stackedWidget);
    this->update();

    // Make the Settings pane active (still)
    adjustTabs(PANE_IDX_SETTINGS);
}

void ExtendedWindow::showEvent(QShowEvent* e) {
    if (mFirstShowEvent && !e->spontaneous()) {
        // This function has things that must be performed
        // after the ctor and after show() is called
        switchToTheme(getSelectedTheme());

        // Set the first tab active
        on_locationButton_clicked();

        mFirstShowEvent = false;

        // There is a gap between the main window and the tool bar. Use the same
        // gap between the tool bar and the extended window.
        move(mToolWindow->geometry().right() + ToolWindow::toolGap,
             mToolWindow->geometry().top());
    }
    QFrame::showEvent(e);
}
