/** @file
  Main file for LoadPciRom shell Debug1 function.

  Copyright (c) 2005 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDebug1CommandsLib.h"
#include <IndustryStandard/Pci22.h>
#include <IndustryStandard/Pci23.h>
#include <IndustryStandard/PeImage.h>
#include <Protocol/Decompress.h>

/**
  Connects all available drives and controllers.

  @retval EFI_SUCCESS     The operation was successful.
  @retval EFI_ABORTED     The abort mechanism was received.
**/
EFI_STATUS
EFIAPI
LoadPciRomConnectAllDriversToAllControllers (
  VOID
  );

/**
  Command entry point.

  @param[in] RomBar       The Rom Base address.
  @param[in] RomSize      The Rom size.
  @param[in] FileName     The file name.

  @retval EFI_SUCCESS             The command completed successfully.
  @retval EFI_INVALID_PARAMETER   Command usage error.
  @retval EFI_UNSUPPORTED         Protocols unsupported.
  @retval EFI_OUT_OF_RESOURCES    Out of memory.
  @retval Other value             Unknown error.
**/
EFI_STATUS
EFIAPI
LoadEfiDriversFromRomImage (
  VOID                      *RomBar,
  UINTN                     RomSize,
  CONST CHAR16              *FileName
  );

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-nc", TypeFlag},
  {NULL, TypeMax}
  };

/**
  Function for 'loadpcirom' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunLoadPciRom (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_SHELL_FILE_INFO     *FileList;
  UINTN                   SourceSize;
  UINT8                   *File1Buffer;
  EFI_STATUS              Status;
  LIST_ENTRY              *Package;
  CHAR16                  *ProblemParam;
  SHELL_STATUS            ShellStatus;
  BOOLEAN                 Connect;
  CONST CHAR16            *Param;
  UINTN                   ParamCount;
  EFI_SHELL_FILE_INFO     *Node;
  //
  // Local variable initializations
  //
  File1Buffer = NULL;
  ShellStatus = SHELL_SUCCESS;
  FileList    = NULL;


  //
  // verify number of arguments
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount(Package) < 2) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      if (ShellCommandLineGetFlag(Package, L"-nc")) {
        Connect = FALSE;
      } else {
        Connect = TRUE;
      }

      //
      // get a list with each file specified by parameters
      // if parameter is a directory then add all the files below it to the list
      //
      for ( ParamCount = 1, Param = ShellCommandLineGetRawValue(Package, ParamCount)
          ; Param != NULL
          ; ParamCount++, Param = ShellCommandLineGetRawValue(Package, ParamCount)
         ){
        Status = ShellOpenFileMetaArg((CHAR16*)Param, EFI_FILE_MODE_WRITE|EFI_FILE_MODE_READ, &FileList);
        if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_OPEN_FAIL), gShellDebug1HiiHandle, Param, Status);
          ShellStatus = SHELL_ACCESS_DENIED;
          break;
        }
      }
      if (ShellStatus == SHELL_SUCCESS  && FileList != NULL) {
        //
        // loop through the list and make sure we are not aborting...
        //
        for ( Node = (EFI_SHELL_FILE_INFO*)GetFirstNode(&FileList->Link)
            ; !IsNull(&FileList->Link, &Node->Link) && !ShellGetExecutionBreakFlag()
            ; Node = (EFI_SHELL_FILE_INFO*)GetNextNode(&FileList->Link, &Node->Link)
           ){
          if (EFI_ERROR(Node->Status)){
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_OPEN_FAIL), gShellDebug1HiiHandle, Node->FullName);
            ShellStatus = SHELL_INVALID_PARAMETER;
            continue;
          }
          if (FileHandleIsDirectory(Node->Handle) == EFI_SUCCESS) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_NOT_DIR), gShellDebug1HiiHandle, Node->FullName);
            ShellStatus = SHELL_INVALID_PARAMETER;
            continue;
          }
          SourceSize  = (UINTN) Node->Info->FileSize;
          File1Buffer = AllocateZeroPool (SourceSize);
          ASSERT(File1Buffer != NULL);
          Status = gEfiShellProtocol->ReadFile(Node->Handle, &SourceSize, File1Buffer);
          if (EFI_ERROR(Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_READ_FAIL), gShellDebug1HiiHandle, Node->FullName);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            Status = LoadEfiDriversFromRomImage (
                      File1Buffer,
                      SourceSize,
                      Node->FullName
                     );

            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_LOAD_PCI_ROM_RES), gShellDebug1HiiHandle, Node->FullName, Status);
          }
          FreePool(File1Buffer);
        }
      } else if (ShellStatus == SHELL_SUCCESS) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_NOT_SPEC), gShellDebug1HiiHandle);
        ShellStatus = SHELL_NOT_FOUND;
      }
      if (FileList != NULL && !IsListEmpty(&FileList->Link)) {
        Status = ShellCloseFileMetaArg(&FileList);
      }
      FileList = NULL;

      if (Connect) {
        Status = LoadPciRomConnectAllDriversToAllControllers ();
      }
    }
  }

  return (ShellStatus);
}

/**
  Command entry point.

  @param[in] RomBar       The Rom Base address.
  @param[in] RomSize      The Rom size.
  @param[in] FileName     The file name.

  @retval EFI_SUCCESS             The command completed successfully.
  @retval EFI_INVALID_PARAMETER   Command usage error.
  @retval EFI_UNSUPPORTED         Protocols unsupported.
  @retval EFI_OUT_OF_RESOURCES    Out of memory.
  @retval Other value             Unknown error.
**/
EFI_STATUS
EFIAPI
LoadEfiDriversFromRomImage (
  VOID                      *RomBar,
  UINTN                     RomSize,
  CONST CHAR16              *FileName
  )

