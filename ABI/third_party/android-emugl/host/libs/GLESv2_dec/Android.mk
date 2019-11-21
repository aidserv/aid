LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

### host library ##########################################
#$(call emugl-begin-static-library,libGLESv2_dec)
#$(call emugl-import, libOpenglCodecCommon)
#$(call emugl-gen-decoder,$(LOCAL_PATH),gles2)

# For gl2_types.h !
#$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))
#$(call emugl-export,C_INCLUDES,$(LOCAL_PATH)/../../include/OpenGLESDispatch)
#$(call emugl-export,C_INCLUDES,$(LOCAL_PATH)/../libGLSnapshot)

#$(call emugl-export,CFLAGS,$(EMUGL_USER_CFLAGS))

#LOCAL_STATIC_LIBRARIES += libGLSnapshot
#LOCAL_SRC_FILES := GLESv2Decoder.cpp

#$(call emugl-end-module)


### fix
#include external/stlport/libstlport.mk #沒有找到#include <map>添加stlport
LOCAL_MODULE := libGLESv2_dec
LOCAL_C_INCLUDES += $(ANDROID_EMU_BASE_INCLUDES) \
					$(EMUGL_COMMON_INCLUDES) $(EMUGL_PATH)/shared \
					$(LOCAL_PATH) \
					$(LOCAL_PATH)/../../include/OpenGLESDispatch \
					$(LOCAL_PATH)/../libGLSnapshot \
					$(EMUGL_PATH)/host/include \
        			$(EMUGL_PATH)/host/include/OpenglRender \
        			$(EMUGL_PATH)/shared/OpenglCodecCommon \
        			$(EMUGL_PATH) \
        			external/libcxx/include 
				
LOCAL_CPPFLAGS += -std=c++11
	
LOCAL_CFLAGS += $(EMUGL_COMMON_CFLAGS) $(EMUGL_USER_CFLAGS)
LOCAL_LDLIBS += $(host_commonLdLibs) -std=c++11 -lstdc++ -Wall
LOCAL_LDFLAGS += -ldl  # 传递给链接器一些额外的参数
LOCAL_SRC_FILES := GLESv2Decoder.cpp \
	gles2_dec.cpp \
	gles2_server_context.cpp 
	
LOCAL_STATIC_LIBRARIES += libemugl_common libOpenglCodecCommon

include $(BUILD_STATIC_LIBRARY)
