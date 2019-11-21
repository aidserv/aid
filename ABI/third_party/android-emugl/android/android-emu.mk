
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

### android-emu-base library ###########################################

commonSources := \
	base/threads/ThreadStore.cpp \
    base/synchronization/MessageChannel.cpp \
    base/memory/LazyInstance.cpp \
    utils/debug.c \
    utils/system.c
	
#	base/threads/FunctorThread.cpp \
#	base/threads/ThreadStore.cpp \
#	base/threads/Async.cpp \
	




host_commonLdLibs := $(CXX_STD_LIB)


### fix

#include external/stlport/libstlport.mk #沒有找到#include <map>添加stlport

LOCAL_MODULE := android-emu-base
LOCAL_C_INCLUDES += \
					$(EMUGL_PATH)/android/base/threads \
					$(EMUGL_PATH)/host/include \
					$(EMUGL_PATH) \
        			external/libcxx/include 
					
LOCAL_STL=c++_static
LOCAL_CPPFLAGS += -std=c++11 \
	-nostdinc++ \
	-fexceptions 
LOCAL_CFLAGS += $(EMUGL_COMMON_CFLAGS) -std=c++11 -lc++ -ldl -D_STLP_USE_NEWALLOC
LOCAL_LDLIBS += $(host_commonLdLibs) -lstdc++ -std=c++11 -lc++
LOCAL_LDFLAGS += -ldl  -DCERES_BUILDING_SHARED_LIBRARY -DCERES_RESTRICT_SCHUR_SPECIALIZATION  # 传递给链接器一些额外的参数
LOCAL_SRC_FILES := $(commonSources)


## LOCAL_CFLAGS += -fstack-protector-all
LOCAL_CFLAGS += -fomit-frame-pointer

include $(BUILD_STATIC_LIBRARY)
