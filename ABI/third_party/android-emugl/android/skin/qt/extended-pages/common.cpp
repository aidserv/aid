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

#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/raised-material-button.h"
#include "android/skin/qt/stylesheet.h"
#include "android/skin/qt/qt-settings.h"
#include <QApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QTemporaryFile>
#include <QVariant>

void setButtonEnabled(QPushButton*  button, SettingsTheme theme, bool isEnabled)
{
    button->setEnabled(isEnabled);
    // If this button has icon image properties, reset its
    // image.
    QString enabledPropStr  = button->property("themeIconName"         ).toString();
    QString disabledPropStr = button->property("themeIconName_disabled").toString();

    // Get the resource name based on the light/dark theme
    QString resName = ":/";
    resName += Ui::stylesheetValues(theme)[Ui::THEME_PATH_VAR];
    resName += "/";

    if (!isEnabled  &&  !disabledPropStr.isNull()) {
        // The button is disabled and has a specific
        // icon for when it is disabled.
        // Apply this icon and tell Qt to use it
        // directly (don't gray it out) even though
        // it is disabled.
        resName += disabledPropStr;
        QIcon icon(resName);
        icon.addPixmap(QPixmap(resName), QIcon::Disabled);
        button->setIcon(icon);
    } else if ( !enabledPropStr.isNull() ) {
        // Use the 'enabled' version of the icon
        // (Let Qt grey the icon if it wants to)
        resName += enabledPropStr;
        QIcon icon(resName);
        button->setIcon(icon);
    }
}

bool directoryIsWritable(const QString& dirName) {
    QString dName = QDir::toNativeSeparators(dirName);

    // Check if this path is writable. (Note that QFileInfo::isWritable()
    // does not work well on Windows.)
    bool dirIsOK = false;
    QDir theDir(dName);
    if (theDir.exists()) {
        // See if we can create a file here
        QTemporaryFile tmpFile(dirName + "/XXXXXX");
        // If we can open a temporary file, the directory is OK
        dirIsOK = tmpFile.open();
        // The temporary file is removed when we leave this scope
    }
    return dirIsOK;
}

QString getScreenshotSaveDirectory()
{
    QSettings settings;
    QString savePath = settings.value(Ui::Settings::SAVE_PATH, "").toString();

    if ( !directoryIsWritable(savePath) ) {
        // This path is not writable.
        // Clear it, so we'll try the default instead.
        savePath = "";
    }

    if (savePath.isEmpty()) {

        // We have no path. Try to determine the path to the desktop.
        QStringList paths =
                QStandardPaths::standardLocations(
                    QStandardPaths::DesktopLocation);
        if (paths.size() > 0) {
            savePath = QDir::toNativeSeparators(paths[0]);

            // Save this for future reference
            settings.setValue(Ui::Settings::SAVE_PATH, savePath);
        }
    }

    return savePath;
}

SettingsTheme getSelectedTheme() {
    QSettings settings;
    return (SettingsTheme)settings.value(Ui::Settings::UI_THEME, SETTINGS_THEME_LIGHT).toInt();
}

void adjustAllButtonsForTheme(SettingsTheme theme)
{
    // Switch to the icon images that are appropriate for this theme.
    // Examine every widget.
    QWidgetList wList = QApplication::allWidgets();
    for (QWidget* w : wList) {
        if (QPushButton *pB = qobject_cast<QPushButton*>(w)) {
            if (!pB->icon().isNull()) {
                setButtonEnabled(pB, theme, pB->isEnabled());
            }

            // Adjust shadow color for material buttons depending on theme.
            if (RaisedMaterialButton* material_btn =
                    qobject_cast<RaisedMaterialButton*>(pB)) {
                material_btn
                    ->shadowEffect()
                    ->setColor(theme == SETTINGS_THEME_LIGHT ?
                                   QColor(200, 200, 200) :
                                   QColor(25, 25, 25));
            }
        }
    }
}


QIcon getIconForCurrentTheme(const QString& icon_name) {
    QString iconType = Ui::stylesheetValues(getSelectedTheme())[Ui::THEME_PATH_VAR];
    return QIcon(":/" + iconType + "/" + icon_name);
}

void setFrameOnTop(QFrame* frame, bool onTop) {
#ifndef __linux__
    // On Linux, the Qt::WindowStaysOnTopHint only works if X11 window
    // managment is bypassed (Qt::X11BypassWindowManagerHint). Unfortunately,
    // this prevents a lot of common operations (like moving or resizing the
    // window), so the "always on top" feature is disabled for Linux.
    Qt::WindowFlags flags = frame->windowFlags();
    const bool isVisible = frame->isVisible();

    if (onTop) {
        flags |= Qt::WindowStaysOnTopHint;
    } else {
        flags &= ~Qt::WindowStaysOnTopHint;
    }
    frame->setWindowFlags(flags);

    if (isVisible) {
        frame->show();
    }
#endif
}
