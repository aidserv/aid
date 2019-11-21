#
# A static library containing the metrics protobuf generated code
#

# Compute METRICS_DIR relative to LOCAL_PATH
METRICS_DIR := $(call my-dir)
METRICS_DIR := $(METRICS_DIR:$(LOCAL_PATH)/%=%)

$(call start-emulator-library,libmetrics_proto)

LOCAL_CFLAGS := $(EMULATOR_COMMON_CFLAGS)

LOCAL_C_INCLUDES := \
    $(PROTOBUF_INCLUDES) \

LOCAL_PROTO_SOURCES := \
    $(METRICS_DIR)/device_restriction_enum.proto \
    $(METRICS_DIR)/clientanalytics.proto \
    $(METRICS_DIR)/studio_stats.proto \

$(call end-emulator-library)

METRICS_PROTO_INCLUDES := $(PROTOBUF_INCLUDES)
METRICS_PROTO_STATIC_LIBRARIES := \
    libmetrics_proto \
    $(PROTOBUF_STATIC_LIBRARIES)
