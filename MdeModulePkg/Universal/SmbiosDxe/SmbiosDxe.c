/** @file
  This code produces the Smbios protocol. It also responsible for constructing
  SMBIOS table into system table.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosDxe.h"

//
// Module Global:
// Since this driver will only ever produce one instance of the
// protocol you are not required to dynamically allocate the PrivateData.
//
SMBIOS_INSTANCE mPrivateData;

UINTN mPreAllocatedPages      = 0;
UINTN mPre64BitAllocatedPages = 0;

//
// Chassis for SMBIOS entry point structure that is to be installed into EFI system config table.
//
SMBIOS_TABLE_ENTRY_POINT *EntryPointStructure    = NULL;
SMBIOS_TABLE_ENTRY_POINT EntryPointStructureData = {
  //
  // AnchorString
  //
  {
    0x5f,
    0x53,
    0x4d,
    0x5f
  },
  //
  // EntryPointStructureChecksum,TO BE FILLED
  //
  0,
  //
  // EntryPointStructure Length
  //
  0x1f,
  //
  // MajorVersion
  //
  0,
  //
  // MinorVersion
  //
  0,
  //
  // MaxStructureSize, TO BE FILLED
  //
  0,
  //
  // EntryPointRevision
  //
  0,
  //
  // FormattedArea
  //
  {
    0,
    0,
    0,
    0,
    0
  },
  //
  // IntermediateAnchorString
  //
  {
    0x5f,
    0x44,
    0x4d,
    0x49,
    0x5f
  },
  //
  // IntermediateChecksum, TO BE FILLED
  //
  0,
  //
  // TableLength, TO BE FILLED
  //
  0,
  //
  // TableAddress, TO BE FILLED
  //
  0,
  //
  // NumberOfSmbiosStructures, TO BE FILLED
  //
  0,
  //
  // SmbiosBcdRevision
  //
  0
};

SMBIOS_TABLE_3_0_ENTRY_POINT *Smbios30EntryPointStructure    = NULL;
SMBIOS_TABLE_3_0_ENTRY_POINT Smbios30EntryPointStructureData = {
  //
  // AnchorString _SM3_
  //
  {
    0x5f,
    0x53,
    0x4d,
    0x33,
    0x5f,
  },
  //
  // EntryPointStructureChecksum,TO BE FILLED
  //
  0,
  //
  // EntryPointLength
  //
  0x18,
  //
  // MajorVersion
  //
  0,
  //
  // MinorVersion
  //
  0,
  //
  // DocRev
  //
  0,
  //
  // EntryPointRevision
  //
  0x01,
  //
  // Reserved
  //
  0,
  //
  // TableMaximumSize,TO BE FILLED
  //
  0,
  //
  // TableAddress,TO BE FILLED
  //
  0
};
/**

  Get the full size of SMBIOS structure including optional strings that follow the formatted structure.

  @param This                   The EFI_SMBIOS_PROTOCOL instance.
  @param Head                   Pointer to the beginning of SMBIOS structure.
  @param Size                   The returned size.
  @param NumberOfStrings        The returned number of optional strings that follow the formatted structure.

  @retval EFI_SUCCESS           Size retured in Size.
  @retval EFI_INVALID_PARAMETER Input SMBIOS structure mal-formed or Size is NULL.

**/
EFI_STATUS
EFIAPI
GetSmbiosStructureSize (
  IN   CONST EFI_SMBIOS_PROTOCOL        *This,
  IN   EFI_SMBIOS_TABLE_HEADER          *Head,
  OUT  UINTN                            *Size,
  OUT  UINTN                            *NumberOfStrings
  )
{
  UINTN  FullSize;
  UINTN  StrLen;
  UINTN  MaxLen;
  INT8*  CharInStr;

  if (Size == NULL || NumberOfStrings == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FullSize = Head->Length;
  CharInStr = (INT8*)Head + Head->Length;
  *Size = FullSize;
  *NumberOfStrings = 0;
  StrLen = 0;
  //
  // look for the two consecutive zeros, check the string limit by the way.
  //
  while (*CharInStr != 0 || *(CharInStr+1) != 0) {
    if (*CharInStr == 0) {
      *Size += 1;
      CharInStr++;
    }

    if (This->MajorVersion < 2 || (This->MajorVersion == 2 && This->MinorVersion < 7)){
      MaxLen = SMBIOS_STRING_MAX_LENGTH;
    } else if (This->MajorVersion < 3) {
      //
      // Reference SMBIOS 2.7, chapter 6.1.3, it will have no limit on the length of each individual text string.
      // However, the length of the entire structure table (including all strings) must be reported
      // in the Structure Table Length field of the SMBIOS Structure Table Entry Point,
      // which is a WORD field limited to 65,535 bytes.
      //
      MaxLen = SMBIOS_TABLE_MAX_LENGTH;
    } else {
      //
      // SMBIOS 3.0 defines the Structure table maximum size as DWORD field limited to 0xFFFFFFFF bytes.
      // Locate the end of string as long as possible.
      //
      MaxLen = SMBIOS_3_0_TABLE_MAX_LENGTH;
    }

    for (StrLen = 0 ; StrLen < MaxLen; StrLen++) {
      if (*(CharInStr+StrLen) == 0) {
        break;
      }
    }

    if (StrLen == MaxLen) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // forward the pointer
    //
    CharInStr += StrLen;
    *Size += StrLen;
    *NumberOfStrings += 1;
  }

  //
  // count ending two zeros.
  //
  *Size += 2;
  return EFI_SUCCESS;
}

/**

  Determin whether an SmbiosHandle has already in use.

  @param Head        Pointer to the beginning of SMBIOS structure.
  @param Handle      A unique handle will be assigned to the SMBIOS record.

  @retval TRUE       Smbios handle already in use.
  @retval FALSE      Smbios handle is NOT used.

**/
BOOLEAN
EFIAPI
CheckSmbiosHandleExistance (
  IN  LIST_ENTRY           *Head,
  IN  EFI_SMBIOS_HANDLE    Handle
  )
{
  LIST_ENTRY              *Link;
  SMBIOS_HANDLE_ENTRY     *HandleEntry;

  for (Link = Head->ForwardLink; Link != Head; Link = Link->ForwardLink) {
    HandleEntry = SMBIOS_HANDLE_ENTRY_FROM_LINK(Link);
    if (HandleEntry->SmbiosHandle == Handle) {
      return TRUE;
    }
  }

  return FALSE;
}

/**

  Get the max SmbiosHandle that could be use.

  @param  This           The EFI_SMBIOS_PROTOCOL instance.
  @param  MaxHandle      The max handle that could be assigned to the SMBIOS record.

**/
VOID
EFIAPI
GetMaxSmbiosHandle (
  IN CONST  EFI_SMBIOS_PROTOCOL   *This,
  IN OUT    EFI_SMBIOS_HANDLE     *MaxHandle
  )
{
  if (This->MajorVersion == 2 && This->MinorVersion == 0) {
    *MaxHandle = 0xFFFE;
  } else {
    *MaxHandle = 0xFEFF;
  }
}

/**

  Get an SmbiosHandle that could use.

  @param  This                   The EFI_SMBIOS_PROTOCOL instance.
  @param  SmbiosHandle           A unique handle will be assigned to the SMBIOS record.

  @retval EFI_SUCCESS            Smbios handle got.
  @retval EFI_OUT_OF_RESOURCES   Smbios handle is NOT available.

**/
EFI_STATUS
EFIAPI
GetAvailableSmbiosHandle (
  IN CONST EFI_SMBIOS_PROTOCOL   *This,
  IN OUT   EFI_SMBIOS_HANDLE     *Handle
  )
{
  LIST_ENTRY              *Head;
  SMBIOS_INSTANCE         *Private;
  EFI_SMBIOS_HANDLE       MaxSmbiosHandle;
  EFI_SMBIOS_HANDLE       AvailableHandle;

  GetMaxSmbiosHandle(This, &MaxSmbiosHandle);

  Private = SMBIOS_INSTANCE_FROM_THIS (This);
  Head = &Private->AllocatedHandleListHead;
  for (AvailableHandle = 0; AvailableHandle < MaxSmbiosHandle; AvailableHandle++) {
    if (!CheckSmbiosHandleExistance(Head, AvailableHandle)) {
      *Handle = AvailableHandle;
      return EFI_SUCCESS;
    }
  }

  return EFI_OUT_OF_RESOURCES;
}


/**
  Add an SMBIOS record.

  @param  This                  The EFI_SMBIOS_PROTOCOL instance.
  @param  ProducerHandle        The handle of the controller or driver associated with the SMBIOS information. NULL
                                means no handle.
  @param  SmbiosHandle          On entry, the handle of the SMBIOS record to add. If FFFEh, then a unique handle
                                will be assigned to the SMBIOS record. If the SMBIOS handle is already in use,
                                EFI_ALREADY_STARTED is returned and the SMBIOS record is not updated.
  @param  Record                The data for the fixed portion of the SMBIOS record. The format of the record is
                                determined by EFI_SMBIOS_TABLE_HEADER.Type. The size of the formatted area is defined
                                by EFI_SMBIOS_TABLE_HEADER.Length and either followed by a double-null (0x0000) or
                                a set of null terminated strings and a null.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_OUT_OF_RESOURCES  Record was not added due to lack of system resources.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed in was already in use.

**/
EFI_STATUS
EFIAPI
SmbiosAdd (
  IN CONST EFI_SMBIOS_PROTOCOL  *This,
  IN EFI_HANDLE                 ProducerHandle, OPTIONAL
  IN OUT EFI_SMBIOS_HANDLE      *SmbiosHandle,
  IN EFI_SMBIOS_TABLE_HEADER    *Record
  )
{
  VOID                        *Raw;
  UINTN                       TotalSize;
  UINTN                       RecordSize;
  UINTN                       StructureSize;
  UINTN                       NumberOfStrings;
  EFI_STATUS                  Status;
  LIST_ENTRY                  *Head;
  SMBIOS_INSTANCE             *Private;
  EFI_SMBIOS_ENTRY            *SmbiosEntry;
  EFI_SMBIOS_HANDLE           MaxSmbiosHandle;
  SMBIOS_HANDLE_ENTRY         *HandleEntry;
  EFI_SMBIOS_RECORD_HEADER    *InternalRecord;
  BOOLEAN                     Smbios32BitTable;
  BOOLEAN                     Smbios64BitTable;

  if (SmbiosHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = SMBIOS_INSTANCE_FROM_THIS (This);
  //
  // Check whether SmbiosHandle is already in use
  //
  Head = &Private->AllocatedHandleListHead;
  if (*SmbiosHandle != SMBIOS_HANDLE_PI_RESERVED && CheckSmbiosHandleExistance(Head, *SmbiosHandle)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // when SmbiosHandle is 0xFFFE, an available handle will be assigned
  //
  if (*SmbiosHandle == SMBIOS_HANDLE_PI_RESERVED) {
    Status = GetAvailableSmbiosHandle(This, SmbiosHandle);
    if (EFI_ERROR(Status)) {
      return Status;
    }
  } else {
    //
    // Check this handle validity
    //
    GetMaxSmbiosHandle(This, &MaxSmbiosHandle);
    if (*SmbiosHandle > MaxSmbiosHandle) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Calculate record size and string number
  //
  Status = GetSmbiosStructureSize(This, Record, &StructureSize, &NumberOfStrings);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Smbios32BitTable = FALSE;
  Smbios64BitTable = FALSE;
  if ((This->MajorVersion < 0x3) ||
      ((This->MajorVersion >= 0x3) && ((PcdGet32 (PcdSmbiosEntryPointProvideMethod) & BIT0) == BIT0))) {
    //
    // For SMBIOS 32-bit table, the length of the entire structure table (including all strings) must be reported
    // in the Structure Table Length field of the SMBIOS Structure Table Entry Point,
    // which is a WORD field limited to 65,535 bytes. So the max size of 32-bit table should not exceed 65,535 bytes.
    //
    if ((EntryPointStructure != NULL) &&
        (EntryPointStructure->TableLength + StructureSize > SMBIOS_TABLE_MAX_LENGTH)) {
      DEBUG ((EFI_D_INFO, "SmbiosAdd: Total length exceeds max 32-bit table length with type = %d size = 0x%x\n", Record->Type, StructureSize));
    } else {
      Smbios32BitTable = TRUE;
      DEBUG ((EFI_D_INFO, "SmbiosAdd: Smbios type %d with size 0x%x is added to 32-bit table\n", Record->Type, StructureSize));
    }
  }

  //
  // For SMBIOS 3.0, Structure table maximum size in Entry Point structure is DWORD field limited to 0xFFFFFFFF bytes.
  //
  if ((This->MajorVersion >= 0x3) && ((PcdGet32 (PcdSmbiosEntryPointProvideMethod) & BIT1) == BIT1)) {
    //
    // For SMBIOS 64-bit table, Structure table maximum size in SMBIOS 3.0 (64-bit) Entry Point
    // is a DWORD field limited to 0xFFFFFFFF bytes. So the max size of 64-bit table should not exceed 0xFFFFFFFF bytes.
    //
    if ((Smbios30EntryPointStructure != NULL) &&
        (Smbios30EntryPointStructure->TableMaximumSize + StructureSize > SMBIOS_3_0_TABLE_MAX_LENGTH)) {
      DEBUG ((EFI_D_INFO, "SmbiosAdd: Total length exceeds max 64-bit table length with type = %d size = 0x%x\n", Record->Type, StructureSize));
    } else {
      DEBUG ((EFI_D_INFO, "SmbiosAdd: Smbios type %d with size 0x%x is added to 64-bit table\n", Record->Type, StructureSize));
      Smbios64BitTable = TRUE;
    }
  }

  if ((!Smbios32BitTable) && (!Smbios64BitTable)) {
    //
    // If both 32-bit and 64-bit table are not updated, quit
    //
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Enter into critical section
  //
  Status = EfiAcquireLockOrFail (&Private->DataLock);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  RecordSize  = sizeof (EFI_SMBIOS_RECORD_HEADER) + StructureSize;
  TotalSize   = sizeof (EFI_SMBIOS_ENTRY) + RecordSize;

  //
  // Allocate internal buffer
  //
  SmbiosEntry = AllocateZeroPool (TotalSize);
  if (SmbiosEntry == NULL) {
    EfiReleaseLock (&Private->DataLock);
    return EFI_OUT_OF_RESOURCES;
  }
  HandleEntry = AllocateZeroPool (sizeof(SMBIOS_HANDLE_ENTRY));
  if (HandleEntry == NULL) {
    EfiReleaseLock (&Private->DataLock);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Build Handle Entry and insert into linked list
  //
  HandleEntry->Signature     = SMBIOS_HANDLE_ENTRY_SIGNATURE;
  HandleEntry->SmbiosHandle  = *SmbiosHandle;
  InsertTailList(&Private->AllocatedHandleListHead, &HandleEntry->Link);

  InternalRecord  = (EFI_SMBIOS_RECORD_HEADER *) (SmbiosEntry + 1);
  Raw     = (VOID *) (InternalRecord + 1);

  //
  // Build internal record Header
  //
  InternalRecord->Version     = EFI_SMBIOS_RECORD_HEADER_VERSION;
  InternalRecord->HeaderSize  = (UINT16) sizeof (EFI_SMBIOS_RECORD_HEADER);
  InternalRecord->RecordSize  = RecordSize;
  InternalRecord->ProducerHandle = ProducerHandle;
  InternalRecord->NumberOfStrings = NumberOfStrings;
  //
  // Insert record into the internal linked list
  //
  SmbiosEntry->Signature    = EFI_SMBIOS_ENTRY_SIGNATURE;
  SmbiosEntry->RecordHeader = InternalRecord;
  SmbiosEntry->RecordSize   = TotalSize;
  SmbiosEntry->Smbios32BitTable = Smbios32BitTable;
  SmbiosEntry->Smbios64BitTable = Smbios64BitTable;
  InsertTailList (&Private->DataListHead, &SmbiosEntry->Link);

  CopyMem (Raw, Record, StructureSize);
  ((EFI_SMBIOS_TABLE_HEADER*)Raw)->Handle = *SmbiosHandle;

  //
  // Some UEFI drivers (such as network) need some information in SMBIOS table.
  // Here we create SMBIOS table and publish it in
  // configuration table, so other UEFI drivers can get SMBIOS table from
  // configuration table without depending on PI SMBIOS protocol.
  //
  SmbiosTableConstruction (Smbios32BitTable, Smbios64BitTable);

  //
  // Leave critical section
  //
  EfiReleaseLock (&Private->DataLock);
  return EFI_SUCCESS;
}

/**
  Update the string associated with an existing SMBIOS record.

  @param  This                  The EFI_SMBIOS_PROTOCOL instance.
  @param  SmbiosHandle          SMBIOS Handle of structure that will have its string updated.
  @param  StringNumber          The non-zero string number of the string to update
  @param  String                Update the StringNumber string with String.

  @retval EFI_SUCCESS           SmbiosHandle had its StringNumber String updated.
  @retval EFI_INVALID_PARAMETER SmbiosHandle does not exist.
  @retval EFI_UNSUPPORTED       String was not added because it is longer than the SMBIOS Table supports.
  @retval EFI_NOT_FOUND         The StringNumber.is not valid for this SMBIOS record.

**/
EFI_STATUS
EFIAPI
SmbiosUpdateString (
  IN CONST EFI_SMBIOS_PROTOCOL      *This,
  IN EFI_SMBIOS_HANDLE              *SmbiosHandle,
  IN UINTN                          *StringNumber,
  IN CHAR8                          *String
  )
{
  UINTN                     InputStrLen;
  UINTN                     TargetStrLen;
  UINTN                     StrIndex;
  UINTN                     TargetStrOffset;
  UINTN                     NewEntrySize;
  CHAR8                     *StrStart;
  VOID                      *Raw;
  LIST_ENTRY                *Link;
  LIST_ENTRY                *Head;
  EFI_STATUS                Status;
  SMBIOS_INSTANCE           *Private;
  EFI_SMBIOS_ENTRY          *SmbiosEntry;
  EFI_SMBIOS_ENTRY          *ResizedSmbiosEntry;
  EFI_SMBIOS_HANDLE         MaxSmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER   *Record;
  EFI_SMBIOS_RECORD_HEADER  *InternalRecord;

  //
  // Check args validity
  //
  GetMaxSmbiosHandle(This, &MaxSmbiosHandle);

  if (*SmbiosHandle > MaxSmbiosHandle) {
    return EFI_INVALID_PARAMETER;
  }

  if (String == NULL) {
    return EFI_ABORTED;
  }

  if (*StringNumber == 0) {
    return EFI_NOT_FOUND;
  }

  InputStrLen = AsciiStrLen(String);

  if (This->MajorVersion < 2 || (This->MajorVersion == 2 && This->MinorVersion < 7)) {
    if (InputStrLen > SMBIOS_STRING_MAX_LENGTH) {
      return EFI_UNSUPPORTED;
    }
  } else if (This->MajorVersion < 3) {
    //
    // Reference SMBIOS 2.7, chapter 6.1.3, it will have no limit on the length of each individual text string.
    // However, the length of the entire structure table (including all strings) must be reported
    // in the Structure Table Length field of the SMBIOS Structure Table Entry Point,
    // which is a WORD field limited to 65,535 bytes.
    //
    if (InputStrLen > SMBIOS_TABLE_MAX_LENGTH) {
      return EFI_UNSUPPORTED;
    }
  } else {
    if (InputStrLen > SMBIOS_3_0_TABLE_MAX_LENGTH) {
      //
      // SMBIOS 3.0 defines the Structure table maximum size as DWORD field limited to 0xFFFFFFFF bytes.
      // The input string length should not exceed 0xFFFFFFFF bytes.
      //
      return EFI_UNSUPPORTED;
    }
  }

  Private = SMBIOS_INSTANCE_FROM_THIS (This);
  //
  // Enter into critical section
  //
  Status = EfiAcquireLockOrFail (&Private->DataLock);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Head = &Private->DataListHead;
  for (Link = Head->ForwardLink; Link != Head; Link = Link->ForwardLink) {
    SmbiosEntry = SMBIOS_ENTRY_FROM_LINK(Link);
    Record = (EFI_SMBIOS_TABLE_HEADER*)(SmbiosEntry->RecordHeader + 1);

    if (Record->Handle == *SmbiosHandle) {
      //
      // Find out the specified SMBIOS record
      //
      if (*StringNumber > SmbiosEntry->RecordHeader->NumberOfStrings) {
        EfiReleaseLock (&Private->DataLock);
        return EFI_NOT_FOUND;
      }
      //
      // Point to unformed string section
      //
      StrStart = (CHAR8 *) Record + Record->Length;

      for (StrIndex = 1, TargetStrOffset = 0; StrIndex < *StringNumber; StrStart++, TargetStrOffset++) {
        //
        // A string ends in 00h
        //
        if (*StrStart == 0) {
          StrIndex++;
        }

        //
        // String section ends in double-null (0000h)
        //
        if (*StrStart == 0 && *(StrStart + 1) == 0) {
          EfiReleaseLock (&Private->DataLock);
          return EFI_NOT_FOUND;
        }
      }

      if (*StrStart == 0) {
        StrStart++;
        TargetStrOffset++;
      }

      //
      // Now we get the string target
      //
      TargetStrLen = AsciiStrLen(StrStart);
      if (InputStrLen == TargetStrLen) {
        AsciiStrCpyS(StrStart, TargetStrLen + 1, String);
        //
        // Some UEFI drivers (such as network) need some information in SMBIOS table.
        // Here we create SMBIOS table and publish it in
        // configuration table, so other UEFI drivers can get SMBIOS table from
        // configuration table without depending on PI SMBIOS protocol.
        //
        SmbiosTableConstruction (SmbiosEntry->Smbios32BitTable, SmbiosEntry->Smbios64BitTable);
        EfiReleaseLock (&Private->DataLock);
        return EFI_SUCCESS;
      }

      SmbiosEntry->Smbios32BitTable = FALSE;
      SmbiosEntry->Smbios64BitTable = FALSE;
      if ((This->MajorVersion < 0x3) ||
          ((This->MajorVersion >= 0x3) && ((PcdGet32 (PcdSmbiosEntryPointProvideMethod) & BIT0) == BIT0))) {
        //
        // 32-bit table is produced, check the valid length.
        //
        if ((EntryPointStructure != NULL) &&
            (EntryPointStructure->TableLength + InputStrLen - TargetStrLen > SMBIOS_TABLE_MAX_LENGTH)) {
          //
          // The length of the entire structure table (including all strings) must be reported
          // in the Structure Table Length field of the SMBIOS Structure Table Entry Point,
          // which is a WORD field limited to 65,535 bytes.
          //
          DEBUG ((EFI_D_INFO, "SmbiosUpdateString: Total length exceeds max 32-bit table length\n"));
        } else {
          DEBUG ((EFI_D_INFO, "SmbiosUpdateString: New smbios record add to 32-bit table\n"));
          SmbiosEntry->Smbios32BitTable = TRUE;
        }
      }

      if ((This->MajorVersion >= 0x3) && ((PcdGet32 (PcdSmbiosEntryPointProvideMethod) & BIT1) == BIT1)) {
        //
        // 64-bit table is produced, check the valid length.
        //
        if ((Smbios30EntryPointStructure != NULL) &&
            (Smbios30EntryPointStructure->TableMaximumSize + InputStrLen - TargetStrLen > SMBIOS_3_0_TABLE_MAX_LENGTH)) {
          DEBUG ((EFI_D_INFO, "SmbiosUpdateString: Total length exceeds max 64-bit table length\n"));
        } else {
          DEBUG ((EFI_D_INFO, "SmbiosUpdateString: New smbios record add to 64-bit table\n"));
          SmbiosEntry->Smbios64BitTable = TRUE;
        }
      }

      if ((!SmbiosEntry->Smbios32BitTable) && (!SmbiosEntry->Smbios64BitTable)) {
        EfiReleaseLock (&Private->DataLock);
        return EFI_UNSUPPORTED;
      }

      //
      // Original string buffer size is not exactly match input string length.
      // Re-allocate buffer is needed.
      //
      NewEntrySize = SmbiosEntry->RecordSize + InputStrLen - TargetStrLen;
      ResizedSmbiosEntry = AllocateZeroPool (NewEntrySize);

      if (ResizedSmbiosEntry == NULL) {
        EfiReleaseLock (&Private->DataLock);
        return EFI_OUT_OF_RESOURCES;
      }

      InternalRecord  = (EFI_SMBIOS_RECORD_HEADER *) (ResizedSmbiosEntry + 1);
      Raw     = (VOID *) (InternalRecord + 1);

      //
      // Build internal record Header
      //
      InternalRecord->Version     = EFI_SMBIOS_RECORD_HEADER_VERSION;
      InternalRecord->HeaderSize  = (UINT16) sizeof (EFI_SMBIOS_RECORD_HEADER);
      InternalRecord->RecordSize  = SmbiosEntry->RecordHeader->RecordSize + InputStrLen - TargetStrLen;
      InternalRecord->ProducerHandle = SmbiosEntry->RecordHeader->ProducerHandle;
      InternalRecord->NumberOfStrings = SmbiosEntry->RecordHeader->NumberOfStrings;

      //
      // Copy SMBIOS structure and optional strings.
      //
      CopyMem (Raw, SmbiosEntry->RecordHeader + 1, Record->Length + TargetStrOffset);
      CopyMem ((VOID*)((UINTN)Raw + Record->Length + TargetStrOffset), String, InputStrLen + 1);
      CopyMem ((CHAR8*)((UINTN)Raw + Record->Length + TargetStrOffset + InputStrLen + 1),
               (CHAR8*)Record + Record->Length + TargetStrOffset + TargetStrLen + 1,
               SmbiosEntry->RecordHeader->RecordSize - sizeof (EFI_SMBIOS_RECORD_HEADER) - Record->Length - TargetStrOffset - TargetStrLen - 1);

      //
      // Insert new record
      //
      ResizedSmbiosEntry->Signature    = EFI_SMBIOS_ENTRY_SIGNATURE;
      ResizedSmbiosEntry->RecordHeader = InternalRecord;
      ResizedSmbiosEntry->RecordSize   = NewEntrySize;
      ResizedSmbiosEntry->Smbios32BitTable = SmbiosEntry->Smbios32BitTable;
      ResizedSmbiosEntry->Smbios64BitTable = SmbiosEntry->Smbios64BitTable;
      InsertTailList (Link->ForwardLink, &ResizedSmbiosEntry->Link);

      //
      // Remove old record
      //
      RemoveEntryList(Link);
      FreePool(SmbiosEntry);
      //
      // Some UEFI drivers (such as network) need some information in SMBIOS table.
      // Here we create SMBIOS table and publish it in
      // configuration table, so other UEFI drivers can get SMBIOS table from
      // configuration table without depending on PI SMBIOS protocol.
      //
      SmbiosTableConstruction (ResizedSmbiosEntry->Smbios32BitTable, ResizedSmbiosEntry->Smbios64BitTable);
      EfiReleaseLock (&Private->DataLock);
      return EFI_SUCCESS;
    }
  }

  EfiReleaseLock (&Private->DataLock);
  return EFI_INVALID_PARAMETER;
}

/**
  Remove an SMBIOS record.

  @param  This                  The EFI_SMBIOS_PROTOCOL instance.
  @param  SmbiosHandle          The handle of the SMBIOS record to remove.

  @retval EFI_SUCCESS           SMBIOS record was removed.
  @retval EFI_INVALID_PARAMETER SmbiosHandle does not specify a valid SMBIOS record.

**/
EFI_STATUS
EFIAPI
SmbiosRemove (
  IN CONST EFI_SMBIOS_PROTOCOL   *This,
  IN EFI_SMBIOS_HANDLE           SmbiosHandle
  )
{
  LIST_ENTRY                 *Link;
  LIST_ENTRY                 *Head;
  EFI_STATUS                 Status;
  EFI_SMBIOS_HANDLE          MaxSmbiosHandle;
  SMBIOS_INSTANCE            *Private;
  EFI_SMBIOS_ENTRY           *SmbiosEntry;
  SMBIOS_HANDLE_ENTRY        *HandleEntry;
  EFI_SMBIOS_TABLE_HEADER    *Record;

  //
  // Check args validity
  //
  GetMaxSmbiosHandle(This, &MaxSmbiosHandle);

  if (SmbiosHandle > MaxSmbiosHandle) {
    return EFI_INVALID_PARAMETER;
  }

  Private = SMBIOS_INSTANCE_FROM_THIS (This);
  //
  // Enter into critical section
  //
  Status = EfiAcquireLockOrFail (&Private->DataLock);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Head = &Private->DataListHead;
  for (Link = Head->ForwardLink; Link != Head; Link = Link->ForwardLink) {
    SmbiosEntry = SMBIOS_ENTRY_FROM_LINK(Link);
    Record = (EFI_SMBIOS_TABLE_HEADER*)(SmbiosEntry->RecordHeader + 1);
    if (Record->Handle == SmbiosHandle) {
      //
      // Remove specified smobios record from DataList
      //
      RemoveEntryList(Link);
      //
      // Remove this handle from AllocatedHandleList
      //
      Head = &Private->AllocatedHandleListHead;
      for (Link = Head->ForwardLink; Link != Head; Link = Link->ForwardLink) {
        HandleEntry = SMBIOS_HANDLE_ENTRY_FROM_LINK(Link);
        if (HandleEntry->SmbiosHandle == SmbiosHandle) {
          RemoveEntryList(Link);
          FreePool(HandleEntry);
          break;
        }
      }
      //
      // Some UEFI drivers (such as network) need some information in SMBIOS table.
      // Here we create SMBIOS table and publish it in
      // configuration table, so other UEFI drivers can get SMBIOS table from
      // configuration table without depending on PI SMBIOS protocol.
      //
      if (SmbiosEntry->Smbios32BitTable) {
        DEBUG ((EFI_D_INFO, "SmbiosRemove: remove from 32-bit table\n"));
      }
      if (SmbiosEntry->Smbios64BitTable) {
        DEBUG ((EFI_D_INFO, "SmbiosRemove: remove from 64-bit table\n"));
      }
      //
      // Update the whole SMBIOS table again based on which table the removed SMBIOS record is in.
      //
      SmbiosTableConstruction (SmbiosEntry->Smbios32BitTable, SmbiosEntry->Smbios64BitTable);
      FreePool(SmbiosEntry);
      EfiReleaseLock (&Private->DataLock);
      return EFI_SUCCESS;
    }
  }

  //
  // Leave critical section
  //
  EfiReleaseLock (&Private->DataLock);
  return EFI_INVALID_PARAMETER;

}

/**
  Allow the caller to discover all or some of the SMBIOS records.

  @param  This                  The EFI_SMBIOS_PROTOCOL instance.
  @param  SmbiosHandle          On entry, points to the previous handle of the SMBIOS record. On exit, points to the
                                next SMBIOS record handle. If it is FFFEh on entry, then the first SMBIOS record
                                handle will be returned. If it returns FFFEh on exit, then there are no more SMBIOS records.
  @param  Type                  On entry it means return the next SMBIOS record of type Type. If a NULL is passed in
                                this functionally it ignored. Type is not modified by the GetNext() function.
  @param  Record                On exit, points to the SMBIOS Record consisting of the formatted area followed by
                                the unformatted area. The unformatted area optionally contains text strings.
  @param  ProducerHandle        On exit, points to the ProducerHandle registered by Add(). If no ProducerHandle was passed into Add() NULL is returned.
                                If a NULL pointer is passed in no data will be returned

  @retval EFI_SUCCESS           SMBIOS record information was successfully returned in Record.
  @retval EFI_NOT_FOUND         The SMBIOS record with SmbiosHandle was the last available record.

**/
EFI_STATUS
EFIAPI
SmbiosGetNext (
  IN CONST EFI_SMBIOS_PROTOCOL      *This,
  IN OUT EFI_SMBIOS_HANDLE          *SmbiosHandle,
  IN EFI_SMBIOS_TYPE                *Type,          OPTIONAL
  OUT EFI_SMBIOS_TABLE_HEADER       **Record,
  OUT EFI_HANDLE                    *ProducerHandle OPTIONAL
  )
{
  BOOLEAN                  StartPointFound;
  LIST_ENTRY               *Link;
  LIST_ENTRY               *Head;
  SMBIOS_INSTANCE          *Private;
  EFI_SMBIOS_ENTRY         *SmbiosEntry;
  EFI_SMBIOS_TABLE_HEADER  *SmbiosTableHeader;

  if (SmbiosHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  StartPointFound = FALSE;
  Private = SMBIOS_INSTANCE_FROM_THIS (This);
  Head = &Private->DataListHead;
  for (Link = Head->ForwardLink; Link != Head; Link = Link->ForwardLink) {
    SmbiosEntry = SMBIOS_ENTRY_FROM_LINK(Link);
    SmbiosTableHeader = (EFI_SMBIOS_TABLE_HEADER*)(SmbiosEntry->RecordHeader + 1);

    //
    // If SmbiosHandle is 0xFFFE, the first matched SMBIOS record handle will be returned
    //
    if (*SmbiosHandle == SMBIOS_HANDLE_PI_RESERVED) {
      if ((Type != NULL) && (*Type != SmbiosTableHeader->Type)) {
        continue;
      }

      *SmbiosHandle = SmbiosTableHeader->Handle;
      *Record =SmbiosTableHeader;
      if (ProducerHandle != NULL) {
        *ProducerHandle = SmbiosEntry->RecordHeader->ProducerHandle;
      }
      return EFI_SUCCESS;
    }

    //
    // Start this round search from the next SMBIOS handle
    //
    if (!StartPointFound && (*SmbiosHandle == SmbiosTableHeader->Handle)) {
      StartPointFound = TRUE;
      continue;
    }

    if (StartPointFound) {
      if ((Type != NULL) && (*Type != SmbiosTableHeader->Type)) {
        continue;
      }

      *SmbiosHandle = SmbiosTableHeader->Handle;
      *Record = SmbiosTableHeader;
      if (ProducerHandle != NULL) {
        *ProducerHandle = SmbiosEntry->RecordHeader->ProducerHandle;
      }

      return EFI_SUCCESS;
    }
  }

  *SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  return EFI_NOT_FOUND;

}

/**
  Allow the caller to discover all of the SMBIOS records.

  @param  This                  The EFI_SMBIOS_PROTOCOL instance.
  @param  CurrentSmbiosEntry    On exit, points to the SMBIOS entry on the list which includes the returned SMBIOS record information.
                                If *CurrentSmbiosEntry is NULL on entry, then the first SMBIOS entry on the list will be returned.
  @param  Record                On exit, points to the SMBIOS Record consisting of the formatted area followed by
                                the unformatted area. The unformatted area optionally contains text strings.

  @retval EFI_SUCCESS           SMBIOS record information was successfully returned in Record.
                                *CurrentSmbiosEntry points to the SMBIOS entry which includes the returned SMBIOS record information.
  @retval EFI_NOT_FOUND         There is no more SMBIOS entry.

**/
EFI_STATUS
EFIAPI
GetNextSmbiosRecord (
  IN CONST EFI_SMBIOS_PROTOCOL         *This,
  IN OUT EFI_SMBIOS_ENTRY              **CurrentSmbiosEntry,
  OUT EFI_SMBIOS_TABLE_HEADER          **Record
  )
{
  LIST_ENTRY               *Link;
  LIST_ENTRY               *Head;
  SMBIOS_INSTANCE          *Private;
  EFI_SMBIOS_ENTRY         *SmbiosEntry;
  EFI_SMBIOS_TABLE_HEADER  *SmbiosTableHeader;

  Private = SMBIOS_INSTANCE_FROM_THIS (This);
  if (*CurrentSmbiosEntry == NULL) {
    //
    // Get the beginning of SMBIOS entry.
    //
    Head = &Private->DataListHead;
  } else {
    //
    // Get previous SMBIOS entry and make it as start point.
    //
    Head = &(*CurrentSmbiosEntry)->Link;
  }

  Link  = Head->ForwardLink;

  if (Link == &Private->DataListHead) {
    //
    // If no more SMBIOS entry in the list, return not found.
    //
    return EFI_NOT_FOUND;
  }

  SmbiosEntry = SMBIOS_ENTRY_FROM_LINK(Link);
  SmbiosTableHeader = (EFI_SMBIOS_TABLE_HEADER*)(SmbiosEntry->RecordHeader + 1);
  *Record = SmbiosTableHeader;
  *CurrentSmbiosEntry = SmbiosEntry;
  return EFI_SUCCESS;
}

/**
  Assembles SMBIOS table from the SMBIOS protocol. Produce Table
  Entry Point and return the pointer to it.

  @param  TableEntryPointStructure   On exit, points to the SMBIOS entrypoint structure.

  @retval EFI_SUCCESS                Structure created sucessfully.
  @retval EFI_OUT_OF_RESOURCES       No enough memory.

**/
EFI_STATUS
EFIAPI
SmbiosCreateTable (
  OUT VOID    **TableEntryPointStructure
  )
{
  UINT8                           *BufferPointer;
  UINTN                           RecordSize;
  UINTN                           NumOfStr;
  EFI_STATUS                      Status;
  EFI_SMBIOS_HANDLE               SmbiosHandle;
  EFI_SMBIOS_PROTOCOL             *SmbiosProtocol;
  EFI_PHYSICAL_ADDRESS            PhysicalAddress;
  EFI_SMBIOS_TABLE_HEADER         *SmbiosRecord;
  EFI_SMBIOS_TABLE_END_STRUCTURE  EndStructure;
  EFI_SMBIOS_ENTRY                *CurrentSmbiosEntry;

  Status            = EFI_SUCCESS;
  BufferPointer     = NULL;

  if (EntryPointStructure == NULL) {
    //
    // Initialize the EntryPointStructure with initial values.
    // It should be done only once.
    // Allocate memory (below 4GB).
    //
    DEBUG ((EFI_D_INFO, "SmbiosCreateTable: Initialize 32-bit entry point structure\n"));
    EntryPointStructureData.MajorVersion  = mPrivateData.Smbios.MajorVersion;
    EntryPointStructureData.MinorVersion  = mPrivateData.Smbios.MinorVersion;
    EntryPointStructureData.SmbiosBcdRevision = (UINT8) ((PcdGet16 (PcdSmbiosVersion) >> 4) & 0xf0) | (UINT8) (PcdGet16 (PcdSmbiosVersion) & 0x0f);
    PhysicalAddress = 0xffffffff;
    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiRuntimeServicesData,
                    EFI_SIZE_TO_PAGES (sizeof (SMBIOS_TABLE_ENTRY_POINT)),
                    &PhysicalAddress
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "SmbiosCreateTable () could not allocate EntryPointStructure < 4GB\n"));
      Status = gBS->AllocatePages (
                      AllocateAnyPages,
                      EfiRuntimeServicesData,
                      EFI_SIZE_TO_PAGES (sizeof (SMBIOS_TABLE_ENTRY_POINT)),
                      &PhysicalAddress
                      );
     if (EFI_ERROR (Status)) {
        return EFI_OUT_OF_RESOURCES;
      }
    }

    EntryPointStructure = (SMBIOS_TABLE_ENTRY_POINT *) (UINTN) PhysicalAddress;

    CopyMem (
      EntryPointStructure,
      &EntryPointStructureData,
      sizeof (SMBIOS_TABLE_ENTRY_POINT)
      );
  }

  //
  // Get Smbios protocol to traverse SMBIOS records.
  //
  SmbiosProtocol = &mPrivateData.Smbios;

  //
  // Make some statistics about all the structures
  //
  EntryPointStructure->NumberOfSmbiosStructures = 0;
  EntryPointStructure->TableLength              = 0;
  EntryPointStructure->MaxStructureSize         = 0;

  //
  // Calculate EPS Table Length
  //
  CurrentSmbiosEntry = NULL;
  do {
    Status = GetNextSmbiosRecord (SmbiosProtocol, &CurrentSmbiosEntry, &SmbiosRecord);

    if ((Status == EFI_SUCCESS) && (CurrentSmbiosEntry->Smbios32BitTable)) {
      GetSmbiosStructureSize(SmbiosProtocol, SmbiosRecord, &RecordSize, &NumOfStr);
      //
      // Record NumberOfSmbiosStructures, TableLength and MaxStructureSize
      //
      EntryPointStructure->NumberOfSmbiosStructures++;
      EntryPointStructure->TableLength = (UINT16) (EntryPointStructure->TableLength + RecordSize);
      if (RecordSize > EntryPointStructure->MaxStructureSize) {
        EntryPointStructure->MaxStructureSize = (UINT16) RecordSize;
      }
    }
  } while (!EFI_ERROR(Status));

  //
  // Create End-Of-Table structure
  //
  GetMaxSmbiosHandle(SmbiosProtocol, &SmbiosHandle);
  EndStructure.Header.Type = SMBIOS_TYPE_END_OF_TABLE;
  EndStructure.Header.Length = (UINT8) sizeof (EFI_SMBIOS_TABLE_HEADER);
  EndStructure.Header.Handle = SmbiosHandle;
  EndStructure.Tailing[0] = 0;
  EndStructure.Tailing[1] = 0;
  EntryPointStructure->NumberOfSmbiosStructures++;
  EntryPointStructure->TableLength = (UINT16) (EntryPointStructure->TableLength + sizeof (EndStructure));
  if (sizeof (EndStructure) > EntryPointStructure->MaxStructureSize) {
    EntryPointStructure->MaxStructureSize = (UINT16) sizeof (EndStructure);
  }

  if (EFI_SIZE_TO_PAGES ((UINT32) EntryPointStructure->TableLength) > mPreAllocatedPages) {
    //
    // If new SMBIOS table size exceeds the previous allocated page,
    // it is time to re-allocate memory (below 4GB).
    //
    DEBUG ((EFI_D_INFO, "%a() re-allocate SMBIOS 32-bit table\n",
      __FUNCTION__));
    if (EntryPointStructure->TableAddress != 0) {
      //
      // Free the previous allocated page
      //
      FreePages (
            (VOID*)(UINTN)EntryPointStructure->TableAddress,
            mPreAllocatedPages
            );
      EntryPointStructure->TableAddress = 0;
      mPreAllocatedPages = 0;
    }

    PhysicalAddress = 0xffffffff;
    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiRuntimeServicesData,
                    EFI_SIZE_TO_PAGES (EntryPointStructure->TableLength),
                    &PhysicalAddress
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "SmbiosCreateTable() could not allocate SMBIOS table < 4GB\n"));
      EntryPointStructure->TableAddress = 0;
      return EFI_OUT_OF_RESOURCES;
    } else {
      EntryPointStructure->TableAddress = (UINT32) PhysicalAddress;
      mPreAllocatedPages = EFI_SIZE_TO_PAGES (EntryPointStructure->TableLength);
    }
  }

  //
  // Assemble the tables
  //
  ASSERT (EntryPointStructure->TableAddress != 0);
  BufferPointer = (UINT8 *) (UINTN) EntryPointStructure->TableAddress;
  CurrentSmbiosEntry = NULL;
  do {
    Status = GetNextSmbiosRecord (SmbiosProtocol, &CurrentSmbiosEntry, &SmbiosRecord);

    if ((Status == EFI_SUCCESS) && (CurrentSmbiosEntry->Smbios32BitTable)) {
      GetSmbiosStructureSize(SmbiosProtocol, SmbiosRecord, &RecordSize, &NumOfStr);
      CopyMem (BufferPointer, SmbiosRecord, RecordSize);
      BufferPointer = BufferPointer + RecordSize;
    }
  } while (!EFI_ERROR(Status));

  //
  // Assemble End-Of-Table structure
  //
  CopyMem (BufferPointer, &EndStructure, sizeof (EndStructure));

  //
  // Fixup checksums in the Entry Point Structure
  //
  EntryPointStructure->IntermediateChecksum = 0;
  EntryPointStructure->EntryPointStructureChecksum = 0;

  EntryPointStructure->IntermediateChecksum =
    CalculateCheckSum8 ((UINT8 *) EntryPointStructure + 0x10, EntryPointStructure->EntryPointLength - 0x10);
  EntryPointStructure->EntryPointStructureChecksum =
    CalculateCheckSum8 ((UINT8 *) EntryPointStructure, EntryPointStructure->EntryPointLength);

  //
  // Returns the pointer
  //
  *TableEntryPointStructure = EntryPointStructure;

  return EFI_SUCCESS;
}

/**
  Assembles SMBIOS 64-bit table from the SMBIOS protocol. Produce Table
  Entry Point and return the pointer to it.

  @param  TableEntryPointStructure   On exit, points to the SMBIOS entrypoint structure.

  @retval EFI_SUCCESS                Structure created sucessfully.
  @retval EFI_OUT_OF_RESOURCES       No enough memory.

**/
EFI_STATUS
EFIAPI
SmbiosCreate64BitTable (
  OUT VOID    **TableEntryPointStructure
  )
{
  UINT8                           *BufferPointer;
  UINTN                           RecordSize;
  UINTN                           NumOfStr;
  EFI_STATUS                      Status;
  EFI_SMBIOS_HANDLE               SmbiosHandle;
  EFI_SMBIOS_PROTOCOL             *SmbiosProtocol;
  EFI_PHYSICAL_ADDRESS            PhysicalAddress;
  EFI_SMBIOS_TABLE_HEADER         *SmbiosRecord;
  EFI_SMBIOS_TABLE_END_STRUCTURE  EndStructure;
  EFI_SMBIOS_ENTRY                *CurrentSmbiosEntry;

  Status            = EFI_SUCCESS;
  BufferPointer     = NULL;

  if (Smbios30EntryPointStructure == NULL) {
    //
    // Initialize the Smbios30EntryPointStructure with initial values.
    // It should be done only once.
    // Allocate memory at any address.
    //
    DEBUG ((EFI_D_INFO, "SmbiosCreateTable: Initialize 64-bit entry point structure\n"));
    Smbios30EntryPointStructureData.MajorVersion  = mPrivateData.Smbios.MajorVersion;
    Smbios30EntryPointStructureData.MinorVersion  = mPrivateData.Smbios.MinorVersion;
    Smbios30EntryPointStructureData.DocRev        = PcdGet8 (PcdSmbiosDocRev);
    Status = gBS->AllocatePages (
                    AllocateAnyPages,
                    EfiRuntimeServicesData,
                    EFI_SIZE_TO_PAGES (sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT)),
                    &PhysicalAddress
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "SmbiosCreate64BitTable() could not allocate Smbios30EntryPointStructure\n"));
      return EFI_OUT_OF_RESOURCES;
    }

    Smbios30EntryPointStructure = (SMBIOS_TABLE_3_0_ENTRY_POINT *) (UINTN) PhysicalAddress;

    CopyMem (
      Smbios30EntryPointStructure,
      &Smbios30EntryPointStructureData,
      sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT)
      );
  }

  //
  // Get Smbios protocol to traverse SMBIOS records.
  //
  SmbiosProtocol = &mPrivateData.Smbios;
  Smbios30EntryPointStructure->TableMaximumSize = 0;

  //
  // Calculate EPS Table Length
  //
  CurrentSmbiosEntry = NULL;
  do {
    Status = GetNextSmbiosRecord (SmbiosProtocol, &CurrentSmbiosEntry, &SmbiosRecord);

    if ((Status == EFI_SUCCESS) && (CurrentSmbiosEntry->Smbios64BitTable)) {
      GetSmbiosStructureSize(SmbiosProtocol, SmbiosRecord, &RecordSize, &NumOfStr);
      //
      // Record TableMaximumSize
      //
      Smbios30EntryPointStructure->TableMaximumSize = (UINT32) (Smbios30EntryPointStructure->TableMaximumSize + RecordSize);
    }
  } while (!EFI_ERROR(Status));

  //
  // Create End-Of-Table structure
  //
  GetMaxSmbiosHandle(SmbiosProtocol, &SmbiosHandle);
  EndStructure.Header.Type = SMBIOS_TYPE_END_OF_TABLE;
  EndStructure.Header.Length = (UINT8) sizeof (EFI_SMBIOS_TABLE_HEADER);
  EndStructure.Header.Handle = SmbiosHandle;
  EndStructure.Tailing[0] = 0;
  EndStructure.Tailing[1] = 0;
  Smbios30EntryPointStructure->TableMaximumSize = (UINT32) (Smbios30EntryPointStructure->TableMaximumSize + sizeof (EndStructure));

  if (EFI_SIZE_TO_PAGES (Smbios30EntryPointStructure->TableMaximumSize) > mPre64BitAllocatedPages) {
    //
    // If new SMBIOS table size exceeds the previous allocated page,
    // it is time to re-allocate memory at anywhere.
    //
    DEBUG ((EFI_D_INFO, "%a() re-allocate SMBIOS 64-bit table\n",
      __FUNCTION__));
    if (Smbios30EntryPointStructure->TableAddress != 0) {
      //
      // Free the previous allocated page
      //
      FreePages (
            (VOID*)(UINTN)Smbios30EntryPointStructure->TableAddress,
            mPre64BitAllocatedPages
            );
      Smbios30EntryPointStructure->TableAddress = 0;
      mPre64BitAllocatedPages = 0;
    }

    Status = gBS->AllocatePages (
                    AllocateAnyPages,
                    EfiRuntimeServicesData,
                    EFI_SIZE_TO_PAGES (Smbios30EntryPointStructure->TableMaximumSize),
                    &PhysicalAddress
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "SmbiosCreateTable() could not allocate SMBIOS 64-bit table\n"));
      Smbios30EntryPointStructure->TableAddress = 0;
      return EFI_OUT_OF_RESOURCES;
    } else {
      Smbios30EntryPointStructure->TableAddress = PhysicalAddress;
      mPre64BitAllocatedPages = EFI_SIZE_TO_PAGES (Smbios30EntryPointStructure->TableMaximumSize);
    }
  }

  //
  // Assemble the tables
  //
  ASSERT (Smbios30EntryPointStructure->TableAddress != 0);
  BufferPointer = (UINT8 *) (UINTN) Smbios30EntryPointStructure->TableAddress;
  CurrentSmbiosEntry = NULL;
  do {
    Status = GetNextSmbiosRecord (SmbiosProtocol, &CurrentSmbiosEntry, &SmbiosRecord);

    if ((Status == EFI_SUCCESS) && (CurrentSmbiosEntry->Smbios64BitTable)) {
      //
      // This record can be added to 64-bit table
      //
      GetSmbiosStructureSize(SmbiosProtocol, SmbiosRecord, &RecordSize, &NumOfStr);
      CopyMem (BufferPointer, SmbiosRecord, RecordSize);
      BufferPointer = BufferPointer + RecordSize;
    }
  } while (!EFI_ERROR(Status));

  //
  // Assemble End-Of-Table structure
  //
  CopyMem (BufferPointer, &EndStructure, sizeof (EndStructure));

  //
  // Fixup checksums in the Entry Point Structure
  //
  Smbios30EntryPointStructure->EntryPointStructureChecksum = 0;
  Smbios30EntryPointStructure->EntryPointStructureChecksum =
    CalculateCheckSum8 ((UINT8 *) Smbios30EntryPointStructure, Smbios30EntryPointStructure->EntryPointLength);

  //
  // Returns the pointer
  //
  *TableEntryPointStructure = Smbios30EntryPointStructure;

  return EFI_SUCCESS;
}

