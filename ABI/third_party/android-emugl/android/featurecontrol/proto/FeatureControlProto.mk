#
# A static library containing the featurecontrol protobuf generated code
#

# Compute FEATURECONTROL_DIR relative to LOCAL_PATH
FEATURECONTROL_DIR := $(call my-dir)
FEATURECONTROL_DIR := $(FEATURECONTROL_DIR:$(LOCAL_PATH)/%=%)

$(call start-emulator-library,libfeaturecontrol_proto)

LOCAL_CFLAGS := $(EMULATOR_COMMON_CFLAGS)

LOCAL_C_INCLUDES := \
    $(PROTOBUF_INCLUDES) \

LOCAL_PROTO_SOURCES := \
    $(FEATURECONTROL_DIR)/emulator_features.proto \
    $(FEATURECONTROL_DIR)/emulator_feature_patterns.proto \

$(call end-emulator-library)

FEATURECONTROL_PROTO_INCLUDES := $(PROTOBUF_INCLUDES)
FEATURECONTROL_PROTO_STATIC_LIBRARIES := \
    libfeaturecontrol_proto \
    $(PROTOBUF_STATIC_LIBRARIES) \
