/** @file
  File System Access

  Copyright (c) 2004 - 2009, Intel Corporation. <BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "NvVarsFileLib.h"

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>


/**
  Writes the variable into the file so it can be restored from
  the file on future boots of the system.

  @param[in]  File - The file to write to
  @param[in]  Name - Variable name string
  @param[in]  NameSize - Size of Name in bytes
  @param[in]  Guid - GUID of variable
  @param[in]  Attributes - Attributes of variable
  @param[in]  Data - Buffer containing Data for variable
  @param[in]  DataSize - Size of Data in bytes

  @return     EFI_STATUS based on the success or failure of the operation

**/
EFI_STATUS
PackVariableIntoFile (
  IN EFI_FILE_HANDLE  File,
  IN CHAR16           *Name,
  IN UINT32           NameSize,
  IN EFI_GUID         *Guid,
  IN UINT32           Attributes,
  IN VOID             *Data,
  IN UINT32           DataSize
  )
{
  EFI_STATUS  Status;
  UINTN       WriteSize;

  WriteSize = sizeof (NameSize);
  Status = FileHandleWrite (File, &WriteSize, &NameSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  WriteSize = NameSize;
  Status = FileHandleWrite (File, &WriteSize, (VOID*) Name);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  WriteSize = sizeof (*Guid);
  Status = FileHandleWrite (File, &WriteSize, (VOID*) Guid);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  WriteSize = sizeof (Attributes);
  Status = FileHandleWrite (File, &WriteSize, &Attributes);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  WriteSize = sizeof (DataSize);
  Status = FileHandleWrite (File, &WriteSize, &DataSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  WriteSize = DataSize;
  Status = FileHandleWrite (File, &WriteSize, Data);

  return Status;
}


/**
  Unpacks the next variable from the NvVars file data

  @param[in]  Buffer - Buffer pointing to the next variable instance
                       On subsequent calls, the pointer should be incremented
                       by the returned SizeUsed value.
  @param[in]  MaxSize - Max allowable size for the variable data
                        On subsequent calls, this should be decremented
                        by the returned SizeUsed value.
  @param[out] Name - Variable name string (address in Buffer)
  @param[out] NameSize - Size of Name in bytes
  @param[out] Guid - GUID of variable (address in Buffer)
  @param[out] Attributes - Attributes of variable
  @param[out] Data - Buffer containing Data for variable (address in Buffer)
  @param[out] DataSize - Size of Data in bytes
  @param[out] SizeUsed - Total size used for this variable instance in Buffer

  @return     EFI_STATUS based on the success or failure of the operation

**/
EFI_STATUS
UnpackVariableFromBuffer (
  IN  VOID     *Buffer,
  IN  UINTN    MaxSize,
  OUT CHAR16   **Name,
  OUT UINT32   *NameSize,
  OUT EFI_GUID **Guid,
  OUT UINT32   *Attributes,
  OUT UINT32   *DataSize,
  OUT VOID     **Data,
  OUT UINTN    *SizeUsed
  )
{
  UINT8  *BytePtr;
  UINTN  Offset;

  BytePtr = (UINT8*)Buffer;
  Offset = 0;

  *NameSize = *(UINT32*) (BytePtr + Offset);
  Offset = Offset + sizeof (UINT32);

  if (Offset > MaxSize) {
    return EFI_INVALID_PARAMETER;
  }

  *Name = (CHAR16*) (BytePtr + Offset);
  Offset = Offset + *(UINT32*)BytePtr;
  if (Offset > MaxSize) {
    return EFI_INVALID_PARAMETER;
  }

  *Guid = (EFI_GUID*) (BytePtr + Offset);
  Offset = Offset + sizeof (EFI_GUID);
  if (Offset > MaxSize) {
    return EFI_INVALID_PARAMETER;
  }

  *Attributes = *(UINT32*) (BytePtr + Offset);
  Offset = Offset + sizeof (UINT32);
  if (Offset > MaxSize) {
    return EFI_INVALID_PARAMETER;
  }

  *DataSize = *(UINT32*) (BytePtr + Offset);
  Offset = Offset + sizeof (UINT32);
  if (Offset > MaxSize) {
    return EFI_INVALID_PARAMETER;
  }

  *Data = (VOID*) (BytePtr + Offset);
  Offset = Offset + *DataSize;
  if (Offset > MaxSize) {
    return EFI_INVALID_PARAMETER;
  }

  *SizeUsed = Offset;

  return EFI_SUCCESS;
}


/**
  Examines the NvVars file contents, and updates variables based on it.

  @param[in]  Buffer - Buffer with NvVars data
  @param[in]  MaxSize - Size of Buffer in bytes
  @param[in]  DryRun - If TRUE, then no variable modifications should be made
                       (If TRUE, the Buffer is still parsed for validity.)

  @return     EFI_STATUS based on the success or failure of the operation

**/
EFI_STATUS
UnpackVariablesFromBuffer (
  IN  VOID     *Buffer,
  IN  UINTN    MaxSize,
  IN  BOOLEAN  DryRun
  )
{
  EFI_STATUS  Status;
  UINTN       Count;
  UINTN       TotalSizeUsed;
  UINTN       SizeUsed;

  CHAR16      *Name;
  UINT32      NameSize;
  CHAR16      *AlignedName;
  UINT32      AlignedNameMaxSize;
  EFI_GUID    *Guid;
  UINT32      Attributes;
  UINT32      DataSize;
  VOID        *Data;

  AlignedName = NULL;
  AlignedNameMaxSize = 0;

  for (
    Status = EFI_SUCCESS, Count = 0, TotalSizeUsed = 0;
    !EFI_ERROR (Status) && (TotalSizeUsed < MaxSize);
    ) {
    Status = UnpackVariableFromBuffer (
               (VOID*) ((UINT8*) Buffer + TotalSizeUsed),
               (MaxSize - TotalSizeUsed),
               &Name,
               &NameSize,
               &Guid,
               &Attributes,
               &DataSize,
               &Data,
               &SizeUsed
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // We copy the name to a separately allocated buffer,
    // to be sure it is 16-bit aligned.
    //
    if (NameSize > AlignedNameMaxSize) {
      if (AlignedName != NULL) {
        FreePool (AlignedName);
      }
      AlignedName = AllocatePool (NameSize);
    }
    if (AlignedName == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    CopyMem (AlignedName, Name, NameSize);

    DEBUG ((
      EFI_D_INFO,
      "Unpacked variable %g:%s\n",
      Guid,
      AlignedName
      ));

    TotalSizeUsed = TotalSizeUsed + SizeUsed;

    DEBUG ((
      EFI_D_INFO,
      "TotalSizeUsed(%d); MaxSize(%d)\n",
      TotalSizeUsed,
      MaxSize
      ));

    if (!DryRun) {
      //
      // Set the variable contents
      //
      gRT->SetVariable (
             AlignedName,
             Guid,
             Attributes,
             DataSize,
             Data
             );

      Count++;

      DEBUG ((
        EFI_D_INFO,
        "Restored variable %g:%s\n",
        Guid,
        AlignedName
        ));
    }

  }

  if (AlignedName != NULL) {
    FreePool (AlignedName);
  }

  //
  // Make sure the entire buffer was used, or else return an error
  //
  if (TotalSizeUsed != MaxSize) {
    DEBUG ((
      EFI_D_INFO,
      "TotalSizeUsed(%d) != MaxSize(%d)\n",
      TotalSizeUsed,
      MaxSize
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (Count > 0) {
    DEBUG ((
      EFI_D_INFO,
      "Restored %d Variables\n",
      Count
      ));
  }

  return EFI_SUCCESS;
}


/**
  Examines the NvVars file contents, and updates variables based on it.

  @param[in]  VarsBuffer - Buffer with NvVars data
  @param[in]  VarsBufferSize - Size of VarsBuffer in bytes

  @return     EFI_STATUS based on the success or failure of the operation

**/
EFI_STATUS
SetVariablesFromBuffer (
  IN VOID   *VarsBuffer,
  IN UINTN  VarsBufferSize
  )
{
  EFI_STATUS  Status;

  //
  // First test to make sure the entire buffer is in a good state
  //
  Status = UnpackVariablesFromBuffer (VarsBuffer, VarsBufferSize, TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "NvVars buffer format was invalid\n"));
    return Status;
  }

  //
  // Now, actually restore the variables.
  //
  Status = UnpackVariablesFromBuffer (VarsBuffer, VarsBufferSize, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return Status;
}

