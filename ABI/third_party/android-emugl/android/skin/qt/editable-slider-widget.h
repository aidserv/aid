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

#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

// This widget is souped-up slider that supports fractional values, and has an editable
// text box next to it that displays the current value.
class EditableSliderWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double value READ getValue WRITE setValue NOTIFY valueChanged USER true);
    Q_PROPERTY(double minimum READ getMinimum WRITE setMinimum);
    Q_PROPERTY(double maximum READ getMaximum WRITE setMaximum);

public:
    explicit EditableSliderWidget(QWidget *parent = 0);

    double getValue() const { return mValue; }
    double getMinimum() const { return mMinimum; }
    double getMaximum() const { return mMaximum; }

    // Sets the current value of the widget.
    // The provided value is clipped to the bounds currently imposed by the
    // widget, i.e. if the provided value is lower than mimimum allowed value, the
    // actual value will be set to the minimum allowed value, and if the provided
    // value is greater than the maximum allowed value, the actual value will be
    // set to max.
    // Calling this method will emit the valueChanged signal as long as the
    // emit_signal parameter is true.
    void setValue(double value, bool emit_signal = true);

    // Changes the lower bound of the allowed value range. The current value is
    // clipped to fit into the new range.
    void setMinimum(double minimum);

    // Changes the upper bound of the allowed value range. The current value is
    // clipped to fit into the new range.
    void setMaximum(double maximum);

    // Equivalent to calling setMinimum and setMaximum one after another, with
    // the corresponding argument. If the range is invalid (i.e. minimum >= maximum),
    // this method will have no effect.
    void setRange(double minimum, double maximum) {
        if (minimum < maximum) {
            setMinimum(minimum);
            setMaximum(maximum);
        }
    }

    // Returns true if the slider is pressed down.
    bool isSliderDown() const {
        return mSlider.isSliderDown();
    }

protected:
    bool eventFilter(QObject*, QEvent*) override;

signals:
    // This signal is emitted when the value stored by the widget changes.
    void valueChanged(double value);
    void valueChanged();
    void sliderPressed();
    void sliderReleased();

private slots:
    // Handles the changes in the child slider widget.
    void sliderValueChanged(int new_value);

    // Handles changes in the child line edit widget.
    void lineEditValueChanged();

private:
    double mValue;
    double mMinimum;
    double mMaximum;
    QHBoxLayout mMainLayout;
    QVBoxLayout mAnnotatedSliderLayout;
    QVBoxLayout mEditBoxLayout;
    QHBoxLayout mSliderLabelsLayout;
    QLabel mMinValueLabel;
    QLabel mMaxValueLabel;
    QSlider mSlider;
    QLineEdit mLineEdit;
    QDoubleValidator mLineEditValidator;
};

