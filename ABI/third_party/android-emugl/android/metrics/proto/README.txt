This directory, qemu/android/metrics/proto, contains a generated protocol
definition for the Android Studio metrics.

If you need to change the proto definition, update it here:
    //depot/google3/logs/proto/wireless/android/sdk/stats/studio_stats.proto

... and then run:
    //depot/google3/wireless/android/sdk/stats/logs/update_emulator_proto.sh

... instead of editing the file in this directory directly.
