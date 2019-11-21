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
#include "android/skin/qt/extended-pages/virtual-sensors-page.h"

#include "android/emulation/control/sensors_agent.h"
#include "android/emulator-window.h"
#include "android/hw-sensors.h"
#include "android/metrics/PeriodicReporter.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/skin/ui.h"

#include "android/skin/qt/stylesheet.h"

#include <QDesktopServices>
#include <QQuaternion>

#include <array>
#include <cassert>
#define M_PI 3.14159265358979323846
VirtualSensorsPage::VirtualSensorsPage(QWidget* parent) :
    QWidget(parent),
    mUi(new Ui::VirtualSensorsPage()),
    mSensorsAgent(nullptr)
{
    mUi->setupUi(this);
    // The initial values are set here to match the initial
    // values reported by an AVD.
    mUi->temperatureSensorValueWidget->setRange(-273.1, 100.0);
    mUi->temperatureSensorValueWidget->setValue(0.0);
    mUi->lightSensorValueWidget->setRange(0, 40000.0);
    mUi->lightSensorValueWidget->setValue(0.0);
    mUi->pressureSensorValueWidget->setRange(0, 1100);
    mUi->pressureSensorValueWidget->setValue(0.0);
    mUi->humiditySensorValueWidget->setRange(0, 100);
    mUi->humiditySensorValueWidget->setValue(0);
    mUi->proximitySensorValueWidget->setRange(0, 10);
    mUi->proximitySensorValueWidget->setValue(1);
    mUi->magNorthWidget->setLocale(QLocale::c());
    mUi->magEastWidget->setLocale(QLocale::c());
    mUi->magVerticalWidget->setLocale(QLocale::c());

    updateSensorValues();

    connect(mUi->accelWidget, SIGNAL(rotationChanged()),
            this, SLOT(onPhoneRotationChanged()));
    connect(mUi->accelWidget, SIGNAL(positionChanged()),
            this, SLOT(onPhonePositionChanged()));

    connect(mUi->accelWidget, SIGNAL(dragStopped()),
            this, SLOT(onDragStopped()));
    connect(mUi->accelWidget, SIGNAL(dragStarted()),
            this, SLOT(onDragStarted()));

    connect(mUi->positionXSlider, SIGNAL(sliderPressed()),
            this, SLOT(onDragStarted()));
    connect(mUi->positionXSlider, SIGNAL(sliderReleased()),
            this, SLOT(onDragStopped()));

    connect(mUi->positionYSlider, SIGNAL(sliderPressed()),
            this, SLOT(onDragStarted()));
    connect(mUi->positionYSlider, SIGNAL(sliderReleased()),
            this, SLOT(onDragStopped()));

    connect(mUi->yawSlider, SIGNAL(sliderPressed()),
            this, SLOT(onDragStarted()));
    connect(mUi->yawSlider, SIGNAL(sliderReleased()),
            this, SLOT(onDragStopped()));

    connect(mUi->pitchSlider, SIGNAL(sliderPressed()),
            this, SLOT(onDragStarted()));
    connect(mUi->pitchSlider, SIGNAL(sliderReleased()),
            this, SLOT(onDragStopped()));

    connect(mUi->rollSlider, SIGNAL(sliderPressed()),
            this, SLOT(onDragStarted()));
    connect(mUi->rollSlider, SIGNAL(sliderReleased()),
            this, SLOT(onDragStopped()));

    connect(this, &VirtualSensorsPage::updateResultingValuesRequired,
            this, &VirtualSensorsPage::updateResultingValues);

    connect(&mAccelerationTimer, SIGNAL(timeout()),
            this, SLOT(updateAccelerations()));
    mAccelerationTimer.setInterval(100);
    mAccelerationTimer.stop();

    mUi->yawSlider->setRange(-180.0, 180.0);
    mUi->pitchSlider->setRange(-180.0, 180.0);
    mUi->rollSlider->setRange(-180.0, 180.0);
    mUi->positionXSlider->setRange(Accelerometer3DWidget::MinX,
                                   Accelerometer3DWidget::MaxX);
    mUi->positionYSlider->setRange(Accelerometer3DWidget::MinY,
                                   Accelerometer3DWidget::MaxY);
    // Historically, the AVD starts up with the screen mostly
    // vertical, but tilted back 4.75 degrees. Retain that
    // initial orientation.
    // We need to do this after we call setRange since setRange will trigger
    // on_*Slider_valueChanged which just trigger setRotation from default
    // value of Sliders.
    static const QQuaternion initialQuat =
            QQuaternion::fromEulerAngles(-4.75, 0.00, 0.00);
    mUi->accelWidget->setRotation(initialQuat);
    onPhoneRotationChanged();

    using android::metrics::PeriodicReporter;
    mMetricsReportingToken = PeriodicReporter::get().addCancelableTask(
            60 * 10 * 1000,  // reporting period
            [this](android_studio::AndroidStudioEvent* event) {
                if (mVirtualSensorsUsed) {
                    event->mutable_emulator_details()
                            ->mutable_used_features()
                            ->set_sensors(true);
                    mMetricsReportingToken.reset();  // Report it only once.
                    return true;
                }
                return false;
    });
}

