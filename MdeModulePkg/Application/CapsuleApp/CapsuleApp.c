/** @file
  A shell application that triggers capsule update process.

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Guid/GlobalVariable.h>
#include <Guid/CapsuleReport.h>
#include <Guid/SystemResourceTable.h>
#include <Guid/FmpCapsule.h>
#include <IndustryStandard/WindowsUxCapsule.h>

#define CAPSULE_HEADER_SIZE  0x20

#define NESTED_CAPSULE_HEADER_SIZE  SIZE_4KB
#define SYSTEM_FIRMWARE_FLAG 0x50000
#define DEVICE_FIRMWARE_FLAG 0x78010

#define MAJOR_VERSION   1
#define MINOR_VERSION   0

#define MAX_CAPSULE_NUM 10

extern UINTN  Argc;
extern CHAR16 **Argv;

//
// Define how many block descriptors we want to test with.
//
UINTN  NumberOfDescriptors = 1;
UINTN  CapsuleFirstIndex;
UINTN  CapsuleLastIndex;

/**
  Dump capsule information

  @param[in] CapsuleName  The name of the capsule image.

  @retval EFI_SUCCESS            The capsule information is dumped.
  @retval EFI_UNSUPPORTED        Input parameter is not valid.
**/
EFI_STATUS
DumpCapsule (
  IN CHAR16                                        *CapsuleName
  );

/**
  Dump capsule status variable.

  @retval EFI_SUCCESS            The capsule status variable is dumped.
  @retval EFI_UNSUPPORTED        Input parameter is not valid.
**/
EFI_STATUS
DmpCapsuleStatusVariable (
  VOID
  );

/**
  Dump FMP protocol info.
**/
VOID
DumpFmpData (
  VOID
  );

/**
  Dump FMP image data.

  @param[in]  ImageTypeId   The ImageTypeId of the FMP image.
                            It is used to identify the FMP protocol.
  @param[in]  ImageIndex    The ImageIndex of the FMP image.
                            It is the input parameter for FMP->GetImage().
  @param[in]  ImageName     The file name to hold the output FMP image.
**/
VOID
DumpFmpImage (
  IN EFI_GUID  *ImageTypeId,
  IN UINTN     ImageIndex,
  IN CHAR16    *ImageName
  );

/**
  Dump ESRT info.
**/
VOID
DumpEsrtData (
  VOID
  );

/**
  Read a file.

  @param[in]  FileName        The file to be read.
  @param[out] BufferSize      The file buffer size
  @param[out] Buffer          The file buffer

  @retval EFI_SUCCESS    Read file successfully
  @retval EFI_NOT_FOUND  Shell protocol or file not found
  @retval others         Read file failed
**/
EFI_STATUS
ReadFileToBuffer (
  IN  CHAR16                               *FileName,
  OUT UINTN                                *BufferSize,
  OUT VOID                                 **Buffer
  );

/**
  Write a file.

  @param[in] FileName        The file to be written.
  @param[in] BufferSize      The file buffer size
  @param[in] Buffer          The file buffer

  @retval EFI_SUCCESS    Write file successfully
  @retval EFI_NOT_FOUND  Shell protocol not found
  @retval others         Write file failed
**/
EFI_STATUS
WriteFileFromBuffer (
  IN  CHAR16                               *FileName,
  IN  UINTN                                BufferSize,
  IN  VOID                                 *Buffer
  );

/**

  This function parse application ARG.

  @return Status
**/
EFI_STATUS
GetArg (
  VOID
  );

