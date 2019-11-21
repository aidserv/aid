# Definitions related to the emulator's UI implementation.

ANDROID_SKIN_LDLIBS :=
ANDROID_SKIN_CFLAGS :=
ANDROID_SKIN_INCLUDES :=
ANDROID_SKIN_STATIC_LIBRARIES :=

ANDROID_SKIN_QT_MOC_SRC_FILES :=
ANDROID_SKIN_QT_RESOURCES :=

ANDROID_SKIN_SOURCES := \
    android/skin/charmap.c \
    android/skin/rect.c \
    android/skin/image.c \
    android/skin/trackball.c \
    android/skin/keyboard.c \
    android/skin/keycode.c \
    android/skin/keycode-buffer.c \
    android/skin/file.c \
    android/skin/window.c \
    android/skin/resource.c \
    android/skin/ui.c \

# enable MMX code for our skin scaler
ANDROID_SKIN_CFLAGS += -DUSE_MMX=1 -mmmx

include $(_ANDROID_EMU_ROOT)/android/skin/qt/sources.mk
