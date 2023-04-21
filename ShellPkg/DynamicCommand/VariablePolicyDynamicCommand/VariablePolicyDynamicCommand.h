/** @file
  Internal header file for the module.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef VAR_POLICY_DYNAMIC_SHELL_COMMAND_H_
#define VAR_POLICY_DYNAMIC_SHELL_COMMAND_H_

#include <Uefi.h>

#define VAR_POLICY_COMMAND_NAME  L"varpolicy"

typedef enum {
  VariableVendorCapsule,
  VariableVendorCapsuleReport,
  VariableVendorGlobal,
  VariableVendorMorControl,
  VariableVendorGuidMax
} VAR_POLICY_CMD_VENDOR_GUID_TYPE;

typedef struct {
  VAR_POLICY_CMD_VENDOR_GUID_TYPE    VendorGuidType;
  EFI_GUID                           *VendorGuid;
  CHAR16                             *Description;
} VAR_POLICY_CMD_VAR_NAMESPACE;

/**
  Log a formatted console message.

  This is not specific to this shell command but scoped so to prevent global
  name conflicts.

  The hex dump is split into lines of 16 dumped bytes.

  The full hex dump is bracketed, and its byte ascii char also print.
  If the byte value is not an ascii code, it will print as '.'

  @param[in] Offset            Offset to be display after PrefixFormat.
                               Offset will be increased for each print line.
  @param[in] Data              The data to dump.
  @param[in] DataSize          Number of bytes in Data.

**/
#define VAR_POLICY_CMD_SHELL_DUMP_HEX(Offset,                                         \
                                      Data,                                           \
                                      DataSize                                        \
                                      )                                               \
        {                                                                             \
          UINT8 *_DataToDump;                                                         \
          UINT8 _Val[50];                                                             \
          UINT8 _Str[20];                                                             \
          UINT8 _TempByte;                                                            \
          UINTN _Size;                                                                \
          UINTN _DumpHexIndex;                                                        \
          UINTN _LocalOffset;                                                         \
          UINTN _LocalDataSize;                                                       \
          CONST CHAR8 *_Hex = "0123456789ABCDEF";                                     \
          _LocalOffset = (Offset);                                                    \
          _LocalDataSize = (DataSize);                                                \
          _DataToDump = (UINT8 *)(Data);                                              \
                                                                                      \
          ASSERT (_DataToDump != NULL);                                               \
                                                                                      \
          while (_LocalDataSize != 0) {                                               \
            _Size = 16;                                                               \
            if (_Size > _LocalDataSize) {                                             \
              _Size = _LocalDataSize;                                                 \
            }                                                                         \
                                                                                      \
            for (_DumpHexIndex = 0; _DumpHexIndex < _Size; _DumpHexIndex += 1) {      \
              _TempByte            = (UINT8) _DataToDump[_DumpHexIndex];              \
              _Val[_DumpHexIndex * 3 + 0]  = (UINT8) _Hex[_TempByte >> 4];            \
              _Val[_DumpHexIndex * 3 + 1]  = (UINT8) _Hex[_TempByte & 0xF];           \
              _Val[_DumpHexIndex * 3 + 2]  =                                          \
                (CHAR8) ((_DumpHexIndex == 7) ? '-' : ' ');                           \
              _Str[_DumpHexIndex]          =                                          \
                (CHAR8) ((_TempByte < ' ' || _TempByte > '~') ? '.' : _TempByte);     \
            }                                                                         \
                                                                                      \
            _Val[_DumpHexIndex * 3]  = 0;                                             \
            _Str[_DumpHexIndex]      = 0;                                             \
                                                                                      \
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_HEX_DUMP_LINE), mVarPolicyShellCommandHiiHandle, _LocalOffset, _Val, _Str); \
            _DataToDump = (UINT8 *)(((UINTN)_DataToDump) + _Size);                    \
            _LocalOffset += _Size;                                                    \
            _LocalDataSize -= _Size;                                                  \
          }                                                                           \
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_LINE_BREAK), mVarPolicyShellCommandHiiHandle); \
        }

#endif