/**
  Create UX capsule.

  @retval EFI_SUCCESS            The capsule header is appended.
  @retval EFI_UNSUPPORTED        Input parameter is not valid.
  @retval EFI_OUT_OF_RESOURCES   No enough resource to create UX capsule.
**/
EFI_STATUS
CreateBmpFmp (
  VOID
  )
{
  CHAR16                                        *OutputCapsuleName;
  VOID                                          *BmpBuffer;
  UINTN                                         FileSize;
  CHAR16                                        *BmpName;
  UINT8                                         *FullCapsuleBuffer;
  UINTN                                         FullCapsuleBufferSize;
  EFI_DISPLAY_CAPSULE                           *DisplayCapsule;
  EFI_STATUS                                    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL                  *Gop;

  Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&Gop);
  if (EFI_ERROR(Status)) {
    Print(L"CapsuleApp: NO GOP is found.\n");
    return EFI_UNSUPPORTED;
  }
  Print(L"Current GOP: Mode - %d, ", Gop->Mode->Mode);
  Print(L"HorizontalResolution - %d, ", Gop->Mode->Info->HorizontalResolution);
  Print(L"VerticalResolution - %d\n", Gop->Mode->Info->VerticalResolution);
  // HorizontalResolution >= BMP_IMAGE_HEADER.PixelWidth
  // VerticalResolution   >= BMP_IMAGE_HEADER.PixelHeight

  if (Argc != 5) {
    Print(L"CapsuleApp: Invalid Parameter.\n");
    return EFI_UNSUPPORTED;
  }

  if (StrCmp(Argv[3], L"-O") != 0) {
    Print(L"CapsuleApp: NO output capsule name.\n");
    return EFI_UNSUPPORTED;
  }
  OutputCapsuleName = Argv[4];

  BmpBuffer = NULL;
  FileSize = 0;
  FullCapsuleBuffer = NULL;

  BmpName = Argv[2];
  Status = ReadFileToBuffer(BmpName, &FileSize, &BmpBuffer);
  if (EFI_ERROR(Status)) {
    Print(L"CapsuleApp: BMP image (%s) is not found.\n", BmpName);
    goto Done;
  }

  FullCapsuleBufferSize = sizeof(EFI_DISPLAY_CAPSULE) + FileSize;
  FullCapsuleBuffer = AllocatePool(FullCapsuleBufferSize);
  if (FullCapsuleBuffer == NULL) {
    Print(L"CapsuleApp: Capsule Buffer size (0x%x) too big.\n", FullCapsuleBufferSize);
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  DisplayCapsule = (EFI_DISPLAY_CAPSULE *)FullCapsuleBuffer;
  CopyGuid(&DisplayCapsule->CapsuleHeader.CapsuleGuid, &gWindowsUxCapsuleGuid);
  DisplayCapsule->CapsuleHeader.HeaderSize = sizeof(DisplayCapsule->CapsuleHeader);
  DisplayCapsule->CapsuleHeader.Flags = CAPSULE_FLAGS_PERSIST_ACROSS_RESET;
  DisplayCapsule->CapsuleHeader.CapsuleImageSize = (UINT32)FullCapsuleBufferSize;

  DisplayCapsule->ImagePayload.Version = 1;
  DisplayCapsule->ImagePayload.Checksum = 0;
  DisplayCapsule->ImagePayload.ImageType = 0; // BMP
  DisplayCapsule->ImagePayload.Reserved = 0;
  DisplayCapsule->ImagePayload.Mode = Gop->Mode->Mode;
  DisplayCapsule->ImagePayload.OffsetX = 0;
  DisplayCapsule->ImagePayload.OffsetY = 0;

  CopyMem((DisplayCapsule + 1), BmpBuffer, FileSize);

  DisplayCapsule->ImagePayload.Checksum = CalculateCheckSum8(FullCapsuleBuffer, FullCapsuleBufferSize);

  Status = WriteFileFromBuffer(OutputCapsuleName, FullCapsuleBufferSize, FullCapsuleBuffer);
  Print(L"CapsuleApp: Write %s %r\n", OutputCapsuleName, Status);

Done:
  if (BmpBuffer != NULL) {
    FreePool(BmpBuffer);
  }

  if (FullCapsuleBuffer != NULL) {
    FreePool(FullCapsuleBuffer);
  }

  return Status;
}

