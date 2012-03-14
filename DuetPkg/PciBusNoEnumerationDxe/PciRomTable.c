/*++

Copyright (c) 2005 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciRomTable.c
  
Abstract:

  Option Rom Support for PCI Bus Driver

Revision History

--*/

#include "PciBus.h"

typedef struct {
  EFI_HANDLE  ImageHandle;
  UINTN       Seg;
  UINT8       Bus;
  UINT8       Dev;
  UINT8       Func;
} EFI_PCI_ROM_IMAGE_MAPPING;

UINTN                      mNumberOfPciRomImages     = 0;
UINTN                      mMaxNumberOfPciRomImages  = 0;
EFI_PCI_ROM_IMAGE_MAPPING  *mRomImageTable           = NULL;

CHAR16 mHexDigit[17] = L"0123456789ABCDEF";

VOID
PciRomAddImageMapping (
  IN EFI_HANDLE  ImageHandle,
  IN UINTN       Seg,
  IN UINT8       Bus,
  IN UINT8       Dev,
  IN UINT8       Func
  )

{
  EFI_PCI_ROM_IMAGE_MAPPING *TempMapping;

  if (mNumberOfPciRomImages >= mMaxNumberOfPciRomImages) {

    mMaxNumberOfPciRomImages += 0x20;

    TempMapping = NULL;
    TempMapping = AllocatePool (mMaxNumberOfPciRomImages * sizeof (EFI_PCI_ROM_IMAGE_MAPPING));
    if (TempMapping == NULL) {
      return ;
    }

    CopyMem (TempMapping, mRomImageTable, mNumberOfPciRomImages * sizeof (EFI_PCI_ROM_IMAGE_MAPPING));

    if (mRomImageTable != NULL) {
      gBS->FreePool (mRomImageTable);
    }

    mRomImageTable = TempMapping;
  }

  mRomImageTable[mNumberOfPciRomImages].ImageHandle = ImageHandle;
  mRomImageTable[mNumberOfPciRomImages].Seg         = Seg;
  mRomImageTable[mNumberOfPciRomImages].Bus         = Bus;
  mRomImageTable[mNumberOfPciRomImages].Dev         = Dev;
  mRomImageTable[mNumberOfPciRomImages].Func        = Func;
  mNumberOfPciRomImages++;
}

VOID
HexToString (
  CHAR16  *String,
  UINTN   Value,
  UINTN   Digits
  )

{
  for (; Digits > 0; Digits--, String++) {
    *String = mHexDigit[((Value >> (4*(Digits-1))) & 0x0f)];
  }
}

EFI_STATUS
PciRomLoadEfiDriversFromRomImage (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_PCI_OPTION_ROM_DESCRIPTOR  *PciOptionRomDescriptor
  )
/*++

Routine Description:
  Command entry point. 

Arguments:
  ImageHandle     The image handle. 
  SystemTable     The system table.

Returns:
  EFI_SUCCESS             - The command completed successfully
  EFI_INVALID_PARAMETER   - Command usage error
  EFI_UNSUPPORTED         - Protocols unsupported
  EFI_OUT_OF_RESOURCES    - Out of memory
  Other value             - Unknown error

--*/
{
  VOID                          *RomBar;
  UINTN                         RomSize;
  CHAR16                        *FileName;
  EFI_PCI_EXPANSION_ROM_HEADER  *EfiRomHeader;
  PCI_DATA_STRUCTURE            *Pcir;
  UINTN                         ImageIndex;
  UINTN                         RomBarOffset;
  UINT32                        ImageSize;
  UINT16                        ImageOffset;
  EFI_HANDLE                    ImageHandle;
  EFI_STATUS                    Status;
  EFI_STATUS                    retStatus;
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

  RomBar = (VOID *) (UINTN) PciOptionRomDescriptor->RomAddress;
  RomSize = (UINTN) PciOptionRomDescriptor->RomLength;
  FileName = L"PciRom Seg=00000000 Bus=00 Dev=00 Func=00 Image=0000";

  HexToString (&FileName[11], PciOptionRomDescriptor->Seg, 8);
  HexToString (&FileName[24], PciOptionRomDescriptor->Bus, 2);
  HexToString (&FileName[31], PciOptionRomDescriptor->Dev, 2);
  HexToString (&FileName[39], PciOptionRomDescriptor->Func, 2);

  ImageIndex    = 0;
  retStatus     = EFI_NOT_FOUND;
  RomBarOffset  = (UINTN) RomBar;

  do {

    EfiRomHeader = (EFI_PCI_EXPANSION_ROM_HEADER *) (UINTN) RomBarOffset;


    if (EfiRomHeader->Signature != PCI_EXPANSION_ROM_HEADER_SIGNATURE) {
      return retStatus;
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
          Status = gBS->LocateProtocol (&gEfiDecompressProtocolGuid, NULL, (VOID **) &Decompress);
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
              DecompressedImageBuffer = NULL;
              DecompressedImageBuffer = AllocatePool (DestinationSize);
              if (DecompressedImageBuffer != NULL) {
                Scratch = AllocatePool (ScratchSize);
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

                  gBS->FreePool (Scratch);
                }
              }
            }
          }
        }

        if (!SkipImage) {

          //
          // load image and start image
          //

          HexToString (&FileName[48], ImageIndex, 4);
          FilePath = FileDevicePath (NULL, FileName);

          Status = gBS->LoadImage (
                          FALSE,
                          This->ImageHandle,
                          FilePath,
                          ImageBuffer,
                          ImageLength,
                          &ImageHandle
                          );
          if (!EFI_ERROR (Status)) {
            Status = gBS->StartImage (ImageHandle, NULL, NULL);
            if (!EFI_ERROR (Status)) {
              PciRomAddImageMapping (
                ImageHandle,
                PciOptionRomDescriptor->Seg,
                PciOptionRomDescriptor->Bus,
                PciOptionRomDescriptor->Dev,
                PciOptionRomDescriptor->Func
                );
              retStatus = Status;
            }
          }
          if (FilePath != NULL) {
            gBS->FreePool (FilePath);
          }
        }

        if (DecompressedImageBuffer != NULL) {
          gBS->FreePool (DecompressedImageBuffer);
        }

      }
    }

    RomBarOffset = RomBarOffset + ImageSize;
    ImageIndex++;
  } while (((Pcir->Indicator & 0x80) == 0x00) && ((RomBarOffset - (UINTN) RomBar) < RomSize));

  return retStatus;
}

