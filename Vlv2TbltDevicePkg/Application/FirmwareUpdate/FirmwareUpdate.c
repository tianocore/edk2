/** @file

Copyright (c) 2007  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

**/

#include "FirmwareUpdate.h"

EFI_HII_HANDLE  HiiHandle;

//
// MinnowMax Flash Layout
//
//Start (hex)	End (hex)	Length (hex)	Area Name
//-----------	---------	------------	---------
//00000000	007FFFFF	00800000	Flash Image
//
//00000000	00000FFF	00001000	Descriptor Region
//00001000	004FFFFF	004FF000	TXE Region
//00500000	007FFFFF	00300000	BIOS Region
//
FV_REGION_INFO mRegionInfo[] = {
  {FixedPcdGet32 (PcdFlashDescriptorBase), FixedPcdGet32 (PcdFlashDescriptorSize), TRUE},
  {FixedPcdGet32 (PcdTxeRomBase), FixedPcdGet32 (PcdTxeRomSize), TRUE},
  {FixedPcdGet32 (PcdBiosRomBase), FixedPcdGet32 (PcdBiosRomSize), TRUE}
};

UINTN mRegionInfoCount = sizeof (mRegionInfo) / sizeof (mRegionInfo[0]);

FV_INPUT_DATA mInputData = {0};

EFI_SPI_PROTOCOL  *mSpiProtocol;

EFI_STATUS
GetRegionIndex (
  IN  EFI_PHYSICAL_ADDRESS  Address,
  OUT UINTN                 *RegionIndex
  )
{
  UINTN Index;

  for (Index = 0; Index < mRegionInfoCount; Index++) {
    if (Address >= mRegionInfo[Index].Base &&
        Address < (mRegionInfo[Index].Base + mRegionInfo[Index].Size)
        ) {
      break;
    }
  }

  *RegionIndex = Index;
  if (Index >= mRegionInfoCount) {
    return EFI_NOT_FOUND;
  }
  return EFI_SUCCESS;
}

BOOLEAN
UpdateBlock (
  IN  EFI_PHYSICAL_ADDRESS  Address
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  if (mInputData.FullFlashUpdate) {
    return TRUE;
  }

  Status = GetRegionIndex (Address, &Index);
  if ((!EFI_ERROR(Status)) && mRegionInfo[Index].Update) {
    return TRUE;
  }

  return FALSE;
}

EFI_STATUS
MarkRegionState (
  IN  EFI_PHYSICAL_ADDRESS  Address,
  IN  BOOLEAN               Update
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  Status = GetRegionIndex (Address, &Index);
  if (!EFI_ERROR(Status)) {
    mRegionInfo[Index].Update = Update;
  }

  return Status;
}

UINTN
InternalPrintToken (
  IN  CONST CHAR16                     *Format,
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *Console,
  IN  VA_LIST                          Marker
  )
{
  EFI_STATUS  Status;
  UINTN       Return;
  CHAR16      *Buffer;
  UINTN       BufferSize;

  ASSERT (Format != NULL);
  ASSERT (((UINTN) Format & BIT0) == 0);
  ASSERT (Console != NULL);

  BufferSize = (PcdGet32 (PcdUefiLibMaxPrintBufferSize) + 1) * sizeof (CHAR16);

  Buffer = (CHAR16 *) AllocatePool(BufferSize);
  ASSERT (Buffer != NULL);

  Return = UnicodeVSPrint (Buffer, BufferSize, Format, Marker);

  if (Console != NULL && Return > 0) {
    //
    // To be extra safe make sure Console has been initialized.
    //
    Status = Console->OutputString (Console, Buffer);
    if (EFI_ERROR (Status)) {
      Return = 0;
    }
  }

  FreePool (Buffer);

  return Return;
}

UINTN
EFIAPI
PrintToken (
  IN UINT16             Token,
  IN EFI_HII_HANDLE     Handle,
  ...
  )
{
  VA_LIST Marker;
  UINTN   Return;
  CHAR16  *Format;

  VA_START (Marker, Handle);

  Format = HiiGetString (Handle, Token, NULL);
  ASSERT (Format != NULL);

  Return = InternalPrintToken (Format, gST->ConOut, Marker);

  FreePool (Format);

  VA_END (Marker);

  return Return;
}