/**
  Get ImageTypeId in the FMP capsule header.

  @param[in] CapsuleHeader  The FMP capsule image header.

  @return ImageTypeId
**/
EFI_GUID *
GetCapsuleImageTypeId (
  IN EFI_CAPSULE_HEADER                            *CapsuleHeader
  )
{
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER       *FmpCapsuleHeader;
  UINT64                                       *ItemOffsetList;
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER *ImageHeader;

  FmpCapsuleHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER *)((UINT8 *)CapsuleHeader + CapsuleHeader->HeaderSize);
  ItemOffsetList = (UINT64 *)(FmpCapsuleHeader + 1);
  if (FmpCapsuleHeader->PayloadItemCount == 0) {
    return NULL;
  }
  ImageHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER *)((UINT8 *)FmpCapsuleHeader + ItemOffsetList[FmpCapsuleHeader->EmbeddedDriverCount]);
  return &ImageHeader->UpdateImageTypeId;
}

/**
  Get ESRT FwType according to ImageTypeId

  @param[in]  ImageTypeId   ImageTypeId of an FMP capsule.

  @return ESRT FwType
**/
UINT32
GetEsrtFwType (
  IN  EFI_GUID                                      *ImageTypeId
  )
{
  EFI_STATUS                 Status;
  EFI_SYSTEM_RESOURCE_TABLE  *Esrt;
  EFI_SYSTEM_RESOURCE_ENTRY  *EsrtEntry;
  UINTN                      Index;

  //
  // Check ESRT
  //
  Status = EfiGetSystemConfigurationTable(&gEfiSystemResourceTableGuid, (VOID **)&Esrt);
  if (!EFI_ERROR(Status)) {
    ASSERT(Esrt != NULL);
    EsrtEntry = (VOID *)(Esrt + 1);
    for (Index = 0; Index < Esrt->FwResourceCount; Index++, EsrtEntry++) {
      if (CompareGuid(&EsrtEntry->FwClass, ImageTypeId)) {
        return EsrtEntry->FwType;
      }
    }
  }

  return ESRT_FW_TYPE_UNKNOWN;
}

