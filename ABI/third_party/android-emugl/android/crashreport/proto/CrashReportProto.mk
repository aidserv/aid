#
# A static library containing the crashinfo protobuf generated code
#

# Compute CRASHREPORT_DIR relative to LOCAL_PATH
CRASHREPORT_DIR := $(call my-dir)
CRASHREPORT_DIR := $(CRASHREPORT_DIR:$(LOCAL_PATH)/%=%)

$(call start-emulator-library,libcrashreport_proto)

LOCAL_CFLAGS := $(EMULATOR_COMMON_CFLAGS)

LOCAL_C_INCLUDES := \
    $(PROTOBUF_INCLUDES) \

LOCAL_PROTO_SOURCES := \
    $(CRASHREPORT_DIR)/crash_info.proto \

$(call end-emulator-library)

CRASHREPORT_PROTO_INCLUDES := $(PROTOBUF_INCLUDES)
CRASHREPORT_PROTO_STATIC_LIBRARIES := \
    libcrashreport_proto \
    libmetrics_proto \
    $(PROTOBUF_STATIC_LIBRARIES) \