void VirtualSensorsPage::showEvent(QShowEvent*) {
    auto layout = skin_ui_get_current_layout(emulator_window_get()->ui);
    if (layout) {
        resetAccelerometerRotationFromSkinLayout(layout->orientation);
    }
    mFirstShow = false;
}

void VirtualSensorsPage::setLayoutChangeNotifier(
        QObject* layout_change_notifier) {
    connect(layout_change_notifier, SIGNAL(layoutChanged(SkinRotation)),
            this, SLOT(onSkinLayoutChange(SkinRotation)));
}

void VirtualSensorsPage::onSkinLayoutChange(SkinRotation rot) {
    resetAccelerometerRotationFromSkinLayout(rot);
}

void VirtualSensorsPage::resetAccelerometerRotationFromSkinLayout(
        SkinRotation orientation) {
    float rot = 0.0;

    // NOTE: the "incorrect" angle values
    // stem from the fact that QQuaternion and SKIN_ROTATION_*
    // disagree on which direction is "positive" (skin uses
    // a different coordinate system with origin at top left
    // and X and Y axis pointing right and down respectively).
    switch (orientation) {
    case SKIN_ROTATION_0:
        rot = 0.0;
        break;
    case SKIN_ROTATION_90:
        rot = -90.0;
        break;
    case SKIN_ROTATION_180:
        rot = 180.0;
        break;
    case SKIN_ROTATION_270:
        rot = 90.0;
        break;
    default:
        assert(0);
    }
    // Rotation reset handler notifies the subscribers, and one of the
    // subscribers is the main window which handles that as a regular
    // rotation command. As we're already handling a rotation command, that
    // would make us to handle it again for the second time, which is
    // very CPU-intensive.
    QSignalBlocker blockSignals(this);
    resetAccelerometerRotation(
            QQuaternion::fromAxisAndAngle(
                0.0, 0.0, 1.0, rot));
}

void VirtualSensorsPage::resetAccelerometerRotation(const QQuaternion& rotation) {
    if (!mFirstShow) mVirtualSensorsUsed = true;
    mUi->accelWidget->setPosition(QVector2D(0.0, 0.0));
    mUi->accelWidget->setRotation(rotation);
    mUi->accelWidget->resetRotationDelta();
    mUi->accelWidget->update();
    onPhoneRotationChanged();
    onPhonePositionChanged();
}

void VirtualSensorsPage::on_rotateToPortrait_clicked() {
    resetAccelerometerRotation(QQuaternion());
}

void VirtualSensorsPage::on_rotateToLandscape_clicked() {
    resetAccelerometerRotation(
        QQuaternion::fromAxisAndAngle(0.0, 0.0, 1.0, 90.0));
}

void VirtualSensorsPage::on_rotateToReversePortrait_clicked() {
    resetAccelerometerRotation(
        QQuaternion::fromAxisAndAngle(0.0, 0.0, 1.0, 180.0));
}

void VirtualSensorsPage::on_rotateToReverseLandscape_clicked() {
    resetAccelerometerRotation(
        QQuaternion::fromAxisAndAngle(0.0, 0.0, 1.0, -90.0));
}

void VirtualSensorsPage::setSensorsAgent(const QAndroidSensorsAgent* agent) {
    mSensorsAgent = agent;

    // Update the agent with current values.
    updateSensorValues();
}

// Helper function
static void setSensorValue(
        const QAndroidSensorsAgent* agent,
        AndroidSensor sensor_id,
        double v1,
        double v2 = 0.0,
        double v3 = 0.0) {
  if (agent) {
        agent->setSensor(sensor_id,
                         static_cast<float>(v1),
                         static_cast<float>(v2),
                         static_cast<float>(v3));
    }
}

void VirtualSensorsPage::on_temperatureSensorValueWidget_valueChanged(
        double value) {
    if (!mFirstShow) mVirtualSensorsUsed = true;
    setSensorValue(mSensorsAgent, ANDROID_SENSOR_TEMPERATURE, value);
}

