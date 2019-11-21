// Copyright (C) 2015 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/angle-input-widget.h"
#include <QtMath>

// Helper function for AngleInputWidget ctor
static void setUpLineEdit(QLineEdit* editor, const QValidator* validator) {
    editor->setValidator(validator);
    editor->setProperty("class", "EditableValue");
}

// Another helper
static void setUpLabel(QLabel* label, const QString& title) {
    label->setText(title);
    label->setProperty("ColorGroup", "Title");
}

AngleInputWidget::AngleInputWidget(QWidget* parent) :
    QWidget(parent),
    mDecimalModeFrame(this),
    mSexagesimalModeFrame(this),
    mDecimalValueEditor(&mDecimalModeFrame),
    mDegreesValueEditor(&mSexagesimalModeFrame),
    mMinutesValueEditor(&mSexagesimalModeFrame),
    mSecondsValueEditor(&mSexagesimalModeFrame),
    mDegreesLabel(&mSexagesimalModeFrame),
    mMinutesLabel(&mSexagesimalModeFrame),
    mSecondsLabel(&mSexagesimalModeFrame),
    mMinValue(0.0),
    mMaxValue(0.0),
    mDecimalValue(0.0),
    mCurrentInputMode(InputMode::Decimal) {

    // Set up validators for the editors.
    setMinValue(-180.0);
    setMaxValue(180.0);
    mDecimalDegreeValidator.setNotation(QDoubleValidator::StandardNotation);
    mDecimalDegreeValidator.setDecimals(100);
    mDecimalDegreeValidator.setLocale(QLocale::C);
    mMinValidator.setBottom(0);
    mMinValidator.setTop(59);
    mSecValidator.setBottom(0.0);
    mSecValidator.setTop(59.0);
    mSecValidator.setLocale(QLocale::C);

    // Set up the editors.
    setUpLineEdit(&mDecimalValueEditor, &mDecimalDegreeValidator);
    setUpLineEdit(&mDegreesValueEditor, &mIntegerDegreeValidator);
    setUpLineEdit(&mMinutesValueEditor, &mMinValidator);
    setUpLineEdit(&mSecondsValueEditor, &mSecValidator);
    setUpLabel(&mDegreesLabel, QString("\u00B0")); // U+00B0 is the "degree" sign in Unicode.
    setUpLabel(&mMinutesLabel, QString("'"));
    setUpLabel(&mSecondsLabel, QString("''"));

    // Lay out the subwidgets horizontally.
    mDecimalFrameLayout.addWidget(&mDecimalValueEditor);
    mDecimalModeFrame.setLayout(&mDecimalFrameLayout);
    mSexagesimalFrameLayout.addWidget(&mDegreesValueEditor);
    mSexagesimalFrameLayout.addWidget(&mDegreesLabel);
    mSexagesimalFrameLayout.addWidget(&mMinutesValueEditor);
    mSexagesimalFrameLayout.addWidget(&mMinutesLabel);
    mSexagesimalFrameLayout.addWidget(&mSecondsValueEditor);
    mSexagesimalFrameLayout.addWidget(&mSecondsLabel);
    mSexagesimalModeFrame.setLayout(&mSexagesimalFrameLayout);
    mLayout.addWidget(&mDecimalModeFrame);
    mLayout.addWidget(&mSexagesimalModeFrame);
    setLayout(&mLayout);

    // Default input mode is "decimal".
    setInputMode(InputMode::Decimal);

    // Ensure the internal value is updated when changes are made in the editor subwidgets.
    connect(&mDecimalValueEditor, SIGNAL(editingFinished()),
            this, SLOT(updateValueFromDecimalInput()));
    connect(&mDegreesValueEditor, SIGNAL(editingFinished()),
            this, SLOT(updateValueFromSexagesimalInput()));
    connect(&mMinutesValueEditor, SIGNAL(editingFinished()),
            this, SLOT(updateValueFromSexagesimalInput()));
    connect(&mSecondsValueEditor, SIGNAL(editingFinished()),
            this, SLOT(updateValueFromSexagesimalInput()));

    // The line edit controls must not be squished by the spacing
    // introduced by additional box layouts. The spacing is removed
    // by setting margins to 0.
    setContentsMargins(0, 0, 0, 0);
    mDecimalModeFrame.setContentsMargins(0, 0, 0, 0);
    mSexagesimalModeFrame.setContentsMargins(0, 0, 0, 0);
    mSexagesimalFrameLayout.setContentsMargins(0, 0, 0, 0);
    mDecimalFrameLayout.setContentsMargins(0, 0, 0, 0);
    mLayout.setContentsMargins(0, 0, 0, 0);
}

void AngleInputWidget::setValue(double value) {
    mDecimalValue =
        (value <= mMinValue ? mMinValue : (value >= mMaxValue ? mMaxValue : value));
    updateView();
}