{
  EFI_PCI_EXPANSION_ROM_HEADER  *EfiRomHeader;
  PCI_DATA_STRUCTURE            *Pcir;
  UINTN                         ImageIndex;
  UINTN                         RomBarOffset;
  UINT32                        ImageSize;
  UINT16                        ImageOffset;
  EFI_HANDLE                    ImageHandle;
  EFI_STATUS                    Status;
  EFI_STATUS                    ReturnStatus;
  CHAR16                        RomFileName[280];
  EFI_DEVICE_PATH_PROTOCOL      *FilePath;
  BOOLEAN                       SkipImage;
  UINT32                        DestinationSize;
  UINT32                        ScratchSize;
  UINT8                         *Scratch;
  VOID                          *ImageBuffer;
  VOID                          *DecompressedImageBuffer;
  UINT32                        ImageLength;
  EFI_DECOMPRESS_PROTOCOL       *Decompress;
  UINT32                        InitializationSize;

  ImageIndex    = 0;
  ReturnStatus     = EFI_NOT_FOUND;
  RomBarOffset  = (UINTN) RomBar;

  do {

    EfiRomHeader = (EFI_PCI_EXPANSION_ROM_HEADER *) (UINTN) RomBarOffset;

    if (EfiRomHeader->Signature != PCI_EXPANSION_ROM_HEADER_SIGNATURE) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_LOADPCIROM_CORRUPT), gShellDebug1HiiHandle, FileName, ImageIndex);