/**
  Append a capsule header on top of current image.
  This function follows Windows UEFI Firmware Update Platform document.

  @retval EFI_SUCCESS            The capsule header is appended.
  @retval EFI_UNSUPPORTED        Input parameter is not valid.
  @retval EFI_OUT_OF_RESOURCES   No enough resource to append capsule header.
**/
EFI_STATUS
CreateNestedFmp (
  VOID
  )
{
  CHAR16                                        *OutputCapsuleName;
  VOID                                          *CapsuleBuffer;
  UINTN                                         FileSize;
  CHAR16                                        *CapsuleName;
  UINT8                                         *FullCapsuleBuffer;
  UINTN                                         FullCapsuleBufferSize;
  EFI_CAPSULE_HEADER                            *NestedCapsuleHeader;
  EFI_GUID                                      *ImageTypeId;
  UINT32                                        FwType;
  EFI_STATUS                                    Status;

  if (Argc != 5) {
    Print(L"CapsuleApp: Invalid Parameter.\n");
    return EFI_UNSUPPORTED;
  }

  if (StrCmp(Argv[3], L"-O") != 0) {
    Print(L"CapsuleApp: NO output capsule name.\n");
    return EFI_UNSUPPORTED;
  }
  OutputCapsuleName = Argv[4];

  CapsuleBuffer = NULL;
  FileSize = 0;
  FullCapsuleBuffer = NULL;

  CapsuleName = Argv[2];
  Status = ReadFileToBuffer(CapsuleName, &FileSize, &CapsuleBuffer);
  if (EFI_ERROR(Status)) {
    Print(L"CapsuleApp: Capsule image (%s) is not found.\n", CapsuleName);
    goto Done;
  }

  ImageTypeId = GetCapsuleImageTypeId(CapsuleBuffer);
  if (ImageTypeId == NULL) {
    Print(L"CapsuleApp: Capsule ImageTypeId is not found.\n");
    goto Done;
  }
  FwType = GetEsrtFwType(ImageTypeId);
  if ((FwType != ESRT_FW_TYPE_SYSTEMFIRMWARE) && (FwType != ESRT_FW_TYPE_DEVICEFIRMWARE)) {
    Print(L"CapsuleApp: Capsule FwType is invalid.\n");
    goto Done;
  }

  FullCapsuleBufferSize = NESTED_CAPSULE_HEADER_SIZE + FileSize;
  FullCapsuleBuffer = AllocatePool(FullCapsuleBufferSize);
  if (FullCapsuleBuffer == NULL) {
    Print(L"CapsuleApp: Capsule Buffer size (0x%x) too big.\n", FullCapsuleBufferSize);
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  NestedCapsuleHeader = (EFI_CAPSULE_HEADER *)FullCapsuleBuffer;
  ZeroMem(NestedCapsuleHeader, NESTED_CAPSULE_HEADER_SIZE);
  CopyGuid(&NestedCapsuleHeader->CapsuleGuid, ImageTypeId);
  NestedCapsuleHeader->HeaderSize = NESTED_CAPSULE_HEADER_SIZE;
  NestedCapsuleHeader->Flags = (FwType == ESRT_FW_TYPE_DEVICEFIRMWARE) ? SYSTEM_FIRMWARE_FLAG : DEVICE_FIRMWARE_FLAG;
  NestedCapsuleHeader->CapsuleImageSize = (UINT32)FullCapsuleBufferSize;

  CopyMem((UINT8 *)NestedCapsuleHeader + NestedCapsuleHeader->HeaderSize, CapsuleBuffer, FileSize);

  Status = WriteFileFromBuffer(OutputCapsuleName, FullCapsuleBufferSize, FullCapsuleBuffer);
  Print(L"CapsuleApp: Write %s %r\n", OutputCapsuleName, Status);

Done:
  if (CapsuleBuffer != NULL) {
    FreePool(CapsuleBuffer);
  }

  if (FullCapsuleBuffer != NULL) {
    FreePool(FullCapsuleBuffer);
  }

  return Status;
}


/**
  Clear capsule status variable.

  @retval EFI_SUCCESS            The capsule status variable is cleared.
**/
EFI_STATUS
ClearCapsuleStatusVariable (
  VOID
  )
{
  EFI_STATUS                          Status;
  UINT32                              Index;
  CHAR16                              CapsuleVarName[20];
  CHAR16                              *TempVarName;

  StrCpyS (CapsuleVarName, sizeof(CapsuleVarName)/sizeof(CapsuleVarName[0]), L"Capsule");
  TempVarName = CapsuleVarName + StrLen (CapsuleVarName);
  Index = 0;

  while (TRUE) {
    UnicodeSPrint (TempVarName, 5 * sizeof(CHAR16), L"%04x", Index);

    Status = gRT->SetVariable (
                    CapsuleVarName,
                    &gEfiCapsuleReportGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    0,
                    (VOID *)NULL
                    );
    if (EFI_ERROR(Status)) {
      //
      // There is no capsule variables, quit
      //
      break;
    }

    Index++;
    if (Index > 0xFFFF) {
      break;
    }
  }

  return EFI_SUCCESS;
}

/**
  Build Gather list for a list of capsule images.

  @param[in]  CapsuleBuffer    An array of pointer to capsule images
  @param[in]  FileSize         An array of UINTN to capsule images size
  @param[in]  CapsuleNum       The count of capsule images
  @param[out] BlockDescriptors The block descriptors for the capsule images

  @retval EFI_SUCCESS The block descriptors for the capsule images are constructed.
**/
EFI_STATUS
BuildGatherList (
  IN VOID                          **CapsuleBuffer,
  IN UINTN                         *FileSize,
  IN UINTN                         CapsuleNum,
  OUT EFI_CAPSULE_BLOCK_DESCRIPTOR **BlockDescriptors
  )
{
  EFI_STATUS                    Status;
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *BlockDescriptors1;
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *BlockDescriptors2;
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *BlockDescriptorPre;
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *BlockDescriptorsHeader;
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *TempBlockPtr;
  UINT8                         *TempDataPtr;
  UINTN                         SizeLeft;
  UINTN                         Size;
  INT32                         Count;
  INT32                         Number;
  UINTN                         Index;

  TempBlockPtr           = NULL;
  BlockDescriptors1      = NULL;
  BlockDescriptors2      = NULL;
  BlockDescriptorPre     = NULL;
  BlockDescriptorsHeader = NULL;

  for (Index = 0; Index < CapsuleNum; Index++) {
    //
    // Allocate memory for the descriptors.
    //
    if (NumberOfDescriptors == 1) {
      Count = 2;
    } else {
      Count = (INT32)(NumberOfDescriptors + 2) / 2;
    }

    Size               = Count * sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR);
    BlockDescriptors1  = AllocateRuntimeZeroPool (Size);
    if (BlockDescriptors1 == NULL) {
      Print (L"CapsuleApp: failed to allocate memory for descriptors\n");
      Status = EFI_OUT_OF_RESOURCES;
      goto ERREXIT;
    } else {
      Print (L"CapsuleApp: creating capsule descriptors at 0x%X\n", (UINTN) BlockDescriptors1);
      Print (L"CapsuleApp: capsule data starts          at 0x%X with size 0x%X\n", (UINTN) CapsuleBuffer[Index], FileSize[Index]);
    }

    //
    // Record descirptor header
    //
    if (Index == 0) {
      BlockDescriptorsHeader = BlockDescriptors1;
    }

    if (BlockDescriptorPre != NULL) {
      BlockDescriptorPre->Union.ContinuationPointer = (UINTN) BlockDescriptors1;
      BlockDescriptorPre->Length = 0;
    }

    //
    // Fill them in
    //
    TempBlockPtr  = BlockDescriptors1;
    TempDataPtr   = CapsuleBuffer[Index];
    SizeLeft      = FileSize[Index];
    for (Number = 0; (Number < Count - 1) && (SizeLeft != 0); Number++) {
      //
      // Divide remaining data in half
      //
      if (NumberOfDescriptors != 1) {
        if (SizeLeft == 1) {
          Size = 1;
        } else {
          Size = SizeLeft / 2;
        }
      } else {
        Size = SizeLeft;
      }
      TempBlockPtr->Union.DataBlock    = (UINTN)TempDataPtr;
      TempBlockPtr->Length  = Size;
      Print (L"CapsuleApp: capsule block/size              0x%X/0x%X\n", (UINTN) TempDataPtr, Size);
      SizeLeft -= Size;
      TempDataPtr += Size;
      TempBlockPtr++;
    }

    //
    // Allocate the second list, point the first block's last entry to point
    // to this one, and fill this one in. Worst case is that the previous
    // list only had one element that pointed here, so we need at least two
    // elements -- one to point to all the data, another to terminate the list.
    //
    if ((NumberOfDescriptors != 1) && (SizeLeft != 0)) {
      Count = (INT32)(NumberOfDescriptors + 2) - Count;
      if (Count == 1) {
        Count++;
      }

      Size              = Count * sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR);
      BlockDescriptors2 = AllocateRuntimeZeroPool (Size);
      if (BlockDescriptors2 == NULL) {
        Print (L"CapsuleApp: failed to allocate memory for descriptors\n");
        Status = EFI_OUT_OF_RESOURCES;
        goto ERREXIT;
      }

      //
      // Point the first list's last element to point to this second list.
      //
      TempBlockPtr->Union.ContinuationPointer   = (UINTN) BlockDescriptors2;

      TempBlockPtr->Length  = 0;
      TempBlockPtr = BlockDescriptors2;
      for (Number = 0; Number < Count - 1; Number++) {
        //
        // If second-to-last one, then dump rest to this element
        //
        if (Number == (Count - 2)) {
          Size = SizeLeft;
        } else {
          //
          // Divide remaining data in half
          //
          if (SizeLeft == 1) {
            Size = 1;
          } else {
            Size = SizeLeft / 2;
          }
        }

        TempBlockPtr->Union.DataBlock    = (UINTN)TempDataPtr;
        TempBlockPtr->Length  = Size;
        Print (L"CapsuleApp: capsule block/size              0x%X/0x%X\n", (UINTN) TempDataPtr, Size);
        SizeLeft -= Size;
        TempDataPtr += Size;
        TempBlockPtr++;
        if (SizeLeft == 0) {
          break;
        }
      }
    }

    BlockDescriptorPre = TempBlockPtr;
    BlockDescriptors1  = NULL;
  }

  //
  // Null-terminate.
  //
  if (TempBlockPtr != NULL) {
    TempBlockPtr->Union.ContinuationPointer    = (UINTN)NULL;
    TempBlockPtr->Length  = 0;
    *BlockDescriptors = BlockDescriptorsHeader;
  }

  return EFI_SUCCESS;