EFI_STATUS
ParseCommandLine (
  IN  UINTN   Argc,
  IN  CHAR16  **Argv
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  //
  // Check to make sure that the command line has enough arguments for minimal
  // operation.  The minimum is just the file name.
  //
  if (Argc < 2 || Argc > 4) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Loop through command line arguments.
  //
  for (Index = 1; Index < Argc; Index++) {
    //
    // Make sure the string is valid.
    //
    if (StrLen (Argv[Index]) == 0) {;
      PrintToken (STRING_TOKEN (STR_FWUPDATE_ZEROLENGTH_ARG), HiiHandle);
      return EFI_INVALID_PARAMETER;
    }

    //
    // Check to see if this is an option or the file name.
    //
    if ((Argv[Index])[0] == L'-' || (Argv[Index])[0] == L'/') {
      //
      // Parse the arguments.
      //
      if ((StrCmp (Argv[Index], L"-h") == 0) ||
          (StrCmp (Argv[Index], L"--help") == 0) ||
          (StrCmp (Argv[Index], L"/?") == 0) ||
          (StrCmp (Argv[Index], L"/h") == 0)) {
        //
        // Print Help Information.
        //
        return EFI_INVALID_PARAMETER;
      } else if (StrCmp (Argv[Index], L"-m") == 0) {
        //
        // Parse the MAC address here.
        //
        Status = ConvertMac(Argv[Index+1]);
        if (EFI_ERROR(Status)) {
          PrintToken (STRING_TOKEN (STR_FWUPDATE_INVAILD_MAC), HiiHandle);
          return Status;
        }

        //
        // Save the MAC address to mInputData.MacValue.
        //
        mInputData.UpdateMac= TRUE;
        Index++;
        } else {
        //
        // Invalid option was provided.
        //
        return EFI_INVALID_PARAMETER;
      }
    }
    if ((Index == Argc - 1) && (StrCmp (Argv[Index - 1], L"-m") != 0)) {
      //
      // The only parameter that is not an option is the firmware image.  Check
      // to make sure that the file exists.
      //
      Status = ShellIsFile (Argv[Index]);
      if (EFI_ERROR (Status)) {
        PrintToken (STRING_TOKEN (STR_FWUPDATE_FILE_NOT_FOUND_ERROR), HiiHandle, Argv[Index]);
        return EFI_INVALID_PARAMETER;
      }
      if (StrLen (Argv[Index]) > INPUT_STRING_LEN) {
        PrintToken (STRING_TOKEN (STR_FWUPDATE_PATH_ERROR), HiiHandle, Argv[Index]);
        return EFI_INVALID_PARAMETER;
      }
      StrCpy (mInputData.FileName, Argv[Index]);
      mInputData.UpdateFromFile = TRUE;
    }
  }

  return EFI_SUCCESS;
}

