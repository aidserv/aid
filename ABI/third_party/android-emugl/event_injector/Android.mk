LOCAL_PATH:=$(call my-dir)
include $(CLEAR_VARS)

#$(call emugl-begin-host-static-library,event_injector)

LOCAL_SRC_FILES := \
    EventInjector.cpp \
    sockets.c \
    emulator-console.c \
    iolooper-select.c

#$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))

#$(call emugl-end-module)


LOCAL_MODULE := event_injector
LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES

include $(BUILD_STATIC_LIBRARY)
