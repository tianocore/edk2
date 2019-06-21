/** @file
  A shell application that triggers capsule update process.

  Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

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
#include <Library/BmpSupportLib.h>
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
DumpCapsuleStatusVariable (
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
  Dump Provisioned Capsule.

  @param[in]  DumpCapsuleInfo  The flag to indicate whether to dump the capsule inforomation.
**/
VOID
DumpProvisionedCapsule (
  IN BOOLEAN                      DumpCapsuleInfo
  );

/**
  Dump all EFI System Partition.
**/
VOID
DumpAllEfiSysPartition (
  VOID
  );

/**
  Process Capsule On Disk.

  @param[in]  CapsuleBuffer       An array of pointer to capsule images
  @param[in]  CapsuleBufferSize   An array of UINTN to capsule images size
  @param[in]  FilePath            An array of capsule images file path
  @param[in]  Map                 File system mapping string
  @param[in]  CapsuleNum          The count of capsule images

  @retval EFI_SUCCESS       Capsule on disk success.
  @retval others            Capsule on disk fail.

**/
EFI_STATUS
ProcessCapsuleOnDisk (
  IN VOID                          **CapsuleBuffer,
  IN UINTN                         *CapsuleBufferSize,
  IN CHAR16                        **FilePath,
  IN CHAR16                        *Map,
  IN UINTN                         CapsuleNum
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
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION          *Info;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL                 *GopBlt;
  UINTN                                         GopBltSize;
  UINTN                                         Height;
  UINTN                                         Width;

  Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&Gop);
  if (EFI_ERROR(Status)) {
    Print(L"CapsuleApp: NO GOP is found.\n");
    return EFI_UNSUPPORTED;
  }
  Info = Gop->Mode->Info;
  Print(L"Current GOP: Mode - %d, ", Gop->Mode->Mode);
  Print(L"HorizontalResolution - %d, ", Info->HorizontalResolution);
  Print(L"VerticalResolution - %d\n", Info->VerticalResolution);
  // HorizontalResolution >= BMP_IMAGE_HEADER.PixelWidth
  // VerticalResolution   >= BMP_IMAGE_HEADER.PixelHeight

  if (Argc != 5) {
    Print(L"CapsuleApp: Incorrect parameter count.\n");
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

  GopBlt = NULL;
  Status = TranslateBmpToGopBlt (
             BmpBuffer,
             FileSize,
             &GopBlt,
             &GopBltSize,
             &Height,
             &Width
             );
  if (EFI_ERROR(Status)) {
    Print(L"CapsuleApp: BMP image (%s) is not valid.\n", BmpName);
    goto Done;
  }
  if (GopBlt != NULL) {
    FreePool (GopBlt);
  }
  Print(L"BMP image (%s), Width - %d, Height - %d\n", BmpName, Width, Height);

  if (Height > Info->VerticalResolution) {
    Status = EFI_INVALID_PARAMETER;
    Print(L"CapsuleApp: BMP image (%s) height is larger than current resolution.\n", BmpName);
    goto Done;
  }
  if (Width > Info->HorizontalResolution) {
    Status = EFI_INVALID_PARAMETER;
    Print(L"CapsuleApp: BMP image (%s) width is larger than current resolution.\n", BmpName);
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

  //
  // Center the bitmap horizontally
  //
  DisplayCapsule->ImagePayload.OffsetX = (UINT32)((Info->HorizontalResolution - Width) / 2);

  //
  // Put bitmap 3/4 down the display.  If bitmap is too tall, then align bottom
  // of bitmap at bottom of display.
  //
  DisplayCapsule->ImagePayload.OffsetY =
    MIN (
      (UINT32)(Info->VerticalResolution - Height),
      (UINT32)(((3 * Info->VerticalResolution) - (2 * Height)) / 4)
      );

  Print(L"BMP image (%s), OffsetX - %d, OffsetY - %d\n",
    BmpName,
    DisplayCapsule->ImagePayload.OffsetX,
    DisplayCapsule->ImagePayload.OffsetY
    );

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
  Validate if it is valid capsule header

  This function assumes the caller provided correct CapsuleHeader pointer
  and CapsuleSize.

  This function validates the fields in EFI_CAPSULE_HEADER.

  @param[in] CapsuleHeader  Points to a capsule header.
  @param[in] CapsuleSize    Size of the whole capsule image.

**/
BOOLEAN
IsValidCapsuleHeader (
  IN EFI_CAPSULE_HEADER     *CapsuleHeader,
  IN UINT64                 CapsuleSize
  )
{
  if (CapsuleSize < sizeof (EFI_CAPSULE_HEADER)) {
    return FALSE;
  }
  if (CapsuleHeader->CapsuleImageSize != CapsuleSize) {
    return FALSE;
  }
  if (CapsuleHeader->HeaderSize > CapsuleHeader->CapsuleImageSize) {
    return FALSE;
  }
  if (CapsuleHeader->HeaderSize < sizeof (EFI_CAPSULE_HEADER)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Return if this CapsuleGuid is a FMP capsule GUID or not.

  @param[in] CapsuleGuid A pointer to EFI_GUID

  @retval TRUE  It is a FMP capsule GUID.
  @retval FALSE It is not a FMP capsule GUID.
**/
BOOLEAN
IsFmpCapsuleGuid (
  IN EFI_GUID  *CapsuleGuid
  )
{
  if (CompareGuid(&gEfiFmpCapsuleGuid, CapsuleGuid)) {
    return TRUE;
  }

  return FALSE;
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
    Print(L"CapsuleApp: Incorrect parameter count.\n");
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
  if (!IsValidCapsuleHeader (CapsuleBuffer, FileSize)) {
    Print(L"CapsuleApp: Capsule image (%s) is not a valid capsule.\n", CapsuleName);
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if (!IsFmpCapsuleGuid (&((EFI_CAPSULE_HEADER *) CapsuleBuffer)->CapsuleGuid)) {
    Print(L"CapsuleApp: Capsule image (%s) is not a FMP capsule.\n", CapsuleName);
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  ImageTypeId = GetCapsuleImageTypeId(CapsuleBuffer);
  if (ImageTypeId == NULL) {
    Print(L"CapsuleApp: Capsule ImageTypeId is not found.\n");
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }
  FwType = GetEsrtFwType(ImageTypeId);
  if ((FwType != ESRT_FW_TYPE_SYSTEMFIRMWARE) && (FwType != ESRT_FW_TYPE_DEVICEFIRMWARE)) {
    Print(L"CapsuleApp: Capsule FwType is invalid.\n");
    Status = EFI_INVALID_PARAMETER;
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
  NestedCapsuleHeader->Flags = (FwType == ESRT_FW_TYPE_SYSTEMFIRMWARE) ? SYSTEM_FIRMWARE_FLAG : DEVICE_FIRMWARE_FLAG;
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
  BOOLEAN                             Found;

  StrCpyS (CapsuleVarName, sizeof(CapsuleVarName)/sizeof(CapsuleVarName[0]), L"Capsule");
  TempVarName = CapsuleVarName + StrLen (CapsuleVarName);
  Index = 0;

  Found = FALSE;
  while (TRUE) {
    UnicodeSPrint (TempVarName, 5 * sizeof(CHAR16), L"%04x", Index);

    Status = gRT->SetVariable (
                    CapsuleVarName,
                    &gEfiCapsuleReportGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    0,
                    (VOID *)NULL
                    );
    if (Status == EFI_NOT_FOUND) {
      //
      // There is no more capsule variables, quit
      //
      break;
    }
    Found = TRUE;

    Print (L"Clear %s %r\n", CapsuleVarName, Status);

    Index++;
    if (Index > 0xFFFF) {
      break;
    }
  }

  if (!Found) {
    Print (L"No any Capsule#### variable found\n");
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
  Print(L"  CapsuleApp <Capsule...> [-NR] [-OD [FSx]]\n");
  Print(L"  CapsuleApp -S\n");
  Print(L"  CapsuleApp -C\n");
  Print(L"  CapsuleApp -P\n");
  Print(L"  CapsuleApp -E\n");
  Print(L"  CapsuleApp -L\n");
  Print(L"  CapsuleApp -L INFO\n");
  Print(L"  CapsuleApp -F\n");
  Print(L"  CapsuleApp -G <BMP> -O <Capsule>\n");
  Print(L"  CapsuleApp -N <Capsule> -O <NestedCapsule>\n");
  Print(L"  CapsuleApp -D <Capsule>\n");
  Print(L"  CapsuleApp -P GET <ImageTypeId> <Index> -O <FileName>\n");
  Print(L"Parameter:\n");
  Print(L"  -NR: No reset will be triggered for the capsule\n");
  Print(L"       with CAPSULE_FLAGS_PERSIST_ACROSS_RESET and without CAPSULE_FLAGS_INITIATE_RESET.\n");
  Print(L"  -OD: Delivery of Capsules via file on Mass Storage device.");
  Print(L"  -S:  Dump capsule report variable (EFI_CAPSULE_REPORT_GUID),\n");
  Print(L"       which is defined in UEFI specification.\n");
  Print(L"  -C:  Clear capsule report variable (EFI_CAPSULE_REPORT_GUID),\n");
  Print(L"       which is defined in UEFI specification.\n");
  Print(L"  -P:  Dump UEFI FMP protocol info, or get image with specified\n");
  Print(L"       ImageTypeId and Index (decimal format) to a file if 'GET'\n");
  Print(L"       option is used.\n");
  Print(L"  -E:  Dump UEFI ESRT table info.\n");
  Print(L"  -L:  Dump provisioned capsule image information.\n");
  Print(L"  -F:  Dump all EFI System Partition.\n");
  Print(L"  -G:  Convert a BMP file to be an UX capsule,\n");
  Print(L"       according to Windows Firmware Update document\n");
  Print(L"  -N:  Append a Capsule Header to an existing FMP capsule image\n");
  Print(L"       with its ImageTypeId supported by the system,\n");
  Print(L"       according to Windows Firmware Update document\n");
  Print(L"  -O:  Output new Capsule file name\n");
  Print(L"  -D:  Dump Capsule image header information, image payload\n");
  Print(L"       information if it is an UX capsule and FMP header\n");
  Print(L"       information if it is a FMP capsule.\n");
}

/**
  Update Capsule image.

  @param[in]  ImageHandle     The image handle.
  @param[in]  SystemTable     The system table.

  @retval EFI_SUCCESS            Command completed successfully.
  @retval EFI_UNSUPPORTED        Command usage unsupported.
  @retval EFI_INVALID_PARAMETER  Command usage invalid.
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
  UINTN                         CapsuleBufferSize[MAX_CAPSULE_NUM];
  VOID                          *CapsuleBuffer[MAX_CAPSULE_NUM];
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *BlockDescriptors;
  EFI_CAPSULE_HEADER            *CapsuleHeaderArray[MAX_CAPSULE_NUM + 1];
  UINT64                        MaxCapsuleSize;
  EFI_RESET_TYPE                ResetType;
  BOOLEAN                       NeedReset;
  BOOLEAN                       NoReset;
  BOOLEAN                       CapsuleOnDisk;
  CHAR16                        *CapsuleName;
  CHAR16                        *CapsuleNames[MAX_CAPSULE_NUM];
  CHAR16                        *MapFsStr;
  UINTN                         CapsuleNum;
  UINTN                         Index;
  UINTN                         ParaOdIndex;
  UINTN                         ParaNrIndex;
  EFI_GUID                      ImageTypeId;
  UINTN                         ImageIndex;

  BlockDescriptors  = NULL;
  MapFsStr          = NULL;
  CapsuleNum        = 0;

  Status = GetArg();
  if (EFI_ERROR(Status)) {
    Print(L"Please use UEFI SHELL to run this application!\n", Status);
    return Status;
  }
  if (Argc < 2) {
    PrintUsage();
    return EFI_UNSUPPORTED;
  }
  if (StrCmp(Argv[1], L"-D") == 0) {
    if (Argc != 3) {
      Print(L"CapsuleApp: Incorrect parameter count.\n");
      return EFI_UNSUPPORTED;
    }
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
    Status = DumpCapsuleStatusVariable();
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
      if (StrCmp(Argv[2], L"GET") != 0) {
        Print(L"CapsuleApp: Unrecognized option(%s).\n", Argv[2]);
        return EFI_UNSUPPORTED;
      } else {
        if (Argc != 7) {
          Print(L"CapsuleApp: Incorrect parameter count.\n");
          return EFI_UNSUPPORTED;
        }

        //
        // FMP->GetImage()
        //
        RStatus = StrToGuid (Argv[3], &ImageTypeId);
        if (RETURN_ERROR (RStatus) || (Argv[3][GUID_STRING_LENGTH] != L'\0')) {
          Print (L"Invalid ImageTypeId - %s\n", Argv[3]);
          return EFI_INVALID_PARAMETER;
        }
        ImageIndex = StrDecimalToUintn(Argv[4]);
        if (StrCmp(Argv[5], L"-O") != 0) {
          Print(L"CapsuleApp: NO output file name.\n");
          return EFI_UNSUPPORTED;
        }
        DumpFmpImage(&ImageTypeId, ImageIndex, Argv[6]);
      }
    }
    return EFI_SUCCESS;
  }

  if (StrCmp(Argv[1], L"-E") == 0) {
    DumpEsrtData();
    return EFI_SUCCESS;
  }

  if (StrCmp(Argv[1], L"-L") == 0) {
    if (Argc >= 3 && StrCmp(Argv[2], L"INFO") == 0) {
      DumpProvisionedCapsule(TRUE);
    } else {
      DumpProvisionedCapsule(FALSE);
    }
    return EFI_SUCCESS;
  }

  if (StrCmp(Argv[1], L"-F") == 0) {
    DumpAllEfiSysPartition();
    return EFI_SUCCESS;
  }

  if (Argv[1][0] == L'-') {
    Print(L"CapsuleApp: Unrecognized option(%s).\n", Argv[1]);
    return EFI_UNSUPPORTED;
  }

  CapsuleFirstIndex = 1;
  NoReset = FALSE;
  CapsuleOnDisk = FALSE;
  ParaOdIndex = 0;
  ParaNrIndex = 0;

  for (Index = 1; Index < Argc; Index++) {
    if (StrCmp(Argv[Index], L"-OD") == 0) {
      ParaOdIndex = Index;
      CapsuleOnDisk = TRUE;
    } else if (StrCmp(Argv[Index], L"-NR") == 0) {
      ParaNrIndex = Index;
      NoReset = TRUE;
    }
  }

  if (ParaOdIndex != 0) {
    if (ParaOdIndex == Argc - 1) {
      MapFsStr = NULL;
    } else if (ParaOdIndex == Argc - 2) {
      MapFsStr = Argv[Argc-1];
    } else {
      Print (L"CapsuleApp: Invalid Position for -OD Options\n");
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    if (ParaNrIndex != 0) {
      if (ParaNrIndex + 1 == ParaOdIndex) {
        CapsuleLastIndex = ParaNrIndex - 1;
      } else {
        Print (L"CapsuleApp: Invalid Position for -NR Options\n");
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }
    } else {
      CapsuleLastIndex = ParaOdIndex - 1;
    }
  } else {
    if (ParaNrIndex != 0) {
      if (ParaNrIndex == Argc -1) {
        CapsuleLastIndex = ParaNrIndex - 1;
      } else {
        Print (L"CapsuleApp: Invalid Position for -NR Options\n");
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }
    } else {
      CapsuleLastIndex = Argc - 1;
    }
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
  ZeroMem(&CapsuleBufferSize, sizeof(CapsuleBufferSize));
  BlockDescriptors = NULL;

  for (Index = 0; Index < CapsuleNum; Index++) {
    CapsuleName = Argv[CapsuleFirstIndex + Index];
    Status = ReadFileToBuffer(CapsuleName, &CapsuleBufferSize[Index], &CapsuleBuffer[Index]);
    if (EFI_ERROR(Status)) {
      Print(L"CapsuleApp: capsule image (%s) is not found.\n", CapsuleName);
      goto Done;
    }
    if (!IsValidCapsuleHeader (CapsuleBuffer[Index], CapsuleBufferSize[Index])) {
      Print(L"CapsuleApp: Capsule image (%s) is not a valid capsule.\n", CapsuleName);
      return EFI_INVALID_PARAMETER;
    }
    CapsuleNames[Index] = CapsuleName;
  }

  //
  // Every capsule use 2 descriptor 1 for data 1 for end
  //
  Status = BuildGatherList(CapsuleBuffer, CapsuleBufferSize, CapsuleNum, &BlockDescriptors);
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
    if (CapsuleBufferSize[Index] > MaxCapsuleSize) {
      Print (L"CapsuleApp: capsule is too large to update, %ld is allowed\n", MaxCapsuleSize);
      Status = EFI_UNSUPPORTED;
      goto Done;
    }
  }

  //
  // Check whether is capsule on disk.
  //
  if (CapsuleOnDisk) {
    Status = ProcessCapsuleOnDisk (CapsuleBuffer, CapsuleBufferSize, CapsuleNames, MapFsStr, CapsuleNum);
    if (Status != EFI_SUCCESS) {
      Print (L"CapsuleApp: failed to update capsule - %r\n", Status);
      goto Done;
    } else {
      if (!NoReset) {
        gRT->ResetSystem (ResetType, EFI_SUCCESS, 0, NULL);
      } else {
        goto Done;
      }
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
