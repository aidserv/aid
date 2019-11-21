# Build files for the Android Wear agent library and test programs.

# First, the Wear agent library, which will be linked into the emulator
# and the standalone executable.

LIBWEAR_AGENT_SOURCES := \

LIBWEAR_AGENT_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/include

# Dummy library used to avoid breaking the QEMU2 build that still depends on
# it for historical reasons. Remove this once it is cleaned up.
$(call start-emulator-library, libandroid-wear-agent)
LOCAL_SRC_FILES := android/wear-agent/dummy.c
$(call end-emulator-library)
