LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

host_OS_SRCS :=
host_common_LDLIBS :=

ifeq ($(BUILD_TARGET_OS),linux)
    host_OS_SRCS = NativeSubWindow_x11.cpp
    host_common_LDLIBS += -lX11 -lrt
endif

ifeq ($(BUILD_TARGET_OS),darwin)
    host_OS_SRCS = NativeSubWindow_cocoa.m
    host_common_LDLIBS += -Wl,-framework,AppKit
endif

ifeq ($(BUILD_TARGET_OS),windows)
    host_OS_SRCS = NativeSubWindow_win32.cpp
    host_common_LDLIBS += -lgdi32
endif

host_common_SRC_FILES := \
    $(host_OS_SRCS) \
    ColorBuffer.cpp \
    FbConfig.cpp \
    FenceSync.cpp \
    FrameBuffer.cpp \
    GLESVersionDetector.cpp \
    ReadBuffer.cpp \
    RenderContext.cpp \
    RenderControl.cpp \
    RendererImpl.cpp \
    RenderLibImpl.cpp \
    RenderThread.cpp \
    RenderThreadInfo.cpp \
    render_api.cpp \
    RenderWindow.cpp \
    SyncThread.cpp \
    TextureDraw.cpp \
    TextureResize.cpp \
    WindowSurface.cpp \
    YUVConverter.cpp \
    RenderServer.cpp \
	NativeSubWindow_x11.cpp \
	
#    ChannelStream.cpp 
#    RenderChannelImpl.cpp 

### host libOpenglRender #################################################
#$(call emugl-begin-shared-library,lib$(BUILD_TARGET_SUFFIX)OpenglRender)

#$(call emugl-import,libGLESv1_dec libGLESv2_dec lib_renderControl_dec libOpenglCodecCommon)

#LOCAL_LDLIBS += $(host_common_LDLIBS)
#LOCAL_LDLIBS += $(ANDROID_EMU_LDLIBS)

#LOCAL_SRC_FILES := $(host_common_SRC_FILES)
#$(call emugl-export,C_INCLUDES,$(EMUGL_PATH)/host/include)
#$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))

# use Translator's egl/gles headers
#LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/Translator/include
#LOCAL_C_INCLUDES += $(EMUGL_PATH)/host/libs/libOpenGLESDispatch
#LOCAL_C_INCLUDES += $(EMUGL_PATH)/shared/emugl/common

#LOCAL_STATIC_LIBRARIES += libemugl_common
#LOCAL_STATIC_LIBRARIES += libOpenGLESDispatch
#LOCAL_STATIC_LIBRARIES += libGLSnapshot

#LOCAL_SYMBOL_FILE := render_api.entries

#$(call emugl-export,CFLAGS,$(EMUGL_USER_CFLAGS))

#$(call emugl-end-module)

### fix
#include external/stlport/libstlport.mk #沒有找到#include <map>添加stlport
LOCAL_MODULE := libOpenglRender
LOCAL_C_INCLUDES += $(ANDROID_EMU_BASE_INCLUDES) \
					$(EMUGL_COMMON_INCLUDES) \
					$(EMUGL_PATH)/shared \
					$(EMUGL_PATH)/host/libs/Translator/include \
					$(EMUGL_PATH)/host/libs/libOpenGLESDispatch \
					$(EMUGL_PATH)/shared/emugl/common \
					$(EMUGL_PATH)/host/include \
        			$(EMUGL_PATH)/host/include/OpenglRender \
					$(EMUGL_PATH)/host/include/OpenGLESDispatch \
        			$(EMUGL_PATH)/shared/OpenglCodecCommon \
        			$(EMUGL_PATH) \
        			$(EMUGL_PATH)/host/libs/GLESv1_dec \
        			$(EMUGL_PATH)/host/libs/GLESv2_dec \
        			$(EMUGL_PATH)/host/libs/renderControl_dec \
        			$(EMUGL_PATH)/host/libs/libGLSnapshot \
        			$(EMUGL_PATH)/host/libs/EGL_dec \
        			external/libcxx/include 

# c++支持        			
LOCAL_STL := c++_static stlport_static
LOCAL_CPPFLAGS += -std=c++11 \
	-nostdinc++ \
	-fexceptions
 
LOCAL_CFLAGS += -DLOG_TAG=\"libOpenglRender\"
LOCAL_CFLAGS += $(EMUGL_COMMON_CFLAGS) $(EMUGL_USER_CFLAGS)
LOCAL_LDFLAGS += -ldl -DCERES_BUILDING_SHARED_LIBRARY -DCERES_RESTRICT_SCHUR_SPECIALIZATION -D_STLP_USE_NEWALLOC \
				-D_STLP_USE_NO_IOSTREAMS \
				-D_STLP_USE_MALLOC \
				# 传递给链接器一些额外的参数
LOCAL_LDLIBS += $(host_commonLdLibs) \
				$(host_common_LDLIBS) \
				$(ANDROID_EMU_LDLIBS) \
				 -lstdc++ -std=c++11 -lc++ -ldl # c++支持

LOCAL_CFLAGS +=  -D_STLP_USE_MALLOC -D_STLP_USE_SIMPLE_NODE_ALLOC
LOCAL_CPPFLAGS += -D_STLP_USE_MALLOC -D_STLP_USE_SIMPLE_NODE_ALLOC

LOCAL_SRC_FILES := $(host_common_SRC_FILES)
APP_STL := c++_static stlport_static                
LOCAL_SYMBOL_FILE := render_api.entries

LOCAL_STATIC_LIBRARIES += libemugl_common
LOCAL_STATIC_LIBRARIES += libOpenGLESDispatch \
						  libGLESv1_dec \
						  libGLESv2_dec \
						  lib_renderControl_dec \
						  libOpenglCodecCommon \
						  android-emu-base \
						  libstlport_static

LOCAL_SHARED_LIBRARIES += libcutils libutils liblog libstlport
						  
#						  libEGL_dec 
						  
#LOCAL_STATIC_LIBRARIES += libGLSnapshot

include $(BUILD_SHARED_LIBRARY)



####
## external/stlport/stlport/stl/_alloc.h:158: error: undefined reference to 'std::__node_alloc::_M_allocate(unsigned int&)
## LOCAL_STATIC_LIBRARIES += libstlport_static

