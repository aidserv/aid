# Definitions related to the Qt-based UI backend.

ANDROID_SKIN_SOURCES += \
    android/skin/qt/event-qt.cpp \
    android/skin/qt/init-qt.cpp \
    android/skin/qt/surface-qt.cpp \
    android/skin/qt/winsys-qt.cpp \

ifeq (darwin,$(BUILD_TARGET_OS))
ANDROID_SKIN_SOURCES += \
    android/skin/qt/mac-native-window.mm
endif

ifeq (windows,$(BUILD_TARGET_OS))
ANDROID_SKIN_SOURCES += \
    android/skin/qt/windows-native-window.cpp
endif

ANDROID_SKIN_INCLUDES += $(EMUGL_INCLUDES) $(EMUGL_SRCDIR)/shared $(ANDROID_EMU_INCLUDES)

# gl-widget.cpp needs to call XInitThreads() directly to work around
# a Qt bug. This implies a direct dependency to libX11.so
ifeq (linux,$(BUILD_TARGET_OS))
ANDROID_SKIN_LDLIBS += -lX11
endif

# gl-widget.cpp depends on libOpenGLESDispatch, which depends on
# libemugl_common. Using libOpenGLESDispatch ensures that the code
# will find and use the same host EGL/GLESv2 libraries as the ones
# used by EmuGL. Doing anything else is prone to really bad failure
# cases.
ANDROID_SKIN_STATIC_LIBRARIES += \
    libOpenGLESDispatch \
    libemugl_common \

ANDROID_SKIN_SOURCES += \
    android/skin/qt/accelerometer-3d-widget.cpp \
    android/skin/qt/angle-input-widget.cpp \
    android/skin/qt/editable-slider-widget.cpp \
    android/skin/qt/emulator-container.cpp \
    android/skin/qt/emulator-overlay.cpp \
    android/skin/qt/emulator-qt-window.cpp \
    android/skin/qt/emulator-qt-no-window.cpp \
    android/skin/qt/event-capturer.cpp \
    android/skin/qt/event-serializer.cpp \
    android/skin/qt/event-subscriber.cpp \
    android/skin/qt/error-dialog.cpp \
    android/skin/qt/gl-canvas.cpp \
    android/skin/qt/gl-common.cpp \
    android/skin/qt/gl-texture-draw.cpp \
    android/skin/qt/gl-widget.cpp \
    android/skin/qt/extended-pages/common.cpp \
    android/skin/qt/extended-pages/battery-page.cpp \
    android/skin/qt/extended-pages/car-data-page.cpp \
    android/skin/qt/extended-pages/bug-report-window.cpp \
    android/skin/qt/extended-pages/cellular-page.cpp \
    android/skin/qt/extended-pages/dpad-page.cpp \
    android/skin/qt/extended-pages/finger-page.cpp \
    android/skin/qt/extended-pages/google-play-page.cpp \
    android/skin/qt/extended-pages/help-page.cpp \
    android/skin/qt/extended-pages/location-page.cpp \
    android/skin/qt/extended-pages/microphone-page.cpp \
    android/skin/qt/extended-pages/record-screen-page.cpp \
    android/skin/qt/extended-pages/rotary-input-dial.cpp \
    android/skin/qt/extended-pages/rotary-input-page.cpp \
    android/skin/qt/extended-pages/settings-page.cpp \
    android/skin/qt/extended-pages/settings-page-proxy.cpp \
    android/skin/qt/extended-pages/telephony-page.cpp \
    android/skin/qt/extended-pages/virtual-sensors-page.cpp \
    android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.cpp \
    android/skin/qt/extended-window.cpp \
    android/skin/qt/size-tweaker.cpp \
    android/skin/qt/QtLooper.cpp \
    android/skin/qt/qt-ui-commands.cpp \
    android/skin/qt/stylesheet.cpp \
    android/skin/qt/tool-window.cpp \
    android/skin/qt/ui-event-recorder.cpp \
    android/skin/qt/user-actions-counter.cpp \
    android/skin/qt/wavefront-obj-parser.cpp

ANDROID_SKIN_QT_MOC_SRC_FILES := \
    android/skin/qt/accelerometer-3d-widget.h \
    android/skin/qt/angle-input-widget.h \
    android/skin/qt/editable-slider-widget.h \
    android/skin/qt/gl-widget.h \
    android/skin/qt/emulator-container.h \
    android/skin/qt/emulator-overlay.h \
    android/skin/qt/emulator-qt-window.h \
    android/skin/qt/emulator-qt-no-window.h \
    android/skin/qt/event-capturer.h \
    android/skin/qt/event-subscriber.h \
    android/skin/qt/extended-pages/battery-page.h \
    android/skin/qt/extended-pages/car-data-page.h \
    android/skin/qt/extended-pages/bug-report-window.h \
    android/skin/qt/extended-pages/cellular-page.h \
    android/skin/qt/extended-pages/dpad-page.h \
    android/skin/qt/extended-pages/finger-page.h \
    android/skin/qt/extended-pages/google-play-page.h \
    android/skin/qt/extended-pages/help-page.h \
    android/skin/qt/extended-pages/location-page.h \
    android/skin/qt/extended-pages/microphone-page.h \
    android/skin/qt/extended-pages/record-screen-page.h \
    android/skin/qt/extended-pages/rotary-input-dial.h \
    android/skin/qt/extended-pages/rotary-input-page.h \
    android/skin/qt/extended-pages/settings-page.h \
    android/skin/qt/extended-pages/telephony-page.h \
    android/skin/qt/extended-pages/virtual-sensors-page.h \
    android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.h \
    android/skin/qt/extended-window.h \
    android/skin/qt/raised-material-button.h \
    android/skin/qt/size-tweaker.h \
    android/skin/qt/QtLooperImpl.h \
    android/skin/qt/tool-window.h \
    android/skin/qt/user-actions-counter.h

ANDROID_SKIN_QT_RESOURCES := \
    android/skin/qt/static_resources.qrc \

ANDROID_SKIN_QT_DYNAMIC_RESOURCES := \
    android/skin/qt/resources.qrc \

ANDROID_SKIN_QT_UI_SRC_FILES := \
    android/skin/qt/extended.ui \
    android/skin/qt/extended-pages/car-data-page.ui \
    android/skin/qt/extended-pages/battery-page.ui \
    android/skin/qt/extended-pages/bug-report-window.ui \
    android/skin/qt/extended-pages/cellular-page.ui \
    android/skin/qt/extended-pages/dpad-page.ui \
    android/skin/qt/extended-pages/finger-page.ui \
    android/skin/qt/extended-pages/google-play-page.ui \
    android/skin/qt/extended-pages/help-page.ui \
    android/skin/qt/extended-pages/location-page.ui \
    android/skin/qt/extended-pages/microphone-page.ui \
    android/skin/qt/extended-pages/record-screen-page.ui \
    android/skin/qt/extended-pages/rotary-input-page.ui \
    android/skin/qt/extended-pages/settings-page.ui \
    android/skin/qt/extended-pages/telephony-page.ui \
    android/skin/qt/extended-pages/virtual-sensors-page.ui \
    android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.ui \
    android/skin/qt/tools.ui \

