#!/bin/bash -e

LINUX_VERSION="Linux version 3.10.0+ (vharron@tifa.mtv.corp.google.com) (gcc version 4.7 (GCC) ) #1 PREEMPT Sat Jan 5 2:45:24 PDT 2008\xA\x0"
TMP_FILE=/tmp/print_mock_kernel_tmp

printf "// a mock uncompressed kernel\n"
printf "// ELF header, followed by an unspecified number of bytes\n"
printf "// followed by a 'Linux version ' string\n"
printf "static const unsigned char kMockKernelElf[] = {\n"
printf '\x7f'ELF0123456789"${LINUX_VERSION}"0123456789 | xxd -i
printf "};\n\n"

printf "// a mock uncompressed kernel without version string\n"
printf "// ELF header, followed by an unspecified number of bytes\n"
printf "static const unsigned char kMockKernelElfNoString[] = {\n"
printf '\x7f'ELF0123456789012345678901234567890123456789 | xxd -i
printf "};\n\n"

printf "// a mock compressed kernel\n"
printf "// an unspecified number of bytes, followed by a gzip header\n"
printf "// gzip stream starts 10 bytes after gzip header, uncompressed gzip stream\n"
printf "// begins with an ELF header as above\n"
printf "static const unsigned char kMockKernelGzip[] = {\n"
printf "0123456789" > $TMP_FILE
printf '\x7f'ELF0123456789"${LINUX_VERSION}"0123456789 | gzip >> $TMP_FILE
cat $TMP_FILE | xxd -i
printf "};\n\n"

printf "// a mock semi-compressed kernel\n"
printf "// an unspecified number of bytes, followed by a version string,\n"
printf "// followed by a gzip header\n"
printf "static const unsigned char kMockKernelSemiCompressed[] = {\n"
printf "01234567899abcdef${LINUX_VERSION}0123455789" > $TMP_FILE
printf "WHATEVER" | gzip >> $TMP_FILE
cat $TMP_FILE | xxd -i
printf "};\n\n"

rm $TMP_FILE


