LOCAL_PATH := $(call my-dir)

host_common_SRC_FILES := \
     GLESv2Imp.cpp       \
     GLESv2Context.cpp   \
     GLESv2Validate.cpp  \
     SamplerData.cpp    \
     ShaderParser.cpp    \
     ShaderValidator.cpp \
     ProgramData.cpp

# ANGLE shader translation is not supported on Windows yet.
host_common_SRC_FILES += ANGLEShaderParser.cpp

### GLES_V2 host implementation (On top of OpenGL) ########################
$(call emugl-begin-shared-library,lib$(BUILD_TARGET_SUFFIX)GLES_V2_translator)
$(call emugl-import, libGLcommon)
$(call emugl-import, libOpenglCodecCommon)

LOCAL_SRC_FILES := $(host_common_SRC_FILES)

# ANGLE shader translation is not supported on Windows yet.
LOCAL_C_INCLUDES += $(ANGLE_TRANSLATION_INCLUDES)
LOCAL_STATIC_LIBRARIES += $(ANGLE_TRANSLATION_STATIC_LIBRARIES)

$(call emugl-end-module)
