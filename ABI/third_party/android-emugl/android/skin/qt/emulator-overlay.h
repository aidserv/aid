// Copyright (C) 2016 The Android Open Source Project
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

#include <QtCore>
#include <QFrame>
#include <QObject>
#include <QRubberBand>
#include <QTimer>
#include <QVariantAnimation>
#include <QWidget>

class EmulatorQtWindow;
class EmulatorContainer;

class EmulatorOverlay : public QFrame {
    Q_OBJECT

public:
    explicit EmulatorOverlay(EmulatorQtWindow* window,
                             EmulatorContainer* container);
    virtual ~EmulatorOverlay();

    void focusOutEvent(QFocusEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void paintEvent(QPaintEvent* e) override;
    void showEvent(QShowEvent* event) override;

    void hideForFlash();
    void showAsFlash();

    void showForMultitouch(bool centerTouches);

    void showForZoom();
    void showForZoomUserHidden();
    bool wasZoomUserHidden() const;

public slots:
    void hide();

private slots:
    void slot_flashAnimationFinished();
    void slot_flashAnimationValueChanged(const QVariant& value);

    void slot_animationValueChanged(const QVariant& value);

private:
    void hideAndFocusContainer();

    void generateTouchEvents(QMouseEvent* event);
    void updateTouchPoints(QMouseEvent* event);

    QPoint primaryPinchPoint() const;
    QPoint secondaryPinchPoint() const;

    QPoint primarySwipePoint() const;
    QPoint secondaryTouchPoint() const;

    EmulatorQtWindow* mEmulatorWindow;
    EmulatorContainer* mContainer;

    // Zoom-related members
    QRubberBand mRubberBand;
    QPoint mRubberbandOrigin;
    QPixmap mCursor;

    // Multitouch-related members
    QImage mCenterImage;
    QImage mTouchImage;
    QPoint mMultitouchCenter;
    QPoint mPrimaryTouchPoint;
    QPoint mSecondaryTouchPoint;
    QPoint mLastMousePos;
    int mCenterPointRadius;
    int mTouchPointRadius;
    bool mIsSwipeGesture;
    bool mReleaseOnClose;

    // Multitouch animation values
    QVariantAnimation mTouchPointAnimation;
    double mLerpValue;

    // Screenshot-flash related members
    QVariantAnimation mFlashAnimation;
    double mFlashValue;

    // Ensure the overlay is never being used for more than one function
    enum class OverlayMode {
        Hidden,
        Flash,
        Multitouch,
        Zoom,
        UserHiddenZoom,
    } mMode;
};
