LOCAL_PATH := $(call my-dir)

GLES_TR_DIR=gles
COMMON_DIR=common

GLES_TR_CPPS=${GLES_TR_DIR}/api_entries.cpp \
             ${GLES_TR_DIR}/buffer_data.cpp \
             ${GLES_TR_DIR}/debug.cpp \
             ${GLES_TR_DIR}/egl_image.cpp \
             ${GLES_TR_DIR}/framebuffer_data.cpp \
             ${GLES_TR_DIR}/gles1_shader_generator.cpp \
             ${GLES_TR_DIR}/gles_context.cpp \
             ${GLES_TR_DIR}/gles_validate.cpp \
             ${GLES_TR_DIR}/matrix.cpp \
             ${GLES_TR_DIR}/paletted_texture_util.cpp \
             ${GLES_TR_DIR}/pass_through.cpp \
             ${GLES_TR_DIR}/program_data.cpp \
             ${GLES_TR_DIR}/program_variant.cpp \
             ${GLES_TR_DIR}/renderbuffer_data.cpp \
             ${GLES_TR_DIR}/shader_data.cpp \
             ${GLES_TR_DIR}/shader_variant.cpp \
             ${GLES_TR_DIR}/share_group.cpp \
             ${GLES_TR_DIR}/state.cpp \
             ${GLES_TR_DIR}/texture_codecs.cpp \
             ${GLES_TR_DIR}/texture_data.cpp \
             ${GLES_TR_DIR}/uniform_value.cpp \
             ${GLES_TR_DIR}/vector.cpp \
             ${GLES_TR_DIR}/translator_interface.cpp \
             ${COMMON_DIR}/log.cpp \
             ${COMMON_DIR}/etc1.cpp \
             ${COMMON_DIR}/RefBase.cpp

$(call emugl-begin-shared-library,lib$(BUILD_TARGET_SUFFIX)GLES12Translator)

$(call emugl-import,libGLESv1_dec libOpenglCodecCommon)

LOCAL_SRC_FILES := $(GLES_TR_CPPS)
$(call emugl-export,C_INCLUDES,$(EMUGL_PATH)/host/include)
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))

LOCAL_C_INCLUDES += ./ ./include
LOCAL_STATIC_LIBRARIES += libemugl_common

$(call emugl-export,CFLAGS,$(EMUGL_USER_CFLAGS))

$(call emugl-end-module)
