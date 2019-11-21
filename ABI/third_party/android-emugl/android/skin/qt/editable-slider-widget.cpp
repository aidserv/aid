#include "android/skin/qt/editable-slider-widget.h"

#include <QEvent>
#include <QStyle>

EditableSliderWidget::EditableSliderWidget(QWidget *parent) :
    QWidget(parent),
    mMinValueLabel(this),
    mMaxValueLabel(this),
    mSlider(Qt::Horizontal, this),
    mLineEdit(this),
    mLineEditValidator(&mLineEdit) {

    // Arrange the child widgets in a layout.
    mSliderLabelsLayout.setSpacing(0);
    mSliderLabelsLayout.addWidget(&mMinValueLabel);
    mSliderLabelsLayout.addWidget(&mMaxValueLabel);
    mAnnotatedSliderLayout.setSpacing(0);
    mAnnotatedSliderLayout.addWidget(&mSlider);
    mAnnotatedSliderLayout.addSpacing(3);
    mAnnotatedSliderLayout.addLayout(&mSliderLabelsLayout);
    mEditBoxLayout.setSpacing(0);
    mEditBoxLayout.addWidget(&mLineEdit);
    mEditBoxLayout.addStretch();
    mMainLayout.setSpacing(0);
    mMainLayout.addLayout(&mAnnotatedSliderLayout);
    mMainLayout.addSpacing(16);
    mMainLayout.addLayout(&mEditBoxLayout);
    setLayout(&mMainLayout);

    // Set initial values.
    setMinimum(static_cast<int>(mSlider.minimum()/10.0));
    setMaximum(static_cast<int>(mSlider.maximum()/10.0));
    setValue(static_cast<int>(mSlider.value()/10.0));

    // Do some adjustments to child widgets.
    mMinValueLabel.setAlignment(Qt::AlignLeft | Qt::AlignTop);
    mMaxValueLabel.setAlignment(Qt::AlignRight | Qt::AlignTop);
    mMinValueLabel.setProperty("ColorGroup", "SliderLabel");
    mMaxValueLabel.setProperty("ColorGroup", "SliderLabel");
    mLineEditValidator.setDecimals(1);
    mLineEditValidator.setNotation(QDoubleValidator::StandardNotation);
    mLineEditValidator.setLocale(QLocale::c());
    mLineEdit.setValidator(&mLineEditValidator);
    mLineEdit.setAlignment(Qt::AlignRight | Qt::AlignTop);
    mLineEdit.setMaximumWidth(60);
    mLineEdit.setTextMargins(0, 0, 0, 4);
    mLineEdit.setProperty("class", "EditableValue");
    mLineEdit.installEventFilter(this);

    // Handle child widgets' value changes in our own slots.
    connect(&mSlider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
    connect(&mSlider, SIGNAL(sliderPressed()), this, SIGNAL(sliderPressed()));
    connect(&mSlider, SIGNAL(sliderReleased()), this, SIGNAL(sliderReleased()));
    connect(&mLineEdit, SIGNAL(editingFinished()), this, SLOT(lineEditValueChanged()));
}

void EditableSliderWidget::setValue(double value, bool emit_signal) {
    // Clip the value to the allowed range.
    mValue =
        value < mMinimum ? mMinimum : (value > mMaximum ? mMaximum : value);

    // Update the values displayed by the slider and the edit box.
    {
        QSignalBlocker blocker(mSlider);
        mSlider.setValue(static_cast<int>(mValue * 10.0));
    }
    mLineEdit.setText(QString("%1").arg(mValue, 0, 'f', 1, '0'));
    if (emit_signal) {
        emit valueChanged(mValue);
        emit valueChanged();
    }
    mLineEdit.setProperty("ColorGroup", QString());
    mLineEdit.style()->unpolish(&mLineEdit);
    mLineEdit.style()->polish(&mLineEdit);
}

void EditableSliderWidget::setMinimum(double minimum) {
    mMinimum = minimum;
    mLineEditValidator.setBottom(mMinimum);
    setValue(mValue); // Force the current value into the new bounds.
    mMinValueLabel.setText(QString::number(mMinimum));
    mSlider.setMinimum(static_cast<int>(mMinimum * 10.0));
}

void EditableSliderWidget::setMaximum(double maximum) {
    mMaximum = maximum;
    mLineEditValidator.setTop(mMaximum);
    setValue(mValue); // Force the current value into the new bounds.
    mMaxValueLabel.setText(QString::number(mMaximum));
    mSlider.setMaximum(static_cast<int>(mMaximum * 10.0));
}

void EditableSliderWidget::sliderValueChanged(int new_value) {
    setValue(static_cast<double>(new_value) / 10.0);
}

void EditableSliderWidget::lineEditValueChanged() {
    QString new_value = mLineEdit.text();
    setValue(new_value.toDouble());
}

bool EditableSliderWidget::eventFilter(QObject*, QEvent* e) {
    if (e->type() == QEvent::FocusIn) {
        // If the edit box was previously highlighted due to an
        // invalid value, un-highlight it.
        mLineEdit.setProperty("ColorGroup", QString());
        mLineEdit.style()->unpolish(&mLineEdit);
        mLineEdit.style()->polish(&mLineEdit);
    } else if (e->type() == QEvent::FocusOut) {
        // When the edit box loses focus, highlight it
        // if it contains an invalid value.
        int dummy;
        QString text = mLineEdit.text();
        if (mLineEdit.validator()->validate(text, dummy) != QValidator::Acceptable) {
            mLineEdit.setProperty("ColorGroup", QString("InvalidInput"));
            mLineEdit.style()->unpolish(&mLineEdit);
            mLineEdit.style()->polish(&mLineEdit);
        }
    }
    return false;
}