EFI_STATUS
PciRomLoadEfiDriversFromOptionRomTable (
  IN EFI_DRIVER_BINDING_PROTOCOL      *This,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  EFI_STATUS                        Status;
  EFI_PCI_OPTION_ROM_TABLE          *PciOptionRomTable;
  EFI_PCI_OPTION_ROM_DESCRIPTOR     *PciOptionRomDescriptor;
  UINTN                             Index;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptors;
  UINT16                            MinBus;
  UINT16                            MaxBus;

  Status = EfiGetSystemConfigurationTable (&gEfiPciOptionRomTableGuid, (VOID **) &PciOptionRomTable);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Status = EFI_NOT_FOUND;

  for (Index = 0; Index < PciOptionRomTable->PciOptionRomCount; Index++) {
    PciOptionRomDescriptor = &PciOptionRomTable->PciOptionRomDescriptors[Index];
    if (!PciOptionRomDescriptor->DontLoadEfiRom) {
      if (PciOptionRomDescriptor->Seg == PciRootBridgeIo->SegmentNumber) {
        Status = PciRootBridgeIo->Configuration (PciRootBridgeIo, (VOID **) &Descriptors);
        if (EFI_ERROR (Status)) {
          return Status;
        }

        PciGetBusRange (&Descriptors, &MinBus, &MaxBus, NULL);
        if ((MinBus <= PciOptionRomDescriptor->Bus) && (PciOptionRomDescriptor->Bus <= MaxBus)) {
          Status = PciRomLoadEfiDriversFromRomImage (This, PciOptionRomDescriptor);
          PciOptionRomDescriptor->DontLoadEfiRom |= 2;
        }
      }
    }
  }

  return Status;
}

EFI_STATUS
PciRomGetRomResourceFromPciOptionRomTable (
  IN EFI_DRIVER_BINDING_PROTOCOL      *This,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  PCI_IO_DEVICE                       *PciIoDevice
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  EFI_STATUS                    Status;
  EFI_PCI_OPTION_ROM_TABLE      *PciOptionRomTable;
  EFI_PCI_OPTION_ROM_DESCRIPTOR *PciOptionRomDescriptor;
  UINTN                         Index;

  Status = EfiGetSystemConfigurationTable (&gEfiPciOptionRomTableGuid, (VOID **) &PciOptionRomTable);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < PciOptionRomTable->PciOptionRomCount; Index++) {
    PciOptionRomDescriptor = &PciOptionRomTable->PciOptionRomDescriptors[Index];
    if (PciOptionRomDescriptor->Seg == PciRootBridgeIo->SegmentNumber &&
        PciOptionRomDescriptor->Bus == PciIoDevice->BusNumber         &&
        PciOptionRomDescriptor->Dev == PciIoDevice->DeviceNumber      &&
        PciOptionRomDescriptor->Func == PciIoDevice->FunctionNumber ) {

      PciIoDevice->PciIo.RomImage = (VOID *) (UINTN) PciOptionRomDescriptor->RomAddress;
      PciIoDevice->PciIo.RomSize  = (UINTN) PciOptionRomDescriptor->RomLength;
    }
  }

  for (Index = 0; Index < mNumberOfPciRomImages; Index++) {
    if (mRomImageTable[Index].Seg  == PciRootBridgeIo->SegmentNumber &&
        mRomImageTable[Index].Bus  == PciIoDevice->BusNumber         &&
        mRomImageTable[Index].Dev  == PciIoDevice->DeviceNumber      &&
        mRomImageTable[Index].Func == PciIoDevice->FunctionNumber    ) {

      AddDriver (PciIoDevice, mRomImageTable[Index].ImageHandle);
    }
  }

  return EFI_SUCCESS;
}
