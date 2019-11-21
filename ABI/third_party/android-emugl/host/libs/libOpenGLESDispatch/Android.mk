LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

### host library ##########################################
#$(call emugl-begin-static-library,libOpenGLESDispatch)
#$(call emugl-import,libGLESv2_dec libGLESv1_dec)

# use Translator's egl headers
#LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/Translator/include
#LOCAL_C_INCLUDES += $(EMUGL_PATH)/shared

#LOCAL_SRC_FILES := EGLDispatch.cpp \
#                   GLESv2Dispatch.cpp \
#                   GLESv1Dispatch.cpp \
#                   OpenGLDispatchLoader.cpp \

#$(call emugl-end-module)

### fix
#include external/stlport/libstlport.mk #沒有找到#include <map>添加stlport
LOCAL_MODULE := libOpenGLESDispatch
LOCAL_C_INCLUDES += $(ANDROID_EMU_BASE_INCLUDES) \
					$(EMUGL_COMMON_INCLUDES) \
					$(EMUGL_PATH)/shared \
					$(EMUGL_PATH)/host/libs/Translator/include \
					$(EMUGL_PATH)/host/include \
        			$(EMUGL_PATH)/host/include/OpenglRender \
        			$(EMUGL_PATH)/shared/OpenglCodecCommon \
        			$(EMUGL_PATH) \
	       			external/libcxx/include 
					
LOCAL_CPPFLAGS += -std=c++11
	
LOCAL_CFLAGS += $(EMUGL_COMMON_CFLAGS) $(EMUGL_USER_CFLAGS)
LOCAL_LDFLAGS += -ldl  # 传递给链接器一些额外的参数
LOCAL_LDLIBS += $(host_commonLdLibs) -std=c++11 -lstdc++ -Wall
LOCAL_SRC_FILES := EGLDispatch.cpp \
                   GLESv2Dispatch.cpp \
                   GLESv1Dispatch.cpp \
                   OpenGLDispatchLoader.cpp \
        		   osDynLibrary.cpp \
	
LOCAL_STATIC_LIBRARIES += libemugl_common libOpenglCodecCommon libGLESv2_dec libGLESv1_dec 

include $(BUILD_STATIC_LIBRARY)