/**
  Create Smbios Table and installs the Smbios Table to the System Table.

  @param  Smbios32BitTable    The flag to update 32-bit table.
  @param  Smbios64BitTable    The flag to update 64-bit table.

**/
VOID
EFIAPI
SmbiosTableConstruction (
  BOOLEAN     Smbios32BitTable,
  BOOLEAN     Smbios64BitTable
  )
{
  UINT8       *Eps;
  UINT8       *Eps64Bit;
  EFI_STATUS  Status;

  if (Smbios32BitTable) {
    Status = SmbiosCreateTable ((VOID **) &Eps);
    if (!EFI_ERROR (Status)) {
      gBS->InstallConfigurationTable (&gEfiSmbiosTableGuid, Eps);
    }
  }

  if (Smbios64BitTable) {
    Status = SmbiosCreate64BitTable ((VOID **) &Eps64Bit);
    if (!EFI_ERROR (Status)) {
      gBS->InstallConfigurationTable (&gEfiSmbios3TableGuid, Eps64Bit);
    }
  }
}

/**

  Driver to produce Smbios protocol and pre-allocate 1 page for the final SMBIOS table.

  @param ImageHandle     Module's image handle
  @param SystemTable     Pointer of EFI_SYSTEM_TABLE

  @retval EFI_SUCCESS    Smbios protocol installed
  @retval Other          No protocol installed, unload driver.

**/
EFI_STATUS
EFIAPI
SmbiosDriverEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS            Status;

  mPrivateData.Signature                = SMBIOS_INSTANCE_SIGNATURE;
  mPrivateData.Smbios.Add               = SmbiosAdd;
  mPrivateData.Smbios.UpdateString      = SmbiosUpdateString;
  mPrivateData.Smbios.Remove            = SmbiosRemove;
  mPrivateData.Smbios.GetNext           = SmbiosGetNext;
  mPrivateData.Smbios.MajorVersion      = (UINT8) (PcdGet16 (PcdSmbiosVersion) >> 8);
  mPrivateData.Smbios.MinorVersion      = (UINT8) (PcdGet16 (PcdSmbiosVersion) & 0x00ff);

  InitializeListHead (&mPrivateData.DataListHead);
  InitializeListHead (&mPrivateData.AllocatedHandleListHead);
  EfiInitializeLock (&mPrivateData.DataLock, TPL_NOTIFY);

  //
  // Make a new handle and install the protocol
  //
  mPrivateData.Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &mPrivateData.Handle,
                  &gEfiSmbiosProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPrivateData.Smbios
                  );

  return Status;
}