//      PrintToken (STRING_TOKEN (STR_LOADPCIROM_IMAGE_CORRUPT), HiiHandle, ImageIndex);
      return ReturnStatus;
    }

    //
    // If the pointer to the PCI Data Structure is invalid, no further images can be located. 
    // The PCI Data Structure must be DWORD aligned. 
    //
    if (EfiRomHeader->PcirOffset == 0 ||
        (EfiRomHeader->PcirOffset & 3) != 0 ||
        RomBarOffset - (UINTN)RomBar + EfiRomHeader->PcirOffset + sizeof (PCI_DATA_STRUCTURE) > RomSize) {
      break;
    }

    Pcir      = (PCI_DATA_STRUCTURE *) (UINTN) (RomBarOffset + EfiRomHeader->PcirOffset);
    //
    // If a valid signature is not present in the PCI Data Structure, no further images can be located.
    //
    if (Pcir->Signature != PCI_DATA_STRUCTURE_SIGNATURE) {
      break;
    }
    ImageSize = Pcir->ImageLength * 512;
    if (RomBarOffset - (UINTN)RomBar + ImageSize > RomSize) {
      break;
    }

    if ((Pcir->CodeType == PCI_CODE_TYPE_EFI_IMAGE) &&
        (EfiRomHeader->EfiSignature == EFI_PCI_EXPANSION_ROM_HEADER_EFISIGNATURE) &&
        ((EfiRomHeader->EfiSubsystem == EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER) ||
         (EfiRomHeader->EfiSubsystem == EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER))) {

      ImageOffset             = EfiRomHeader->EfiImageHeaderOffset;
      InitializationSize      = EfiRomHeader->InitializationSize * 512;

      if (InitializationSize <= ImageSize && ImageOffset < InitializationSize) {

        ImageBuffer             = (VOID *) (UINTN) (RomBarOffset + ImageOffset);
        ImageLength             = InitializationSize - ImageOffset;
        DecompressedImageBuffer = NULL;

        //
        // decompress here if needed
        //
        SkipImage = FALSE;
        if (EfiRomHeader->CompressionType > EFI_PCI_EXPANSION_ROM_HEADER_COMPRESSED) {
          SkipImage = TRUE;
        }

        if (EfiRomHeader->CompressionType == EFI_PCI_EXPANSION_ROM_HEADER_COMPRESSED) {
          Status = gBS->LocateProtocol (&gEfiDecompressProtocolGuid, NULL, (VOID**)&Decompress);
          ASSERT_EFI_ERROR(Status);
          if (EFI_ERROR (Status)) {
            SkipImage = TRUE;
          } else {
            SkipImage = TRUE;
            Status = Decompress->GetInfo (
                                  Decompress,
                                  ImageBuffer,
                                  ImageLength,
                                  &DestinationSize,
                                  &ScratchSize
                                 );
            if (!EFI_ERROR (Status)) {
              DecompressedImageBuffer = AllocateZeroPool (DestinationSize);
              if (ImageBuffer != NULL) {
                Scratch = AllocateZeroPool (ScratchSize);
                if (Scratch != NULL) {
                  Status = Decompress->Decompress (
                                        Decompress,
                                        ImageBuffer,
                                        ImageLength,
                                        DecompressedImageBuffer,
                                        DestinationSize,
                                        Scratch,
                                        ScratchSize
                                       );
                  if (!EFI_ERROR (Status)) {
                    ImageBuffer = DecompressedImageBuffer;
                    ImageLength = DestinationSize;
                    SkipImage   = FALSE;
                  }

                  FreePool (Scratch);
                }
              }
            }
          }
        }

        if (!SkipImage) {
          //
          // load image and start image
          //
          UnicodeSPrint (RomFileName, sizeof (RomFileName), L"%s[%d]", FileName, ImageIndex);
          FilePath = FileDevicePath (NULL, RomFileName);

          Status = gBS->LoadImage (
                        TRUE,
                        gImageHandle,
                        FilePath,
                        ImageBuffer,
                        ImageLength,
                        &ImageHandle
                       );
          if (EFI_ERROR (Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_LOADPCIROM_LOAD_FAIL), gShellDebug1HiiHandle, FileName, ImageIndex, Status);
//            PrintToken (STRING_TOKEN (STR_LOADPCIROM_LOAD_IMAGE_ERROR), HiiHandle, ImageIndex, Status);
          } else {
            Status = gBS->StartImage (ImageHandle, NULL, NULL);
            if (EFI_ERROR (Status)) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_LOADPCIROM_START_FAIL), gShellDebug1HiiHandle, FileName, ImageIndex, Status);
//              PrintToken (STRING_TOKEN (STR_LOADPCIROM_START_IMAGE), HiiHandle, ImageIndex, Status);
            } else {
              ReturnStatus = Status;
            }
          }
        }

        if (DecompressedImageBuffer != NULL) {
          FreePool (DecompressedImageBuffer);
        }

      }
    }

    RomBarOffset = RomBarOffset + ImageSize;
    ImageIndex++;
  } while (((Pcir->Indicator & 0x80) == 0x00) && ((RomBarOffset - (UINTN) RomBar) < RomSize));

  return ReturnStatus;
}

/**
  Connects all available drives and controllers.

  @retval EFI_SUCCESS     The operation was successful.
  @retval EFI_ABORTED     The abort mechanism was received.
**/
EFI_STATUS
EFIAPI
LoadPciRomConnectAllDriversToAllControllers (
  VOID
  )

{
  EFI_STATUS  Status;
  UINTN       AllHandleCount;
  EFI_HANDLE  *AllHandleBuffer;
  UINTN       Index;
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuffer;
  UINTN       *HandleType;
  UINTN       HandleIndex;
  BOOLEAN     Parent;
  BOOLEAN     Device;

  Status = gBS->LocateHandleBuffer(
            AllHandles,
            NULL,
            NULL,
            &AllHandleCount,
            &AllHandleBuffer
           );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < AllHandleCount; Index++) {
    if (ShellGetExecutionBreakFlag ()) {
      Status = EFI_ABORTED;
      goto Done;
    }
    //
    // Scan the handle database
    //
    Status = ParseHandleDatabaseByRelationshipWithType(
      NULL,
      AllHandleBuffer[Index],
      &HandleCount,
      &HandleBuffer,
      &HandleType
     );
/*
    Status = LibScanHandleDatabase (
              NULL,
              NULL,
              AllHandleBuffer[Index],
              NULL,
              &HandleCount,
              &HandleBuffer,
              &HandleType
             );
*/
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    Device = TRUE;
    if ((HandleType[Index] & HR_DRIVER_BINDING_HANDLE) != 0) {
      Device = FALSE;
    }

    if ((HandleType[Index] & HR_IMAGE_HANDLE) != 0) {
      Device = FALSE;
    }

    if (Device) {
      Parent = FALSE;
      for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
        if ((HandleType[HandleIndex] & HR_PARENT_HANDLE) != 0) {
          Parent = TRUE;
        }
      }

      if (!Parent) {
        if ((HandleType[Index] & HR_DEVICE_HANDLE) != 0) {
          Status = gBS->ConnectController (
                        AllHandleBuffer[Index],
                        NULL,
                        NULL,
                        TRUE
                       );
        }
      }
    }

    FreePool (HandleBuffer);
    FreePool (HandleType);
  }

Done:
  FreePool (AllHandleBuffer);
  return Status;
}
