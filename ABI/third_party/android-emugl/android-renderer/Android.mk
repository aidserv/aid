LOCAL_PATH:=$(call my-dir)
include $(CLEAR_VARS)

#$(call emugl-begin-host-executable,emulator_test_renderer)
#$(call emugl-import,libOpenglRender event_injector)

#LOCAL_SRC_FILES := main.cpp

#LOCAL_CFLAGS += $(EMUGL_SDL_CFLAGS) -g -O0
#LOCAL_LDLIBS += $(EMUGL_SDL_LDLIBS)

#LOCAL_STATIC_LIBRARIES += libSDL libSDLmain

##$(call emugl-end-module)


LOCAL_MODULE := emulator_android_renderer
LOCAL_C_INCLUDES += $(EMUGL_COMMON_INCLUDES)
LOCAL_CFLAGS += $(EMUGL_COMMON_CFLAGS)
LOCAL_C_INCLUDES += $(EMUGL_PATH)/shared/OpenglCodecCommon
LOCAL_C_INCLUDES += $(EMUGL_PATH)/shared/OpenglOsUtils
#LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/Translator/include
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/include
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/GLESv2_dec
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/GLESv1_dec
LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/renderControl_dec
LOCAL_C_INCLUDES += $(EMUGL_PATH)/tests/emulator_test_renderer \
				    $(EMUGL_PATH) \
        			external/libcxx/include \
        			$(EMUGL_PATH)/event_injector


LOCAL_C_INCLUDES += system/extras/tests/include \
    bionic \
    bionic/libstdc++/include \
    external/stlport/stlport \
	$(call include-path-for, opengl-tests-includes)

LOCAL_SHARED_LIBRARIES += libOpenglRender \
	libcutils \
	liblog \
	libandroidfw \
	libutils \
	libbinder \
    libui \
	libskia \
    libEGL \
    libGLESv1_CM \
    libGLESv2 \
    libmedia \
    libgui \
    libandroid
LOCAL_SHARED_LIBRARIES += libcutils libutils libstlport

LOCAL_LDFLAGS += -ldl
LOCAL_PRELINK_MODULE := false
LDLIBS += $(host_common_LDLIBS) 
				 
LOCAL_LDLIBS += -lstdc++ -std=c++11 -lc++ -ldl # c++支持


LOCAL_CPPFLAGS += -std=c++11 \
	-nostdinc++ \
	-fexceptions 
					 
#_EMUGL_INCLUDE_TYPE := BUILD_HOST_STATIC_LIBRARY
LOCAL_MODULE_BITS := 32
LOCAL_SRC_FILES := main.cpp WindowSurface.cpp
LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES

LOCAL_STATIC_LIBRARIES += event_injector

include $(BUILD_EXECUTABLE)