ERREXIT:
  if (BlockDescriptors1 != NULL) {
    FreePool(BlockDescriptors1);
  }

  if (BlockDescriptors2 != NULL) {
    FreePool(BlockDescriptors2);
  }

  return Status;
}

/**
  Clear the Gather list for a list of capsule images.

  @param[in]  BlockDescriptors The block descriptors for the capsule images
  @param[in]  CapsuleNum       The count of capsule images
**/
VOID
CleanGatherList (
  IN EFI_CAPSULE_BLOCK_DESCRIPTOR   *BlockDescriptors,
  IN UINTN                          CapsuleNum
  )
{
  EFI_CAPSULE_BLOCK_DESCRIPTOR   *TempBlockPtr;
  EFI_CAPSULE_BLOCK_DESCRIPTOR   *TempBlockPtr1;
  EFI_CAPSULE_BLOCK_DESCRIPTOR   *TempBlockPtr2;
  UINTN                          Index;

  if (BlockDescriptors != NULL) {
    TempBlockPtr1 = BlockDescriptors;
    while (1){
      TempBlockPtr = TempBlockPtr1;
      for (Index = 0; Index < CapsuleNum; Index++) {
        if (TempBlockPtr[Index].Length == 0) {
          break;
        }
      }

      if (TempBlockPtr[Index].Union.ContinuationPointer == (UINTN)NULL) {
        break;
      }

      TempBlockPtr2 = (VOID *) ((UINTN) TempBlockPtr[Index].Union.ContinuationPointer);
      FreePool(TempBlockPtr1);
      TempBlockPtr1 = TempBlockPtr2;
    }
  }
}