void VirtualSensorsPage::on_proximitySensorValueWidget_valueChanged(
        double value) {
    if (!mFirstShow) mVirtualSensorsUsed = true;
    setSensorValue(mSensorsAgent, ANDROID_SENSOR_PROXIMITY, value);
}

void VirtualSensorsPage::on_lightSensorValueWidget_valueChanged(double value) {
    if (!mFirstShow) mVirtualSensorsUsed = true;
    setSensorValue(mSensorsAgent, ANDROID_SENSOR_LIGHT, value);
}

void VirtualSensorsPage::on_pressureSensorValueWidget_valueChanged(
    double value) {
    if (!mFirstShow) mVirtualSensorsUsed = true;
    setSensorValue(mSensorsAgent, ANDROID_SENSOR_PRESSURE, value);
}

void VirtualSensorsPage::on_humiditySensorValueWidget_valueChanged(
    double value) {
    if (!mFirstShow) mVirtualSensorsUsed = true;
    setSensorValue(mSensorsAgent, ANDROID_SENSOR_HUMIDITY, value);
}

void VirtualSensorsPage::on_magNorthWidget_valueChanged(double value) {
    updateSensorValues();
}
void VirtualSensorsPage::on_magEastWidget_valueChanged(double value) {
    updateSensorValues();
}
void VirtualSensorsPage::on_magVerticalWidget_valueChanged(double value) {
    updateSensorValues();
}

void VirtualSensorsPage::onPhoneRotationChanged() {
    const QQuaternion& rotation = mUi->accelWidget->rotation();
    // CAVEAT: There is some inconsistency related to the terms "yaw",
    // "pitch" and "roll" between the QQuaternion docs and how
    // these terms are defined in the Android docs.
    // According to android docs:
    // When the device lies flat, screen-up, the Z
    // axis comes out of the screen, the Y axis comes
    // out of the top and the X axis comes out of the right side
    // of the device. The same coordinate system is used by the
    // accelerometer control widget.
    // Android docs define "roll" as the rotation around the Y axis.
    // However, QQuaternion defines "roll" as the rotation around
    // the Z axis. Essentially, "yaw" and "roll" are switched.
    // For consistency, we stick with Android definitions of
    // yaw, pitch and roll.
    float x, y, z;
    rotation.getEulerAngles(&x, &y, &z);
    mUi->yawSlider->setValue(z, false);
    mUi->pitchSlider->setValue(x, false);
    mUi->rollSlider->setValue(y, false);
    updateSensorValues();
}

void VirtualSensorsPage::setAccelerometerRotationFromSliders() {
    // WARNING: read the comment in VirtualSensorsPage::onPhoneRotationChanged
    // before changing the order of these arguments!!
    mUi->accelWidget->setRotation(
        QQuaternion::fromEulerAngles(
            mUi->pitchSlider->getValue(),
            mUi->rollSlider->getValue(),
            mUi->yawSlider->getValue()));
    updateSensorValues();
    mUi->accelWidget->update();
}

void VirtualSensorsPage::on_yawSlider_valueChanged(double) {
    setAccelerometerRotationFromSliders();
}

void VirtualSensorsPage::on_pitchSlider_valueChanged(double) {
    setAccelerometerRotationFromSliders();
}

void VirtualSensorsPage::on_rollSlider_valueChanged(double) {
    setAccelerometerRotationFromSliders();
}

void VirtualSensorsPage::setPhonePositionFromSliders() {
    mCurrentPosition = QVector3D(mUi->positionXSlider->getValue(),
                                 mUi->positionYSlider->getValue(),
                                 0.0);
    mUi->accelWidget->setPosition(mCurrentPosition.toVector2D());
    mUi->accelWidget->update();
}

void VirtualSensorsPage::on_positionXSlider_valueChanged(double) {
    setPhonePositionFromSliders();
}

void VirtualSensorsPage::on_positionYSlider_valueChanged(double) {
    setPhonePositionFromSliders();
}

