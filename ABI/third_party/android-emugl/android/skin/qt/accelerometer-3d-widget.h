// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/skin/qt/gl-widget.h"
#include "GLES2/gl2.h"

#include <QMatrix4x4>
#include <QMouseEvent>
#include <QVector3D>

#include <array>
#include <string>

// A widget that displays a 3D model of a device and lets the user
// manipulate its rotation and position.
// The changes in rotation and position can be used to
// derive the values which should be reported by the virtual accelerometer,
// gyroscope, and magnetometer.
class Accelerometer3DWidget : public GLWidget {
    Q_OBJECT
    Q_PROPERTY(QQuaternion rotation READ rotation WRITE setRotation NOTIFY rotationChanged USER true);
    Q_PROPERTY(QVector2D position READ position WRITE setPosition NOTIFY positionChanged USER true);
public:
    enum class OperationMode {
        Rotate,
        Move
    };

     Accelerometer3DWidget(QWidget *parent = 0);
    ~Accelerometer3DWidget();

signals:
    // Emitted when the model's rotation changes.
    void rotationChanged();

    // Emitted when the position of the model changes.
    void positionChanged();

    // Emitted when the user begins dragging the model.
    void dragStarted();

    // Emitted when the usr stops dragging the model.
    void dragStopped();

public slots:
    // Sets the rotation quaternion.
    void setRotation(const QQuaternion& quat) {
        mDQuat = QQuaternion();

        mDQuat = quat * mQuat.conjugate();
        recalcRotationUpdateInterval();
        mQuat = quat;
    }

    // Sets the X and Y coordinates of the model's origin;
    void setPosition(const QVector2D& pos) { mTranslation = pos; }

    // Sets the widget's operation mode which determines how it reacts
    // to mose dragging. It may either rotate the model or move it around.
    void setOperationMode(OperationMode mode) { mOperationMode = mode; }

public:
    // Getters for the rotation quaternion and delta.
    const QQuaternion& rotation() const { return mQuat; }
    const QQuaternion& rotationDelta() const { return mDQuat; }
    void resetRotationDelta() { mDQuat = QQuaternion(); }

    // Returns milliseconds since last rotation update.
    float rotationUpdateIntervalSecs() const {
        if (mUpdateIntervalMs < 16) return 0.016; // 16 ms
        return mUpdateIntervalMs / 1000.0f;
    }

    // Returns the X and Y coordinates of the model's origin.
    const QVector2D& position() const { return mTranslation; }

    static constexpr float MaxX = 7.0;
    static constexpr float MinX = -7.0;
    static constexpr float MaxY = 4.0;
    static constexpr float MinY = -4.0;

protected:
    // This is called once, after the GL context is created, to do some one-off
    // setup work.
    bool initGL() override;

    // Called every time the widget changes its dimensions.
    void resizeGL(int, int) override;

    // Called every time the widget needs to be repainted.
    void repaintGL() override;

    // This is called every time the mouse is moved.
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent* event);

private:
    bool initProgram();
    bool initModel();
    bool initTextures();

    // Returns the world coordinates of a point on the XY plane
    // that corresponds to the given point on the screen.
    QVector2D screenToXYPlane(int x, int y) const;

    QQuaternion mQuat;

    // Tracking and timing rates of rotation change
    // (for gyroscope)
    QQuaternion mDQuat;
    uint64_t mUpdateIntervalMs = 0;
    uint64_t mLastRotationUpdateMs = 0;
    static const uint64_t ROTATION_UPDATE_RESET_TIME_MS = 100;
    static const size_t ROTATION_UPDATE_TIME_WINDOW_SIZE = 8;
    std::array<uint64_t, ROTATION_UPDATE_TIME_WINDOW_SIZE>
        mLastUpdateIntervals = {};
    size_t mRotationUpdateTimeWindowElt = 0;
    void recalcRotationUpdateInterval();

    QVector2D mTranslation;
    QMatrix4x4 mPerspective;
    QMatrix4x4 mCameraTransform;

    GLuint mProgram;
    GLuint mVertexDataBuffer;
    GLuint mVertexIndexBuffer;
    GLuint mDiffuseMap;
    GLuint mSpecularMap;
    GLuint mGlossMap;
    GLuint mEnvMap;
    int mElementsCount;

    GLuint mVertexPositionAttribute;
    GLuint mVertexNormalAttribute;
    GLuint mVertexUVAttribute;

    int mPrevMouseX;
    int mPrevMouseY;
    QVector2D mPrevDragOrigin;
    bool mTracking;
    bool mDragging;
    double mXYPlaneDepth;
    OperationMode mOperationMode;
};
