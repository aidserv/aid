LOCAL_PATH := $(call my-dir)

### GLESv1 implementation ###########################################
$(call emugl-begin-shared-library,libGLESv1_CM_emulation)
$(call emugl-import,libOpenglSystemCommon libGLESv1_enc lib_renderControl_enc)

LOCAL_CFLAGS += -DLOG_TAG=\"GLES_emulation\" -DGL_GLEXT_PROTOTYPES

LOCAL_SRC_FILES := gl.cpp
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -lt 21 && echo PreLollipop),PreLollipop)
    LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/egl
else
    LOCAL_MODULE_RELATIVE_PATH := egl
endif

$(call emugl-end-module)
