# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

# Static library.
# =================================================
include $(CLEAR_VARS)

LOCAL_MODULE := breakpad_client
LOCAL_LDLIBS += -lstdc++
LOCAL_CPPFLAGS += -std=c++11

LOCAL_CPP_EXTENSION := .cc

LOCAL_SRC_FILES := \
    src/client/linux/dump_writer_common/seccomp_unwinder.cc \
    src/client/linux/dump_writer_common/thread_info.cc \
    src/client/linux/dump_writer_common/ucontext_reader.cc \
    src/client/linux/handler/minidump_descriptor.cc \
    src/client/linux/minidump_writer/linux_core_dumper.cc \
    src/client/linux/minidump_writer/linux_dumper.cc \
    src/client/linux/minidump_writer/linux_ptrace_dumper.cc \
    src/client/linux/minidump_writer/minidump_writer.cc \
    src/client/minidump_file_writer.cc \
    src/common/convert_UTF.c \
    src/common/linux/elf_core_dump.cc \
    src/common/linux/elfutils.cc \
    src/common/linux/file_id.cc \
    src/common/linux/guid_creator.cc \
    src/common/linux/linux_libc_support.cc \
    src/common/linux/memory_mapped_file.cc \
    src/common/linux/safe_readlink.cc \
    src/common/string_conversion.cc \

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/src/common/android/include \
    $(LOCAL_PATH)/src \
    external/libcxx/include 	\
	$(LOCAL_PATH)/src	\
	$(LOCAL_PATH)/src/common/android/include

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_C_INCLUDES)

# Work around b/25435766 core2md segfault.
LOCAL_CLANG_CFLAGS_x86 += -mno-stackrealign
LOCAL_CLANG := true

include $(BUILD_STATIC_LIBRARY)

# core2md binary.
# =================================================
include $(CLEAR_VARS)

LOCAL_MODULE := core2md

LOCAL_CPP_EXTENSION := .cc

LOCAL_SRC_FILES := src/tools/linux/core2md/core2md.cc

LOCAL_STATIC_LIBRARIES := breakpad_client

# Work around b/25435766 core2md segfault.
LOCAL_CLANG_CFLAGS_x86 += -mno-stackrealign
LOCAL_CLANG := true

include $(BUILD_EXECUTABLE)

# dump_syms host tool.
# =================================================
include $(CLEAR_VARS)
LOCAL_MODULE := dump_syms
LOCAL_MODULE_HOST_OS := linux
LOCAL_CLANG := true
LOCAL_CPPFLAGS_linux := -DHAVE_A_OUT_H -Wno-unused-parameter
LOCAL_CPP_EXTENSION := .cc
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/src
LOCAL_SRC_FILES := \
    src/common/dwarf/bytereader.cc \
    src/common/dwarf/dwarf2diehandler.cc \
    src/common/dwarf/dwarf2reader.cc \
    src/common/dwarf_cfi_to_module.cc \
    src/common/dwarf_cu_to_module.cc \
    src/common/dwarf_line_to_module.cc \
    src/common/language.cc \
    src/common/module.cc \
    src/common/stabs_reader.cc \
    src/common/stabs_to_module.cc \
    src/common/linux/crc32.cc \
    src/common/linux/dump_symbols.cc \
    src/common/linux/elfutils.cc \
    src/common/linux/elf_symbols_to_module.cc \
    src/common/linux/file_id.cc \
    src/common/linux/linux_libc_support.cc \
    src/common/linux/memory_mapped_file.cc \
    src/tools/linux/dump_syms/dump_syms.cc

#LOCAL_SRC_FILES_linux := \
#    src/common/linux/crc32.cc \
#    src/common/linux/dump_symbols.cc \
#    src/common/linux/elfutils.cc \
#    src/common/linux/elf_symbols_to_module.cc \
#    src/common/linux/file_id.cc \
#    src/common/linux/linux_libc_support.cc \
#    src/common/linux/memory_mapped_file.cc \
#    src/tools/linux/dump_syms/dump_syms.cc

include $(BUILD_HOST_EXECUTABLE)
