LOCAL_PATH := $(call my-dir)

$(call emugl-begin-static-library,libGLSnapshot)
$(call emugl-import,libGLESv2_dec libOpenglCodecCommon)
$(call emugl-export,C_INCLUDES,$(EMUGL_PATH)/host/include)
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/libOpenGLESDispatch
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/include/OpenGLESDispatch
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/GLESv2_dec

LOCAL_STATIC_LIBRARIES += libOpenGLESDispatch
LOCAL_STATIC_LIBRARIES += libemugl_common
LOCAL_STATIC_LIBRARIES += android-emu-base

LOCAL_SRC_FILES := GLSnapshot.cpp

$(call emugl-end-module)