INTN
EFIAPI
ShellAppMain (
  IN UINTN Argc,
  IN CHAR16 **Argv
  )
{
  EFI_STATUS            Status;
  UINTN                 Index;
  UINT32                FileSize;
  UINT32                BufferSize;
  UINT8                 *FileBuffer;
  UINT8                 *Buffer;
  EFI_PHYSICAL_ADDRESS  Address;
  UINTN                 CountOfBlocks;
  EFI_TPL               OldTpl;
  BOOLEAN               ResetRequired;
  BOOLEAN               FlashError;

  Index             = 0;
  FileSize          = 0;
  BufferSize        = 0;
  FileBuffer        = NULL;
  Buffer            = NULL;
  Address           = 0;
  CountOfBlocks     = 0;
  ResetRequired     = FALSE;
  FlashError        = FALSE;

  Status = EFI_SUCCESS;

  mInputData.FullFlashUpdate = TRUE;

  //
  // Publish our HII data.
  //
  HiiHandle = HiiAddPackages (
                &gEfiCallerIdGuid,
                NULL,
                FirmwareUpdateStrings,
                NULL
                );
  if (HiiHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Locate the SPI protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiSpiProtocolGuid,
                  NULL,
                  (VOID **)&mSpiProtocol
                  );
  if (EFI_ERROR (Status)) {
    PrintToken (STRING_TOKEN (STR_SPI_NOT_FOUND), HiiHandle);
    return EFI_DEVICE_ERROR;
  }

  //
  // Parse the command line.
  //
  Status = ParseCommandLine (Argc, Argv);
  if (EFI_ERROR (Status)) {
    PrintHelpInfo ();
    Status = EFI_SUCCESS;
    goto Done;
  }

  //
  // Display sign-on information.
  //
  PrintToken (STRING_TOKEN (STR_FWUPDATE_FIRMWARE_VOL_UPDATE), HiiHandle);
  PrintToken (STRING_TOKEN (STR_FWUPDATE_VERSION), HiiHandle);
  PrintToken (STRING_TOKEN (STR_FWUPDATE_COPYRIGHT), HiiHandle);

  //
  // Test to see if the firmware needs to be updated.
  //
  if (mInputData.UpdateFromFile) {
    //
    // Get the file to use in the update.
    //
    PrintToken (STRING_TOKEN (STR_FWUPDATE_READ_FILE), HiiHandle, mInputData.FileName);
    Status = ReadFileData (mInputData.FileName, &FileBuffer, &FileSize);
    if (EFI_ERROR (Status)) {
      PrintToken (STRING_TOKEN (STR_FWUPDATE_READ_FILE_ERROR), HiiHandle, mInputData.FileName);
      goto Done;
    }

    //
    // Check that the file and flash sizes match.
    //
    if (FileSize != PcdGet32 (PcdFlashChipSize)) {
      PrintToken (STRING_TOKEN (STR_FWUPDATE_SIZE), HiiHandle);
      Status = EFI_UNSUPPORTED;
      goto Done;
    }

    //
    // Display flash update information.
    //
    PrintToken (STRING_TOKEN (STR_FWUPDATE_UPDATING_FIRMWARE), HiiHandle);

    //
    // Update it.
    //
    Buffer        = FileBuffer;
    BufferSize    = FileSize;
    Address       = PcdGet32 (PcdFlashChipBase);
    CountOfBlocks = (UINTN) (BufferSize / BLOCK_SIZE);

    //
    // Raise TPL to TPL_NOTIFY to block any event handler,
    // while still allowing RaiseTPL(TPL_NOTIFY) within
    // output driver during Print().
    //
    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    for (Index = 0; Index < CountOfBlocks; Index++) {
      //
      // Handle block based on address and contents.
      //
      if (!UpdateBlock (Address)) {
        DEBUG((EFI_D_INFO, "Skipping block at 0x%lx\n", Address));
      } else if (!EFI_ERROR (InternalCompareBlock (Address, Buffer))) {
        DEBUG((EFI_D_INFO, "Skipping block at 0x%lx (already programmed)\n", Address));
      } else {
        //
        // Display a dot for each block being updated.
        //
        Print (L".");

        //
        // Flag that the flash image will be changed and the system must be rebooted
        // to use the change.
        //
        ResetRequired = TRUE;

        //
        // Make updating process uninterruptable,
        // so that the flash memory area is not accessed by other entities
        // which may interfere with the updating process.
        //
        Status  = InternalEraseBlock (Address);
        ASSERT_EFI_ERROR(Status);
        if (EFI_ERROR (Status)) {
          gBS->RestoreTPL (OldTpl);
          FlashError = TRUE;
          goto Done;
        }
        Status = InternalWriteBlock (
                  Address,
                  Buffer,
                  (BufferSize > BLOCK_SIZE ? BLOCK_SIZE : BufferSize)
                  );
        if (EFI_ERROR (Status)) {
          gBS->RestoreTPL (OldTpl);
          FlashError = TRUE;
          goto Done;
        }
      }

      //
      // Move to next block to update.
      //
      Address += BLOCK_SIZE;
      Buffer += BLOCK_SIZE;
      if (BufferSize > BLOCK_SIZE) {
        BufferSize -= BLOCK_SIZE;
      } else {
        BufferSize = 0;
      }
    }
    gBS->RestoreTPL (OldTpl);

    //
    // Print result of update.
    //
    if (!FlashError) {
      if (ResetRequired) {
        Print (L"\n");
        PrintToken (STRING_TOKEN (STR_FWUPDATE_UPDATE_SUCCESS), HiiHandle);
      } else {
        PrintToken (STRING_TOKEN (STR_FWUPDATE_NO_RESET), HiiHandle);
      }
    } else {
      goto Done;
    }
  }

  //
  // All flash updates are done so see if the system needs to be reset.
  //
  if (ResetRequired && !FlashError) {
    //
    // Update successful.
    //
    for (Index = 5; Index > 0; Index--) {
      PrintToken (STRING_TOKEN (STR_FWUPDATE_SHUTDOWN), HiiHandle, Index);
      gBS->Stall (1000000);
    }

    gRT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    PrintToken (STRING_TOKEN (STR_FWUPDATE_MANUAL_RESET), HiiHandle);
    CpuDeadLoop ();
  }

Done:
  //
  // Print flash update failure message if error detected.
  //
  if (FlashError) {
    PrintToken (STRING_TOKEN (STR_FWUPDATE_UPDATE_FAILED), HiiHandle, Index);
  }

  //
  // Do cleanup.
  //
  if (HiiHandle != NULL) {
    HiiRemovePackages (HiiHandle);
  }
  if (FileBuffer) {
    gBS->FreePool (FileBuffer);
  }

  return Status;
}