void VirtualSensorsPage::updateSensorValues() {
    if (!mFirstShow) mVirtualSensorsUsed = true;
    // Gravity and magnetic vector in the device's frame of
    // reference.
    QVector3D gravity_vector(0.0, 9.81, 0.0);
    QVector3D magnetic_vector(
            mUi->magNorthWidget->value(),
            mUi->magEastWidget->value(),
            mUi->magVerticalWidget->value());

    QQuaternion device_rotation_quat = mUi->accelWidget->rotation();
    QQuaternion rotationDelta =
        mUi->accelWidget->rotationDelta();
    float dx, dy, dz;
    rotationDelta.getEulerAngles(&dx, &dy, &dz);

    // Gravity and magnetic vectors as observed by the device.
    // Note how we're applying the *inverse* of the transformation
    // represented by device_rotation_quat to the "absolute" coordinates
    // of the vectors.
    QVector3D device_gravity_vector =
        device_rotation_quat.conjugate().rotatedVector(gravity_vector);
    QVector3D device_magnetic_vector =
        device_rotation_quat.conjugate().rotatedVector(magnetic_vector);
    QVector3D acceleration = device_gravity_vector - mLinearAcceleration;

    float gyroUpdateRate = mUi->accelWidget->rotationUpdateIntervalSecs();

    // Convert raw UI pitch/roll/yaw delta
    // to device-space rotation in radians per second.
    QVector3D gyroscope =
        device_rotation_quat.conjugate().rotatedVector(
            QVector3D(dx, dy, dz)) * M_PI / 180.0 / gyroUpdateRate;

    setSensorValue(mSensorsAgent,
                   ANDROID_SENSOR_ACCELERATION,
                   acceleration.x(),
                   acceleration.y(),
                   acceleration.z());

    setSensorValue(mSensorsAgent, ANDROID_SENSOR_GYROSCOPE,
                   gyroscope.x(), gyroscope.y(), gyroscope.z());

    setSensorValue(mSensorsAgent, ANDROID_SENSOR_GYROSCOPE_UNCALIBRATED,
                   gyroscope.x(), gyroscope.y(), gyroscope.z());

    setSensorValue(mSensorsAgent,
                   ANDROID_SENSOR_MAGNETIC_FIELD,
                   device_magnetic_vector.x(),
                   device_magnetic_vector.y(),
                   device_magnetic_vector.z());

    setSensorValue(mSensorsAgent,
                   ANDROID_SENSOR_MAGNETIC_FIELD_UNCALIBRATED,
                   device_magnetic_vector.x(),
                   device_magnetic_vector.y(),
                   device_magnetic_vector.z());

    // Update the "rotation" label according to the simulated gravity vector.
    QVector3D normalized_gravity = device_gravity_vector.normalized();
    static const std::array<std::pair<QVector3D, SkinRotation>, 4> directions {
      std::make_pair(QVector3D(0, 1, 0), SKIN_ROTATION_0),
      std::make_pair(QVector3D(-1, 0, 0), SKIN_ROTATION_90),
      std::make_pair(QVector3D(0, -1, 0), SKIN_ROTATION_180),
      std::make_pair(QVector3D(1, 0, 0), SKIN_ROTATION_270)
    };

    QString rotation_label;
    SkinRotation coarse_orientation = mCoarseOrientation;
    for (const auto& v : directions) {
      if (fabs(QVector3D::dotProduct(normalized_gravity, v.first) - 1.0) < 0.1) {
        coarse_orientation = v.second;
        break;
      }
    }

    if (coarse_orientation != mCoarseOrientation) {
      mCoarseOrientation = coarse_orientation;
      emit(coarseOrientationChanged(mCoarseOrientation));
    }

    // Emit a signal to update the UI. We cannot just update
    // the UI here because the current function is sometimes
    // called from a non-Qt thread.
    // We only block signals for this widget if it's running on the Qt thread,
    // so it's Ok to call the connected function directly.
    if (signalsBlocked()) {
        updateResultingValues(acceleration, gyroscope, device_magnetic_vector);
    } else {
        emit updateResultingValuesRequired(
                acceleration, gyroscope, device_magnetic_vector);
    }
}

