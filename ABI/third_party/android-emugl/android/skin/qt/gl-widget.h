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

#include "android/skin/qt/gl-canvas.h"
#include "android/skin/qt/gl-texture-draw.h"

#include <cmath>
#include <memory>

#include <QWidget>

struct EGLState;
struct EGLDispatch;

// Helper class used to perform EGL/GLESv2 rendering inside a Qt widget.
// Relies on EmuGL's OpenGLESDispatch library to access the correct set of
// host graphics libraries, and provide dispatch tables to call their functions.
//
// Client should create a derived class and override initGL(), repaintGL()
// and resizeGL() properly. These methods can use the |mEGL| and |mGLES2|
// fields to perform EGL/GLESv2 calls.
class GLWidget : public QWidget {
    Q_OBJECT
public:
    explicit GLWidget(QWidget* parent = 0);

    virtual ~GLWidget();

    QPaintEngine* paintEngine() const override { return nullptr; }
    bool readyForRendering() const { return mValid; }

public slots:
    void renderFrame();
    bool makeContextCurrent();

private slots:
    void handleScreenChange(QScreen*);

protected:
    // Dispatch tables for EGL and GLESv2 APIs. Note that these will be nullptr
    // if there was a problem when loading the host libraries.
    const EGLDispatch* mEGL;
    const GLESv2Dispatch* mGLES2;

    // Called the first time a frame needs to be rendered by the widget.
    // This will always happen before the first repaintGL() or resizeGL()
    // call. The implementation can assume that |mEGL| and |mGLES2| are valid,
    // and that the widget's context is already set.
    // This function should return true if the initialization is successful and
    // false otherwise.
    virtual bool initGL() { return true; }

    // Called whenever a frame needs to be repainted by the widget.
    // The implementation can assume that |mEGL| and |mGLES2| are valid,
    // and that the widget's context is already set.
    virtual void repaintGL() {}

    // Called whenever the widget needs to be resized and updated.
    // Note that this will always followed by a call to repaintGL(). The
    // implementation can assume that |mEGL| and |mGLES2| are valid and that
    // the widget's context is already set. |w| and |h| are the widget's
    // new dimensions in pixels.
    virtual void resizeGL(int w, int h) {}

    int realPixelsWidth() const { return std::ceil(width() * devicePixelRatio()); }
    int realPixelsHeight() const { return std::ceil(height() * devicePixelRatio()); }
    void toggleAA() { mEnableAA = !mEnableAA; }

private:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void showEvent(QShowEvent*) override;

    bool ensureInit();
    void destroyContext();

    EGLState* mEGLState;
    bool mValid;
    bool mEnableAA;
    bool mFirstShow = true;

    std::unique_ptr<GLCanvas> mCanvas;
    std::unique_ptr<TextureDraw> mTextureDraw;
};