STATIC
EFI_STATUS
InternalEraseBlock (
  IN  EFI_PHYSICAL_ADDRESS BaseAddress
  )
/*++

Routine Description:

  Erase the whole block.

Arguments:

  BaseAddress  - Base address of the block to be erased.

Returns:

  EFI_SUCCESS - The command completed successfully.
  Other       - Device error or wirte-locked, operation failed.

--*/
{
  EFI_STATUS                              Status;
  UINTN                                   NumBytes;

  NumBytes = BLOCK_SIZE;

  Status = SpiFlashBlockErase ((UINTN) BaseAddress, &NumBytes);

  return Status;
}

#if 0
STATIC
EFI_STATUS
InternalReadBlock (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  OUT VOID                  *ReadBuffer
  )
{
  EFI_STATUS    Status;
  UINT32        BlockSize;

  BlockSize = BLOCK_SIZE;

  Status = SpiFlashRead ((UINTN) BaseAddress, &BlockSize, ReadBuffer);

  return Status;
}
#endif

STATIC
EFI_STATUS
InternalCompareBlock (
  IN  EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN  UINT8                       *Buffer
  )
{
  EFI_STATUS                              Status;
  VOID                                    *CompareBuffer;
  UINT32                                  NumBytes;
  INTN                                    CompareResult;

  NumBytes = BLOCK_SIZE;
  CompareBuffer = AllocatePool (NumBytes);
  if (CompareBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  Status = SpiFlashRead ((UINTN) BaseAddress, &NumBytes, CompareBuffer);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  CompareResult = CompareMem (CompareBuffer, Buffer, BLOCK_SIZE);
  if (CompareResult != 0) {
    Status = EFI_VOLUME_CORRUPTED;
  }

Done:
  if (CompareBuffer != NULL) {
    FreePool (CompareBuffer);
  }

  return Status;
}

STATIC
EFI_STATUS
InternalWriteBlock (
  IN  EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN  UINT8                       *Buffer,
  IN  UINT32                      BufferSize
  )
/*++

Routine Description:

  Write a block of data.

Arguments:

  BaseAddress  - Base address of the block.
  Buffer       - Data buffer.
  BufferSize   - Size of the buffer.

Returns:

  EFI_SUCCESS           - The command completed successfully.
  EFI_INVALID_PARAMETER - Invalid parameter, can not proceed.
  Other                 - Device error or wirte-locked, operation failed.

--*/
{
  EFI_STATUS                              Status;

  Status = SpiFlashWrite ((UINTN) BaseAddress, &BufferSize, Buffer);
  ASSERT_EFI_ERROR(Status);
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "\nFlash write error."));
    return Status;
  }

  WriteBackInvalidateDataCacheRange ((VOID *) (UINTN) BaseAddress, BLOCK_SIZE);

  Status = InternalCompareBlock (BaseAddress, Buffer);
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "\nError when writing to BaseAddress %lx with different at offset %x.", BaseAddress, Status));
  } else {
    DEBUG((EFI_D_INFO, "\nVerified data written to Block at %lx is correct.", BaseAddress));
  }

  return Status;

}