void VirtualSensorsPage::updateResultingValues(QVector3D acceleration,
                                               QVector3D gyroscope,
                                               QVector3D device_magnetic_vector) {

    static const QString rotation_labels[] = {
        "ROTATION_0",
        "ROTATION_90",
        "ROTATION_180",
        "ROTATION_270"
    };

    // Update labels with new values.
    QString table_html;
    QTextStream table_html_stream(&table_html);
    table_html_stream.setRealNumberPrecision(2);
    table_html_stream.setNumberFlags(table_html_stream.numberFlags() |
                                     QTextStream::ForcePoint);
    table_html_stream.setRealNumberNotation(QTextStream::FixedNotation);
    table_html_stream
        << "<table border=\"0\""
        << "       cellpadding=\"3\" style=\"font-size:" <<
           Ui::stylesheetFontSize(false) << "\">"
        << "<tr>"
        << "<td>" << tr("Accelerometer (m/s<sup>2</sup>)") << ":</td>"
        << "<td align=left>" << acceleration.x() << "</td>"
        << "<td align=left>" << acceleration.y() << "</td>"
        << "<td align=left>" << acceleration.z() << "</td></tr>"
        << "<tr>"
        << "<td>" << tr("Gyroscope (rad/s)") << ":</td>"
        << "<td align=left>" << gyroscope.x() << "</td>"
        << "<td align=left>" << gyroscope.y() << "</td>"
        << "<td align=left>" << gyroscope.z() << "</td></tr>"
        << "<tr>"
        << "<td>" << tr("Magnetometer (&mu;T)") << ":</td>"
        << "<td align=left>" << device_magnetic_vector.x() << "</td>"
        << "<td align=left>" << device_magnetic_vector.y() << "</td>"
        << "<td align=left>" << device_magnetic_vector.z() << "</td></tr>"
        << "<tr><td>" << tr("Rotation")
        << ":</td><td colspan = \"3\" align=left>"
        << rotation_labels[mCoarseOrientation - SKIN_ROTATION_0]
        << "</td></tr>"
        << "</table>";
    mUi->resultingAccelerometerValues->setText(table_html);
}

void VirtualSensorsPage::onPhonePositionChanged() {
    const QVector2D& pos = mUi->accelWidget->position();
    mCurrentPosition = QVector3D(pos.x(), pos.y(), 0.0);
    mUi->positionXSlider->setValue(pos.x(), false);
    mUi->positionYSlider->setValue(pos.y(), false);
}

void VirtualSensorsPage::updateAccelerations() {
    static const float k = 100.0;
    static const float mass = 1.0;
    static const float meters_per_unit = 0.0254;

    QVector3D delta =
        mUi->accelWidget->rotation().conjugate().rotatedVector(
            meters_per_unit * (mCurrentPosition - mPrevPosition));
    mLinearAcceleration = delta * k / mass;
    mPrevPosition = mCurrentPosition;

    mUi->accelWidget->setRotation(
        QQuaternion::fromEulerAngles(
            mUi->pitchSlider->getValue(),
            mUi->rollSlider->getValue(),
            mUi->yawSlider->getValue()));
    updateSensorValues();

    mUi->accelWidget->resetRotationDelta();
}

void VirtualSensorsPage::on_accelModeRotate_toggled() {
    if (mUi->accelModeRotate->isChecked()) {
        mUi->accelWidget->setOperationMode(
            Accelerometer3DWidget::OperationMode::Rotate);
        mUi->accelerometerSliders->setCurrentIndex(0);
    }
}

void VirtualSensorsPage::on_accelModeMove_toggled() {
    if (mUi->accelModeMove->isChecked()) {
        mUi->accelWidget->setOperationMode(
            Accelerometer3DWidget::OperationMode::Move);
        mUi->accelerometerSliders->setCurrentIndex(1);
    }
}

void VirtualSensorsPage::on_helpMagneticField_clicked() {
    QDesktopServices::openUrl(QUrl::fromEncoded(
            "https://developer.android.com/reference/android/hardware/Sensor.html#TYPE_MAGNETIC_FIELD"));
}

void VirtualSensorsPage::on_helpLight_clicked() {
    QDesktopServices::openUrl(QUrl::fromEncoded(
            "https://developer.android.com/reference/android/hardware/Sensor.html#TYPE_LIGHT"));
}

void VirtualSensorsPage::on_helpPressure_clicked() {
    QDesktopServices::openUrl(QUrl::fromEncoded(
            "https://developer.android.com/reference/android/hardware/Sensor.html#TYPE_PRESSURE"));
}

void VirtualSensorsPage::on_helpAmbientTemp_clicked() {
  QDesktopServices::openUrl(QUrl::fromEncoded(
            "https://developer.android.com/reference/android/hardware/Sensor.html#TYPE_AMBIENT_TEMPERATURE"));
}

void VirtualSensorsPage::on_helpProximity_clicked() {
  QDesktopServices::openUrl(QUrl::fromEncoded(
            "https://developer.android.com/reference/android/hardware/Sensor.html#TYPE_PROXIMITY"));
}

void VirtualSensorsPage::on_helpHumidity_clicked() {
  QDesktopServices::openUrl(QUrl::fromEncoded(
            "https://developer.android.com/reference/android/hardware/Sensor.html#TYPE_RELATIVE_HUMIDITY"));
}
