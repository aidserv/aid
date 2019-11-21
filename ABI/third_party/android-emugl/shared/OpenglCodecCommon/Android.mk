# This build script corresponds to a library containing many definitions
# common to both the guest and the host. They relate to
#
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

##sheng add TcpStream.cpp TcpStream.h SocketStream.cpp SockteStream.h
commonSources := \
        glUtils.cpp \
		TcpStream.cpp \
		SocketStream.cpp \
        ChecksumCalculator.cpp \
        ChecksumCalculatorThreadInfo.cpp \
        sockets.cpp
        
        
host_commonSources := $(commonSources)

host_commonLdLibs := $(CXX_STD_LIB)

### OpenglCodecCommon  host ##############################################
#$(call emugl-begin-static-library,libOpenglCodecCommon)

#LOCAL_SRC_FILES := $(host_commonSources)
#$(call emugl-import, libemugl_common)
#$(call emugl-export,C_INCLUDES,$(EMUGL_PATH)/host/include/OpenglRender $(LOCAL_PATH))
#$(call emugl-export,LDLIBS,$(host_commonLdLibs))
#LOCAL_C_INCLUDE += TcpStream.h SockteStream.h
#$(call emugl-end-module)


### fix
#include external/stlport/libstlport.mk #沒有找到#include <map>添加stlport
LOCAL_MODULE := libOpenglCodecCommon
LOCAL_C_INCLUDES += $(ANDROID_EMU_BASE_INCLUDES) \
					$(EMUGL_COMMON_INCLUDES) \
					$(EMUGL_PATH)/shared \
					$(EMUGL_PATH)/host/include \
					$(EMUGL_PATH) \
        			$(EMUGL_PATH)/host/include/OpenglRender \
        			external/libcxx/include 
LOCAL_CPPFLAGS += -std=c++11
					
LOCAL_CFLAGS += $(EMUGL_COMMON_CFLAGS) -ldl -D_STLP_USE_NEWALLOC
LOCAL_LDLIBS += $(host_commonLdLibs) -lstdc++ -Wall \
				 -lstdc++ -std=c++11 -lc++ -ldl # c++支持
LOCAL_LDFLAGS += -ldl  # 传递给链接器一些额外的参数
LOCAL_SRC_FILES := $(host_commonSources)

LOCAL_STATIC_LIBRARIES += libemugl_common

include $(BUILD_STATIC_LIBRARY)
