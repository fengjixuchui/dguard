#pragma once

#include <ntifs.h>

UINT32
DgiCRC32_8Bytes(const PVOID data, UINT32 length, UINT32 previousCrc32);