/**
  Print APP usage.
**/
VOID
PrintUsage (
  VOID
  )
{
  Print(L"CapsuleApp:  usage\n");
  Print(L"  CapsuleApp <Capsule...> [-NR]\n");
  Print(L"  CapsuleApp -S\n");
  Print(L"  CapsuleApp -C\n");
  Print(L"  CapsuleApp -P\n");
  Print(L"  CapsuleApp -E\n");
  Print(L"  CapsuleApp -G <BMP> -O <Capsule>\n");
  Print(L"  CapsuleApp -N <Capsule> -O <NestedCapsule>\n");
  Print(L"  CapsuleApp -D <Capsule>\n");
  Print(L"  CapsuleApp -P GET <ImageTypeId> <Index> -O <FileName>\n");
  Print(L"Parameter:\n");
  Print(L"  -NR: No reset will be triggered for the capsule\n");
  Print(L"       with CAPSULE_FLAGS_PERSIST_ACROSS_RESET and without CAPSULE_FLAGS_INITIATE_RESET.\n");
  Print(L"  -S:  Dump capsule report variable (EFI_CAPSULE_REPORT_GUID),\n");
  Print(L"       which is defined in UEFI specification.\n");
  Print(L"  -C:  Clear capsule report variable (EFI_CAPSULE_RPORT_GUID),\n");
  Print(L"       which is defined in UEFI specification.\n");
  Print(L"  -P:  Dump UEFI FMP protocol info.\n");
  Print(L"  -E:  Dump UEFI ESRT table info.\n");
  Print(L"  -G:  Convert a BMP file to be a UX capsule,\n");
  Print(L"       according to Windows Firmware Update document\n");
  Print(L"  -N:  Append a Capsule Header to an existing capsule image,\n");
  Print(L"       according to Windows Firmware Update document\n");
  Print(L"  -O:  Output new Capsule file name\n");
  Print(L"  -D:  Dump Capsule image header information and FMP header information,\n");
  Print(L"       if it is an FMP capsule.\n");
}

