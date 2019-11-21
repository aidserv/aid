// Copyright (C) 2015 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QIntValidator>
#include <QWidget>

// This is a widget for specifying angle values.
// It has two input modes - decimal and sexagesimal.
// In decimal mode, it allows the user to specify a single
// decimal value in degrees, i.e. 10.5 degrees
// In sexagesimal mode, it allows the user to specify the value
// in terms of degrees, minutes and seconds, i.e. 10 degrees and
// 30 minutes.
// The widget limits the input values to a certain range. The
// default range is [-180; +180].
class AngleInputWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double value READ value NOTIFY valueChanged USER true);

public:
    enum class InputMode {
        Decimal,
        Sexagesimal
    };

    explicit AngleInputWidget(QWidget* parent = nullptr);

    // For setting the value programmatically.
    // The value will be clipped to the current value range.
    // This will NOT trigger a valueChanged signal.
    void setValue(double value);

    // Returns the decimal measure of the angle, in degrees.
    double value() const { return mDecimalValue; }

    // Indicates which input mode the widget is currently in.
    InputMode currentMode() const { return mCurrentInputMode; }

    // Modify the range of input values allowed by the widget by
    // setting the minimum allowed value.
    // If the provided argument is greater than the current max value,
    // the range is not modified.
    // If at the time of the call, the current value is less than the
    // provided argument, it is set to the provided argument.
    void setMinValue(double value);

    // Modify the range of input values allowed by the widget by
    // setting the maximum allowed value.
    // If the provided argument is less than the current min value,
    // the range is not modified.
    // If at the time of the call, the current value is greater than
    // the provided argument, it is set to the provided argument.
    void setMaxValue(double value);

    // Get the current minimum value allowed by the widget.
    double minValue() const { return mMinValue; }

    // Get the current maximum value allowed by the widget.
    double maxValue() const { return mMaxValue; }

    // This function reads the latest Lat/Lon input from the user.
    // If a number is valid, it is used. If it is invalid, the
    // previous valid number is re-displayed.
    // This way, the user sees the Lat/Lon that is actually sent
    // to the AVD.
    void forceUpdate();

public slots:
    // Sets the widget's current input mode to either decimal or sexagesimal.
    void setInputMode(InputMode);

signals:
    // Emitted when the user changes the value.
    // Regardless of the current input mode, new_value will always be a decimal
    // value specified in degrees, i.e. 10.5 degrees.
    void valueChanged(double new_value);

private slots:
    // Updates the internal value based on the decimal editor.
    void updateValueFromDecimalInput();

    // Updates the internal value based on the sexagesimal editor.
    void updateValueFromSexagesimalInput();

    // Checks that the provided argument fits into the range permitted
    // by the widget. If it does, updates the internal value and emits
    // the appropriate signal.
    void validateAndUpdateValue(double new_value);

    // Updates the currently displayed set of widgets (depending on the
    // input mode) to correctly display the current value.
    void updateView();
private:
    QFrame mDecimalModeFrame;
    QFrame mSexagesimalModeFrame;
    QHBoxLayout mDecimalFrameLayout;
    QHBoxLayout mSexagesimalFrameLayout;
    QLineEdit mDecimalValueEditor;
    QLineEdit mDegreesValueEditor;
    QLineEdit mMinutesValueEditor;
    QLineEdit mSecondsValueEditor;
    QLabel mDegreesLabel;
    QLabel mMinutesLabel;
    QLabel mSecondsLabel;
    QDoubleValidator mDecimalDegreeValidator;
    QIntValidator mIntegerDegreeValidator;
    QIntValidator mMinValidator;
    QDoubleValidator mSecValidator;
    QHBoxLayout mLayout;
    QDoubleValidator* mValueValidator;
    double mMinValue;
    double mMaxValue;
    double mDecimalValue;
    InputMode mCurrentInputMode;
};
