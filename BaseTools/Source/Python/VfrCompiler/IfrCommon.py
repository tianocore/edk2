## @file
# This file is used to define common funtions.
#
# Copyright (c) 2022-, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import Common.EdkLogger as EdkLogger
from VfrCompiler.IfrCtypes import EFI_GUID
from Common.BuildToolError import PARAMETER_INVALID

# Enumeration of EFI_STATUS.
RETURN_SUCCESS = EFI_SUCCESS = 0
EFI_BUFFER_TOO_SMALL = 0x8000000000000000 | (5)
EFI_ABORTED = 0x8000000000000000 | (21)
EFI_OUT_OF_RESOURCES = 0x8000000000000000 | (9)
EFI_INVALID_PARAMETER = 0x8000000000000000 | (2)
EFI_NOT_FOUND = 0x8000000000000000 | (14)
RETURN_INVALID_PARAMETER = 0x8000000000000000 | (2)
RETURN_UNSUPPORTED = 0x8000000000000000 | (3)

ASCII_GUID_BUFFER_INDEX_1 = 8
ASCII_GUID_BUFFER_INDEX_2 = 13
ASCII_GUID_BUFFER_INDEX_3 = 18
ASCII_GUID_BUFFER_INDEX_4 = 23
ASCII_GUID_BUFFER_MAX_INDEX = 36
GUID_BUFFER_VALUE_LEN = 11


def EFI_ERROR(A):
    return A < 0


# Converts a string to an EFI_GUID.
def StringToGuid(AsciiGuidBuffer: str, GuidBuffer: EFI_GUID):
    Data4 = [0] * 8
    if AsciiGuidBuffer is None or GuidBuffer is None:
        return EFI_INVALID_PARAMETER
    Index = 0
    while Index < ASCII_GUID_BUFFER_MAX_INDEX:
        if (
            Index == ASCII_GUID_BUFFER_INDEX_1
            or Index == ASCII_GUID_BUFFER_INDEX_2
            or Index == ASCII_GUID_BUFFER_INDEX_3
            or Index == ASCII_GUID_BUFFER_INDEX_4
        ):
            if AsciiGuidBuffer[Index] != "-":
                break
        elif (
            (AsciiGuidBuffer[Index] >= "0" and AsciiGuidBuffer[Index] <= "9")
            or (AsciiGuidBuffer[Index] >= "a" and AsciiGuidBuffer[Index] <= "f")
            or (AsciiGuidBuffer[Index] >= "A" and AsciiGuidBuffer[Index] <= "F")
        ):
            Index += 1
            continue
        else:
            break
        Index += 1
        continue

    if Index < ASCII_GUID_BUFFER_MAX_INDEX:
        EdkLogger.error("VfrCompiler", PARAMETER_INVALID, "Invalid option value")
        return EFI_ABORTED
    Index = GUID_BUFFER_VALUE_LEN
    try:
        Data1 = int(AsciiGuidBuffer[0:8], 16)
        Data2 = int(AsciiGuidBuffer[9:13], 16)
        Data3 = int(AsciiGuidBuffer[14:18], 16)
        Data4[0] = int(AsciiGuidBuffer[19:21], 16)
        Data4[1] = int(AsciiGuidBuffer[21:23], 16)
        Data4[2] = int(AsciiGuidBuffer[24:26], 16)
        Data4[3] = int(AsciiGuidBuffer[26:28], 16)
        Data4[4] = int(AsciiGuidBuffer[28:30], 16)
        Data4[5] = int(AsciiGuidBuffer[30:32], 16)
        Data4[6] = int(AsciiGuidBuffer[32:34], 16)
        Data4[7] = int(AsciiGuidBuffer[34:36], 16)
    except Exception:
        EdkLogger.error("VfrCompiler", PARAMETER_INVALID, "Invalid Data value!")
        Index = 0

    # Verify the correct number of items were scanned.
    if Index != GUID_BUFFER_VALUE_LEN:
        EdkLogger.error("VfrCompiler", PARAMETER_INVALID, "Invalid option value")
        return EFI_ABORTED

    # Copy the data into our GUID.
    GuidBuffer.Data1 = Data1
    GuidBuffer.Data2 = Data2
    GuidBuffer.Data3 = Data3
    GuidBuffer.Data4[0] = Data4[0]
    GuidBuffer.Data4[1] = Data4[1]
    GuidBuffer.Data4[2] = Data4[2]
    GuidBuffer.Data4[3] = Data4[3]
    GuidBuffer.Data4[4] = Data4[4]
    GuidBuffer.Data4[5] = Data4[5]
    GuidBuffer.Data4[6] = Data4[6]
    GuidBuffer.Data4[7] = Data4[7]
    Status = EFI_SUCCESS

    return Status, GuidBuffer
