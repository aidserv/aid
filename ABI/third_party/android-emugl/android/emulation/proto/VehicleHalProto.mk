#
# A static library containing the vehicle hal protobuf generated code
#

# Compute METRICS_DIR relative to LOCAL_PATH
PROTOS_DIR := $(call my-dir)
PROTOS_DIR := $(PROTOS_DIR:$(LOCAL_PATH)/%=%)

$(call start-emulator-library,libvehicle_proto)

LOCAL_CFLAGS := $(EMULATOR_COMMON_CFLAGS)

LOCAL_C_INCLUDES := \
    $(PROTOBUF_INCLUDES) \

LOCAL_PROTO_SOURCES := \
    $(PROTOS_DIR)/VehicleHalProto.proto

$(call end-emulator-library)

VEHICLE_PROTO_INCLUDES := $(PROTOBUF_INCLUDES)
VEHICLE_PROTO_STATIC_LIBRARIES := \
    libvehicle_proto \
    $(PROTOBUF_STATIC_LIBRARIES)
