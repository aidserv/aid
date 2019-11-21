LOCAL_PATH:=$(call my-dir)

#include android/third_party/libsdl2_my.mk
$(call emugl-begin-executable,emulator_v2_renderer)
#$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)OpenglRender)

LOCAL_C_INCLUDES += $(SDL2_INCLUDES) \
	$(EMUGL_PATH)/host/libs \
	$(EMUGL_PATH)/host/include \
	../../../android/android-emu \
	../../../android/sdl-1.2.15/include \

#	/media/a/58727160-d3c1-4e0c-956c-6c48dff1b9df/work/android5.1/prebuilts/tools/windows/sdl/include/SDL \
#	/media/a/58727160-d3c1-4e0c-956c-6c48dff1b9df/work/android7.1/prebuilts/android-emulator-build/qemu-android-deps/windows-x86_64/include/SDL2
	
	
$(info $(LOCAL_PATH))
#$(info $(SDL2_INCLUDES))

LOCAL_SRC_FILES := main.cpp \

#	../../../android/android-emu/android/utils/dll.c
	
ifeq ($(BUILD_TARGET_OS),linux)
LOCAL_LDLIBS += -ldl
endif

ifeq ($(BUILD_TARGET_OS),windows)
LOCAL_LDLIBS += -lstdc++ -lkernel32
LOCAL_LDLIBS += -lws2_32	#WSAStartup
endif

#LOCAL_CFLAGS += $(EMUGL_SDL_CFLAGS) -g -O0
#LOCAL_LDLIBS += $(EMUGL_SDL_LDLIBS)

#LOCAL_STATIC_LIBRARIES += libSDL2 libSDL2main

$(call emugl-end-module)
