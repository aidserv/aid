//Copyright (C) 2016 The Android Open Source Project
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

#include "android/settings-agent.h"
#include <QString>
#include <QHash>

namespace Ui {

// Returns a string containing the style sheet for the
// given UI theme.
const QString& stylesheetForTheme(SettingsTheme theme);

// Retuns a string containing the style sheet for fonts.
// |hi_density| should be true for Retina displays.
const QString& fontStylesheet(bool hi_density);

// Returns a string representing font size value in a style sheet.
const QString& stylesheetFontSize(bool large);

// Returns a mapping of stylesheet template variable names to their
// values in a given theme.
const QHash<QString, QString>& stylesheetValues(SettingsTheme theme);

extern const char THEME_PATH_VAR[];
extern const char MAJOR_TAB_COLOR_VAR[];
extern const char TAB_BKG_COLOR_VAR[];
extern const char TAB_SELECTED_COLOR_VAR[];
}