/**
  Update Capsule image.

  @param[in]  ImageHandle     The image handle.
  @param[in]  SystemTable     The system table.

  @retval EFI_SUCCESS            Command completed successfully.
  @retval EFI_INVALID_PARAMETER  Command usage error.
  @retval EFI_NOT_FOUND          The input file can't be found.
**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                    Status;
  RETURN_STATUS                 RStatus;
  UINTN                         FileSize[MAX_CAPSULE_NUM];
  VOID                          *CapsuleBuffer[MAX_CAPSULE_NUM];
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *BlockDescriptors;
  EFI_CAPSULE_HEADER             *CapsuleHeaderArray[MAX_CAPSULE_NUM + 1];
  UINT64                         MaxCapsuleSize;
  EFI_RESET_TYPE                 ResetType;
  BOOLEAN                        NeedReset;
  BOOLEAN                        NoReset;
  CHAR16                         *CapsuleName;
  UINTN                          CapsuleNum;
  UINTN                          Index;

  Status = GetArg();
  if (EFI_ERROR(Status)) {
    Print(L"Please use UEFI SHELL to run this application!\n", Status);
    return Status;
  }
  if (Argc < 2) {
    PrintUsage();
    return EFI_INVALID_PARAMETER;
  }
  if (StrCmp(Argv[1], L"-D") == 0) {
    Status = DumpCapsule(Argv[2]);
    return Status;
  }
  if (StrCmp(Argv[1], L"-G") == 0) {
    Status = CreateBmpFmp();
    return Status;
  }
  if (StrCmp(Argv[1], L"-N") == 0) {
    Status = CreateNestedFmp();
    return Status;
  }
  if (StrCmp(Argv[1], L"-S") == 0) {
    Status = DmpCapsuleStatusVariable();
    return EFI_SUCCESS;
  }
  if (StrCmp(Argv[1], L"-C") == 0) {
    Status = ClearCapsuleStatusVariable();
    return Status;
  }
  if (StrCmp(Argv[1], L"-P") == 0) {
    if (Argc == 2) {
      DumpFmpData();
    }
    if (Argc >= 3) {
      if (StrCmp(Argv[2], L"GET") == 0) {
        EFI_GUID  ImageTypeId;
        UINTN     ImageIndex;
        //
        // FMP->GetImage()
        //
        RStatus = StrToGuid (Argv[3], &ImageTypeId);
        if (RETURN_ERROR (RStatus) || (Argv[3][GUID_STRING_LENGTH] != L'\0')) {
          Print (L"Invalid ImageTypeId - %s\n", Argv[3]);
          return EFI_INVALID_PARAMETER;
        }
        ImageIndex = StrDecimalToUintn(Argv[4]);
        if (StrCmp(Argv[5], L"-O") == 0) {
          DumpFmpImage(&ImageTypeId, ImageIndex, Argv[6]);
        }
      }
    }
    return EFI_SUCCESS;
  }
  if (StrCmp(Argv[1], L"-E") == 0) {
    DumpEsrtData();
    return EFI_SUCCESS;
  }
  CapsuleFirstIndex = 1;
  NoReset = FALSE;
  if ((Argc > 1) && (StrCmp(Argv[Argc - 1], L"-NR") == 0)) {
    NoReset = TRUE;
    CapsuleLastIndex = Argc - 2;
  } else {
    CapsuleLastIndex = Argc - 1;
  }
  CapsuleNum = CapsuleLastIndex - CapsuleFirstIndex + 1;

  if (CapsuleFirstIndex > CapsuleLastIndex) {
    Print(L"CapsuleApp: NO capsule image.\n");
    return EFI_UNSUPPORTED;
  }
  if (CapsuleNum > MAX_CAPSULE_NUM) {
    Print(L"CapsuleApp: Too many capsule images.\n");
    return EFI_UNSUPPORTED;
  }

  ZeroMem(&CapsuleBuffer, sizeof(CapsuleBuffer));
  ZeroMem(&FileSize, sizeof(FileSize));
  BlockDescriptors = NULL;

  for (Index = 0; Index < CapsuleNum; Index++) {
    CapsuleName = Argv[CapsuleFirstIndex + Index];
    Status = ReadFileToBuffer(CapsuleName, &FileSize[Index], &CapsuleBuffer[Index]);
    if (EFI_ERROR(Status)) {
      Print(L"CapsuleApp: capsule image (%s) is not found.\n", CapsuleName);
      goto Done;
    }
  }

  //
  // Every capsule use 2 descriptor 1 for data 1 for end
  //
  Status = BuildGatherList(CapsuleBuffer, FileSize, CapsuleNum, &BlockDescriptors);
  if (EFI_ERROR(Status)) {
    goto Done;
  }

  //
  // Call the runtime service capsule.
  //
  NeedReset = FALSE;
  for (Index = 0; Index < CapsuleNum; Index++) {
    CapsuleHeaderArray[Index] = (EFI_CAPSULE_HEADER *) CapsuleBuffer[Index];
    if ((CapsuleHeaderArray[Index]->Flags & CAPSULE_FLAGS_PERSIST_ACROSS_RESET) != 0) {
      NeedReset = TRUE;
    }
  }
  CapsuleHeaderArray[CapsuleNum] = NULL;

  //
  // Inquire platform capability of UpdateCapsule.
  //
  Status = gRT->QueryCapsuleCapabilities (CapsuleHeaderArray, CapsuleNum, &MaxCapsuleSize, &ResetType);
  if (EFI_ERROR(Status)) {
    Print (L"CapsuleApp: failed to query capsule capability - %r\n", Status);
    goto Done;
  }

  for (Index = 0; Index < CapsuleNum; Index++) {
    if (FileSize[Index] > MaxCapsuleSize) {
      Print (L"CapsuleApp: capsule is too large to update, %ld is allowed\n", MaxCapsuleSize);
      Status = EFI_UNSUPPORTED;
      goto Done;
    }
  }

  //
  // Check whether the input capsule image has the flag of persist across system reset.
  //
  if (NeedReset) {
    Status = gRT->UpdateCapsule(CapsuleHeaderArray,CapsuleNum,(UINTN) BlockDescriptors);
    if (Status != EFI_SUCCESS) {
      Print (L"CapsuleApp: failed to update capsule - %r\n", Status);
      goto Done;
    }
    //
    // For capsule with CAPSULE_FLAGS_PERSIST_ACROSS_RESET + CAPSULE_FLAGS_INITIATE_RESET,
    // a system reset should have been triggered by gRT->UpdateCapsule() calling above.
    //
    // For capsule with CAPSULE_FLAGS_PERSIST_ACROSS_RESET and without CAPSULE_FLAGS_INITIATE_RESET,
    // check if -NR (no-reset) has been specified or not.
    //
    if (!NoReset) {
      //
      // For capsule who has reset flag and no -NR (no-reset) has been specified, after calling UpdateCapsule service,
      // trigger a system reset to process capsule persist across a system reset.
      //
      gRT->ResetSystem (ResetType, EFI_SUCCESS, 0, NULL);
    }
  } else {
    //
    // For capsule who has no reset flag, only call UpdateCapsule Service without a
    // system reset. The service will process the capsule immediately.
    //
    Status = gRT->UpdateCapsule (CapsuleHeaderArray,CapsuleNum,(UINTN) BlockDescriptors);
    if (Status != EFI_SUCCESS) {
      Print (L"CapsuleApp: failed to update capsule - %r\n", Status);
    }
  }

  Status = EFI_SUCCESS;

Done:
  for (Index = 0; Index < CapsuleNum; Index++) {
    if (CapsuleBuffer[Index] != NULL) {
      FreePool (CapsuleBuffer[Index]);
    }
  }

  CleanGatherList(BlockDescriptors, CapsuleNum);

  return Status;
}