STATIC
EFI_STATUS
ReadFileData (
  IN  CHAR16   *FileName,
  OUT UINT8    **Buffer,
  OUT UINT32   *BufferSize
  )
{
  EFI_STATUS             Status;
  SHELL_FILE_HANDLE      FileHandle;
  UINT64                 Size;
  VOID                   *NewBuffer;
  UINTN                  ReadSize;

  FileHandle = NULL;
  NewBuffer = NULL;
  Size = 0;

  Status = ShellOpenFileByName (FileName, &FileHandle, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = FileHandleIsDirectory (FileHandle);
  if (!EFI_ERROR (Status)) {
    Status = EFI_NOT_FOUND;
    goto Done;
  }

  Status = FileHandleGetSize (FileHandle, &Size);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  NewBuffer = AllocatePool ((UINTN) Size);

  ReadSize = (UINTN) Size;
  Status = FileHandleRead (FileHandle, &ReadSize, NewBuffer);
  if (EFI_ERROR (Status)) {
    goto Done;
  } else if (ReadSize != (UINTN) Size) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

Done:
  if (FileHandle != NULL) {
    ShellCloseFile (&FileHandle);
  }

  if (EFI_ERROR (Status)) {
    if (NewBuffer != NULL) {
      FreePool (NewBuffer);
    }
  } else {
    *Buffer = NewBuffer;
    *BufferSize = (UINT32) Size;
  }

  return Status;
}

STATIC
VOID
PrintHelpInfo (
  VOID
  )
/*++

Routine Description:

  Print out help information.

Arguments:

  None.

Returns:

  None.

--*/
{
  PrintToken (STRING_TOKEN (STR_FWUPDATE_FIRMWARE_VOL_UPDATE), HiiHandle);
  PrintToken (STRING_TOKEN (STR_FWUPDATE_VERSION), HiiHandle);
  PrintToken (STRING_TOKEN (STR_FWUPDATE_COPYRIGHT), HiiHandle);

  Print (L"\n");
  PrintToken (STRING_TOKEN (STR_FWUPDATE_USAGE), HiiHandle);
  PrintToken (STRING_TOKEN (STR_FWUPDATE_USAGE_1), HiiHandle);
  PrintToken (STRING_TOKEN (STR_FWUPDATE_USAGE_2), HiiHandle);
  PrintToken (STRING_TOKEN (STR_FWUPDATE_USAGE_3), HiiHandle);
  PrintToken (STRING_TOKEN (STR_FWUPDATE_USAGE_4), HiiHandle);

  Print (L"\n");
}

/**
  Read NumBytes bytes of data from the address specified by
  PAddress into Buffer.

  @param[in]      Address       The starting physical address of the read.
  @param[in,out]  NumBytes      On input, the number of bytes to read. On output, the number
                                of bytes actually read.
  @param[out]     Buffer        The destination data buffer for the read.

  @retval         EFI_SUCCESS       Opertion is successful.
  @retval         EFI_DEVICE_ERROR  If there is any device errors.

**/
EFI_STATUS
EFIAPI
SpiFlashRead (
  IN     UINTN     Address,
  IN OUT UINT32    *NumBytes,
     OUT UINT8     *Buffer
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;
  UINTN         Offset = 0;

  ASSERT ((NumBytes != NULL) && (Buffer != NULL));

    Offset = Address - (UINTN)PcdGet32 (PcdFlashChipBase);

    Status = mSpiProtocol->Execute (
                             mSpiProtocol,
                             1, //SPI_READ,
                             0, //SPI_WREN,
                             TRUE,
                             TRUE,
                             FALSE,
                             Offset,
                             BLOCK_SIZE,
                             Buffer,
                             EnumSpiRegionAll
                             );
    return Status;

}

/**
  Write NumBytes bytes of data from Buffer to the address specified by
  PAddresss.

  @param[in]      Address         The starting physical address of the write.
  @param[in,out]  NumBytes        On input, the number of bytes to write. On output,
                                  the actual number of bytes written.
  @param[in]      Buffer          The source data buffer for the write.

  @retval         EFI_SUCCESS       Opertion is successful.
  @retval         EFI_DEVICE_ERROR  If there is any device errors.

**/
EFI_STATUS
EFIAPI
SpiFlashWrite (
  IN     UINTN     Address,
  IN OUT UINT32    *NumBytes,
  IN     UINT8     *Buffer
  )
{
  EFI_STATUS                Status;
  UINTN                     Offset;
  UINT32                    Length;
  UINT32                    RemainingBytes;

  ASSERT ((NumBytes != NULL) && (Buffer != NULL));
  ASSERT (Address >= (UINTN)PcdGet32 (PcdFlashChipBase));

  Offset    = Address - (UINTN)PcdGet32 (PcdFlashChipBase);

  ASSERT ((*NumBytes + Offset) <= (UINTN)PcdGet32 (PcdFlashChipSize));

  Status = EFI_SUCCESS;
  RemainingBytes = *NumBytes;

  while (RemainingBytes > 0) {
    if (RemainingBytes > SIZE_4KB) {
      Length = SIZE_4KB;
    } else {
      Length = RemainingBytes;
    }
    Status = mSpiProtocol->Execute (
                             mSpiProtocol,
                             SPI_PROG,
                             SPI_WREN,
                             TRUE,
                             TRUE,
                             TRUE,
                             (UINT32) Offset,
                             Length,
                             Buffer,
                             EnumSpiRegionAll
                             );
    if (EFI_ERROR (Status)) {
      break;
    }
    RemainingBytes -= Length;
    Offset += Length;
    Buffer += Length;
  }

  //
  // Actual number of bytes written.
  //
  *NumBytes -= RemainingBytes;

  return Status;
}

/**
  Erase the block starting at Address.

  @param[in]  Address         The starting physical address of the block to be erased.
                              This library assume that caller garantee that the PAddress
                              is at the starting address of this block.
  @param[in]  NumBytes        On input, the number of bytes of the logical block to be erased.
                              On output, the actual number of bytes erased.

  @retval     EFI_SUCCESS.      Opertion is successful.
  @retval     EFI_DEVICE_ERROR  If there is any device errors.

**/
EFI_STATUS
EFIAPI
SpiFlashBlockErase (
  IN UINTN    Address,
  IN UINTN    *NumBytes
  )
{
  EFI_STATUS          Status;
  UINTN               Offset;
  UINTN               RemainingBytes;

  ASSERT (NumBytes != NULL);
  ASSERT (Address >= (UINTN)PcdGet32 (PcdFlashChipBase));

  Offset    = Address - (UINTN)PcdGet32 (PcdFlashChipBase);

  ASSERT ((*NumBytes % SIZE_4KB) == 0);
  ASSERT ((*NumBytes + Offset) <= (UINTN)PcdGet32 (PcdFlashChipSize));

  Status = EFI_SUCCESS;
  RemainingBytes = *NumBytes;

    while (RemainingBytes > 0) {
      Status = mSpiProtocol->Execute (
                               mSpiProtocol,
                               SPI_SERASE,
                               SPI_WREN,
                               FALSE,
                               TRUE,
                               FALSE,
                               (UINT32) Offset,
                               0,
                               NULL,
                               EnumSpiRegionAll
                               );
      if (EFI_ERROR (Status)) {
        break;
      }
      RemainingBytes -= SIZE_4KB;
      Offset         += SIZE_4KB;
    }

  //
  // Actual number of bytes erased.
  //
  *NumBytes -= RemainingBytes;

  return Status;
}

EFI_STATUS
EFIAPI
ConvertMac (
  CHAR16 *Str
  )
{
  UINTN Index;
  UINT8 Temp[MAC_ADD_STR_LEN];

  if (Str == NULL)
    return EFI_INVALID_PARAMETER;

  if (StrLen(Str) != MAC_ADD_STR_LEN)
    return EFI_INVALID_PARAMETER;

  for (Index = 0; Index < MAC_ADD_STR_LEN; Index++) {
    if (Str[Index] >= 0x30 && Str[Index] <= 0x39) {
      Temp[Index] = (UINT8)Str[Index] - 0x30;
    } else if (Str[Index] >= 0x41 && Str[Index] <= 0x46) {
      Temp[Index] = (UINT8)Str[Index] - 0x37;
    } else if (Str[Index] >= 0x61 && Str[Index] <= 0x66) {
      Temp[Index] = (UINT8)Str[Index] - 0x57;
    } else {
      return EFI_INVALID_PARAMETER;
    }
  }

  for (Index = 0; Index < MAC_ADD_BYTE_COUNT; Index++) {
    mInputData.MacValue[Index] = (Temp[2 * Index] << 4) + Temp[2 * Index + 1];
  }

  return EFI_SUCCESS;
}