void AngleInputWidget::setMinValue(double value) {
   if (value > mMaxValue) {
       return;
   }
   mMinValue = value;
   mDecimalDegreeValidator.setBottom(mMinValue);
   mIntegerDegreeValidator.setBottom(qFloor(mMinValue));
   if (mDecimalValue < mMinValue) {
       mDecimalValue = mMinValue;
   }
}

void AngleInputWidget::setMaxValue(double value) {
   if (value < mMinValue) {
       return;
   }
   mMaxValue = value;
   mDecimalDegreeValidator.setTop(mMaxValue);
   mIntegerDegreeValidator.setTop(qFloor(mMaxValue));
   if (mDecimalValue > mMaxValue) {
       mDecimalValue = mMaxValue;
   }
}

void AngleInputWidget::forceUpdate() {
    switch (mCurrentInputMode) {
    case InputMode::Decimal:
        updateValueFromDecimalInput();
        break;
    case InputMode::Sexagesimal:
        updateValueFromSexagesimalInput();
        break;
    }
}


const int MINUTES_IN_DEGREE = 60;
const int SECONDS_IN_MINUTE = 60;
const int SECONDS_IN_DEGREE = MINUTES_IN_DEGREE * SECONDS_IN_MINUTE;

static int sgn(double x) {
    return (x > 0.0) - (x < 0.0);
}

void AngleInputWidget::updateView() {
    switch(mCurrentInputMode) {
    case InputMode::Decimal:
        // Update the visible sub-widgets.
        mDecimalModeFrame.show();
        mSexagesimalModeFrame.hide();

        // Ensure the correct value is displayed in the editor.
        mDecimalValueEditor.setText(QString::number(mDecimalValue));
        break;

    case InputMode::Sexagesimal:
        // Update the visible sub-widgets.
        mDecimalModeFrame.hide();
        mSexagesimalModeFrame.show();

        // Convert the decimal value to sexagesimal representation.
        int sign = sgn(mDecimalValue);
        double seconds = qFabs(mDecimalValue) * SECONDS_IN_DEGREE;
        int whole_seconds = qFloor(seconds);
        double remainder_seconds = seconds - whole_seconds;

        int degrees = sign * whole_seconds / SECONDS_IN_DEGREE;
        whole_seconds = whole_seconds % SECONDS_IN_DEGREE;
        int minutes = whole_seconds / SECONDS_IN_MINUTE;
        seconds = whole_seconds % SECONDS_IN_MINUTE + remainder_seconds;

        // Ensure the correct value is displayed in the editor.
        // There is a slight tweak to handle the case of negative angles
        // whose absolute value is > 0 and < 1. We want to display the
        // always display the "-" sign in the first box, even if degrees
        // is 0.
        mDegreesValueEditor.setText(
                QString(degrees == 0 && sign == -1 ? "-" : "") +
                QString::number(degrees));
        mMinutesValueEditor.setText(QString::number(minutes));
        mSecondsValueEditor.setText(QString::number(seconds));
        break;
    }
}

void AngleInputWidget::setInputMode(AngleInputWidget::InputMode mode) {
    mCurrentInputMode = mode;
    updateView();
}

void AngleInputWidget::updateValueFromDecimalInput() {
    validateAndUpdateValue(mDecimalValueEditor.text().toDouble());
}

static bool hasValidValue(const QLineEdit& le) {
    QString str = le.text();
    int pos = 0;
    return le.validator()->validate(str, pos) == QValidator::Acceptable;
}

void AngleInputWidget::updateValueFromSexagesimalInput() {
    // Ensure that ALL input boxes contain a valid value.
    if (!hasValidValue(mDegreesValueEditor) ||
        !hasValidValue(mMinutesValueEditor) ||
        !hasValidValue(mSecondsValueEditor)) {
        return;
    }

    int degrees = mDegreesValueEditor.text().toInt();
    int minutes = mMinutesValueEditor.text().toInt();
    double seconds = mSecondsValueEditor.text().toDouble();

    // Again, there's a tweak to handle the case when the abs
    // value of a negative angle is between 0 and 1 degrees,
    // i.e. 0 deg 30 min 25 sec. "-" sign is always in the "degrees"
    // box, even if "degrees" is 0.
    int sign =
        degrees == 0 && mDegreesValueEditor.text()[0] == '-'
            ? -1
            : sgn(degrees);

    validateAndUpdateValue(
        (sign == 0 ? 1 : sign) * (abs(degrees) +
                minutes / static_cast<double>(MINUTES_IN_DEGREE) +
                seconds / static_cast<double>(SECONDS_IN_DEGREE)));
}

void AngleInputWidget::validateAndUpdateValue(double new_value) {
    if (new_value >= mMinValue && new_value <= mMaxValue) {
        mDecimalValue = new_value;
        emit(valueChanged(mDecimalValue));
    } else {
        // Force display the old value if the provided value was out of range.
        updateView();
    }
}
