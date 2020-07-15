/** @file
  INI configuration parsing library.

  The INI file format is:
    ================
    [SectionName]
    EntryName=EntryValue
    ================

    Where:
      1) SectionName is an ASCII string. The valid format is [A-Za-z0-9_]+
      2) EntryName is an ASCII string. The valid format is [A-Za-z0-9_]+
      3) EntryValue can be:
         3.1) an ASCII String. The valid format is [A-Za-z0-9_]+
         3.2) a GUID. The valid format is xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx, where x is [A-Fa-f0-9]
         3.3) a decimal value. The valid format is [0-9]+
         3.4) a hexadecimal value. The valid format is 0x[A-Fa-f0-9]+
      4) '#' or ';' can be used as comment at anywhere.
      5) TAB(0x20) or SPACE(0x9) can be used as separator.
      6) LF(\n, 0xA) or CR(\r, 0xD) can be used as line break.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef __INI_PARSING_LIB_H__
#define __INI_PARSING_LIB_H__

/**
  Open an INI config file and return a context.

  @param[in] DataBuffer      Config raw file buffer.
  @param[in] BufferSize      Size of raw buffer.

  @return       Config data buffer is opened and context is returned.
  @retval NULL  No enough memory is allocated.
  @retval NULL  Config data buffer is invalid.
**/
VOID *
EFIAPI
OpenIniFile (
  IN      UINT8                         *DataBuffer,
  IN      UINTN                         BufferSize
  );

/**
  Get section entry string value.

  @param[in]  Context         INI Config file context.
  @param[in]  SectionName     Section name.
  @param[in]  EntryName       Section entry name.
  @param[out] EntryValue      Point to the got entry string value.

  @retval EFI_SUCCESS    Section entry string value is got.
  @retval EFI_NOT_FOUND  Section is not found.
**/
EFI_STATUS
EFIAPI
GetStringFromDataFile (
  IN      VOID                          *Context,
  IN      CHAR8                         *SectionName,
  IN      CHAR8                         *EntryName,
  OUT     CHAR8                         **EntryValue
  );

/**
  Get section entry GUID value.

  @param[in]  Context         INI Config file context.
  @param[in]  SectionName     Section name.
  @param[in]  EntryName       Section entry name.
  @param[out] Guid            Point to the got GUID value.

  @retval EFI_SUCCESS    Section entry GUID value is got.
  @retval EFI_NOT_FOUND  Section is not found.
**/
EFI_STATUS
EFIAPI
GetGuidFromDataFile (
  IN      VOID                          *Context,
  IN      CHAR8                         *SectionName,
  IN      CHAR8                         *EntryName,
  OUT     EFI_GUID                      *Guid
  );

/**
  Get section entry decimal UINTN value.

  @param[in]  Context         INI Config file context.
  @param[in]  SectionName     Section name.
  @param[in]  EntryName       Section entry name.
  @param[out] Data            Point to the got decimal UINTN value.

  @retval EFI_SUCCESS    Section entry decimal UINTN value is got.
  @retval EFI_NOT_FOUND  Section is not found.
**/
EFI_STATUS
EFIAPI
GetDecimalUintnFromDataFile (
  IN      VOID                          *Context,
  IN      CHAR8                         *SectionName,
  IN      CHAR8                         *EntryName,
  OUT     UINTN                         *Data
  );

/**
  Get section entry hexadecimal UINTN value.

  @param[in]  Context         INI Config file context.
  @param[in]  SectionName     Section name.
  @param[in]  EntryName       Section entry name.
  @param[out] Data            Point to the got hexadecimal UINTN value.

  @retval EFI_SUCCESS    Section entry hexadecimal UINTN value is got.
  @retval EFI_NOT_FOUND  Section is not found.
**/
EFI_STATUS
EFIAPI
GetHexUintnFromDataFile (
  IN      VOID                          *Context,
  IN      CHAR8                         *SectionName,
  IN      CHAR8                         *EntryName,
  OUT     UINTN                         *Data
  );

/**
  Get section entry hexadecimal UINT64 value.

  @param[in]  Context         INI Config file context.
  @param[in]  SectionName     Section name.
  @param[in]  EntryName       Section entry name.
  @param[out] Data            Point to the got hexadecimal UINT64 value.

  @retval EFI_SUCCESS    Section entry hexadecimal UINT64 value is got.
  @retval EFI_NOT_FOUND  Section is not found.
**/
EFI_STATUS
EFIAPI
GetHexUint64FromDataFile (
  IN      VOID                          *Context,
  IN      CHAR8                         *SectionName,
  IN      CHAR8                         *EntryName,
  OUT     UINT64                        *Data
  );

/**
  Close an INI config file and free the context.

  @param[in] Context         INI Config file context.
**/
VOID
EFIAPI
CloseIniFile (
  IN      VOID                          *Context
  );

#endif

