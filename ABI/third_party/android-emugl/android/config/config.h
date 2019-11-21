/* This file is included by target-specific files under
 * android/config/target-$ARCH/config.h, but contains all config
 * definitions that are independent of the target CPU.
 *
 * Do not include directly.
 */
#include "config-host.h"

#define CONFIG_SOFTMMU 1
#ifndef _WIN32
#define CONFIG_NAND_LIMITS 1
#endif
