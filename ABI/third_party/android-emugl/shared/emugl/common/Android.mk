# This build script corresponds to a library containing many definitions
# common to both the guest and the host. They relate to
#
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

### emugl_common host library ###########################################

commonSources := \
        crash_reporter.cpp \
        logging.cpp \
        misc.cpp \
        osDynLibrary.cpp \
        stringparsing.cpp \
        sockets.cpp \
        sync_device.cpp \
        osThreadUnix.cpp \
        feature_control.cpp 


##        dma_device.cpp 
##        
##        shared_library.cpp 


host_commonSources := $(commonSources)

host_commonLdLibs := $(CXX_STD_LIB)

#ifneq (windows,$(BUILD_TARGET_OS))
#    host_commonLdLibs += -ldl -lpthread
#endif

#$(call emugl-begin-static-library,libemugl_common)
#LOCAL_SRC_FILES := $(host_commonSources)
#$(call emugl-export,C_INCLUDES,$(EMUGL_PATH)/shared)
#$(call emugl-export,LDLIBS,$(host_commonLdLibs))
#$(call emugl-end-module)

### fix

#include external/stlport/libstlport.mk
include external/stlport/libstlport.mk #沒有找到#include <map>添加stlport

LOCAL_MODULE := libemugl_common
LOCAL_C_INCLUDES += $(ANDROID_EMU_BASE_INCLUDES) \
					$(EMUGL_COMMON_INCLUDES) \
					$(EMUGL_PATH)/shared \
					$(EMUGL_PATH)/host/include \
					$(EMUGL_PATH) \

#        			external/libcxx/include 
					
#LOCAL_CPPFLAGS += -std=c++11
					
LOCAL_CFLAGS += $(EMUGL_COMMON_CFLAGS) -ldl -D_STLP_USE_NEWALLOC
LOCAL_LDLIBS += $(host_commonLdLibs) -lstdc++ \
				 -lstdc++ -std=c++11 -lc++ -ldl # c++支持
LOCAL_LDFLAGS += -ldl  # 传递给链接器一些额外的参数
LOCAL_SRC_FILES := $(host_commonSources)


## LOCAL_CFLAGS += -fstack-protector-all
LOCAL_CFLAGS += -fomit-frame-pointer

include $(BUILD_STATIC_LIBRARY)
