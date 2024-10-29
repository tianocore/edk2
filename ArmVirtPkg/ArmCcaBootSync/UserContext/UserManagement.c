/** @file
  User Data Management interfaces.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <stdio.h>
#include <sys/stat.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include "Include/BootSyncSecureChannel.h"

#define DATA_SUFFIX_LEN     (8)
#define DATA_SUFFIX_VARDAT  "_VAR.dat"
#define DATA_SUFFIX_SECRET  "_SEC.dat"

// File Name = RPV + SUFFIX + '\0'
#define DATA_FILE_NAME_LEN  (ARM_CCA_REALM_CFG_RPV_SIZE + DATA_SUFFIX_LEN + 1)

/**
  A helper function to check if a buffer is zero.

  @param[in]  Data         A pointer to the buffer.
  @param[in]  Length       Length of the buffer to check.

  @retval BOOLEAN          TRUE if buffer is zero else FALSE.
**/
STATIC
BOOLEAN
IsZeroMem (
  IN UINT8  *Data,
  IN UINTN  Len
  )
{
  UINTN  Index;

  Index = 0;
  while (Len > 0) {
    if (Data[Index] != 0) {
      return FALSE;
    }

    Index++;
    Len--;
  }

  return TRUE;
}

/**
  Get the Boot Sync Data from 'RPV'_<VAR~SEC>.dat file.

  @param[in]  Rpv          A pointer to the RPV value.
  @param[in]  Suffix       The suffix for the data file.
  @param[out] Data         The Boot Sync Data.
  @param[out] DataSize     The Boot Sync Data size.

  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           User data management file not found.
  @retval EFI_ACCESS_DENIED       Failed to read from user data management file.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
  @retval EFI_SUCCESS             Success.
**/
STATIC
EFI_STATUS
EFIAPI
BootSyncGetData (
  IN  UINT8        *Rpv,
  IN  CONST CHAR8  *Suffix,
  OUT UINT8        **Data,
  OUT UINTN        *DataSize
  )
{
  EFI_STATUS   Status;
  CHAR8        *Buffer;
  CHAR8        FileName[DATA_FILE_NAME_LEN];
  FILE         *FileHandle;
  struct stat  st;
  size_t       BytesRead;
  size_t       BytesToRead;

  if (IsZeroMem (Rpv, ARM_CCA_REALM_CFG_RPV_SIZE)) {
    // The RPV is not specified so we cannot locate the User Data.
    DEBUG ((DEBUG_ERROR, "Error: RPV not specified!\n"));
    return EFI_INVALID_PARAMETER;
  }

  AsciiStrCpyS (FileName, DATA_FILE_NAME_LEN, (CHAR8 *)Rpv);
  AsciiStrCatS (
    FileName,
    (DATA_FILE_NAME_LEN - DATA_SUFFIX_LEN),
    Suffix
    );

  DEBUG ((DEBUG_INFO, "FileName = %a\n", FileName));

  FileHandle = fopen (FileName, "rb");
  if (FileHandle == NULL) {
    DEBUG ((DEBUG_ERROR, "Error: %a Not found!\n", FileName));
    return EFI_NOT_FOUND;
  }

  stat (FileName, &st);
  DEBUG ((DEBUG_INFO, "Info: File Size = %d\n", st.st_size));

  if (st.st_size <= 0) {
    Status = EFI_INVALID_PARAMETER;
    goto error_handler;
  }

  Buffer = AllocatePool (st.st_size);
  if (Buffer == NULL) {
    ASSERT (FALSE);
    Status =  EFI_OUT_OF_RESOURCES;
    goto error_handler;
  }

  BytesToRead = st.st_size;
  BytesRead   = fread (Buffer, 1, BytesToRead, FileHandle);
  if (BytesRead != BytesToRead) {
    ASSERT (FALSE);
    DEBUG ((
      DEBUG_ERROR,
      "Error: Failed to read file bytes read = %d\n",
      BytesRead
      ));
    Status =  EFI_ACCESS_DENIED;
    FreePool (Buffer);
    goto error_handler;
  }

  *Data     = (UINT8 *)Buffer;
  *DataSize = BytesRead;
  Status    = EFI_SUCCESS;

error_handler:
  fclose (FileHandle);
  return Status;
}

/**
  Get the UEFI Variable Data.

  @param[in]   SecChannel          Pointer to the secure channel.
  @param[out]  Data                UEFI Variable data.
  @param[out]  DataSize            UEFI Variable data size.

  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
BootSyncGetVariableData (
  IN  SECURE_CHANNEL  *SecChannel,
  OUT UINT8           **Data,
  OUT UINTN           *DataSize
  )
{
  return BootSyncGetData (SecChannel->Rpv, DATA_SUFFIX_VARDAT, Data, DataSize);
}

/**
  Get the Secret Data.

  @param[in]   SecChannel          Pointer to the secure channel.
  @param[out]  Data                Secret data.
  @param[out]  DataSize            Secret data size.

  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
BootSyncGetSecretData (
  IN SECURE_CHANNEL  *SecChannel,
  OUT UINT8          **Data,
  OUT UINTN          *DataSize
  )
{
  return BootSyncGetData (SecChannel->Rpv, DATA_SUFFIX_SECRET, Data, DataSize);
}
