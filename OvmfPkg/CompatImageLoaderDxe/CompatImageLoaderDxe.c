/** @file
 *  PE/COFF emulator protocol implementation to start Linux kernel
 *  images from non-native firmware
 *
 *  Copyright (c) 2020, ARM Ltd. All rights reserved.<BR>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 */

#include <PiDxe.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PeCoffLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/PeCoffImageEmulator.h>

#pragma pack (1)
typedef struct {
  UINT8     Type;
  UINT8     Size;
  UINT16    MachineType;
  UINT32    EntryPoint;
} PE_COMPAT_TYPE1;
#pragma pack ()

STATIC
BOOLEAN
EFIAPI
IsImageSupported (
  IN  EDKII_PECOFF_IMAGE_EMULATOR_PROTOCOL  *This,
  IN  UINT16                                ImageType,
  IN  EFI_DEVICE_PATH_PROTOCOL              *DevicePath   OPTIONAL
  )
{
  return ImageType == EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION;
}

STATIC
EFI_IMAGE_ENTRY_POINT
EFIAPI
GetCompatEntryPoint (
  IN  EFI_PHYSICAL_ADDRESS  ImageBase
  )
{
  EFI_IMAGE_DOS_HEADER      *DosHdr;
  UINTN                     PeCoffHeaderOffset;
  EFI_IMAGE_NT_HEADERS32    *Pe32;
  EFI_IMAGE_SECTION_HEADER  *Section;
  UINTN                     NumberOfSections;
  PE_COMPAT_TYPE1           *PeCompat;
  UINTN                     PeCompatEnd;

  DosHdr = (EFI_IMAGE_DOS_HEADER *)(UINTN)ImageBase;
  if (DosHdr->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
    return NULL;
  }

  PeCoffHeaderOffset = DosHdr->e_lfanew;
  Pe32               = (EFI_IMAGE_NT_HEADERS32 *)((UINTN)ImageBase + PeCoffHeaderOffset);

  Section = (EFI_IMAGE_SECTION_HEADER *)((UINTN)&Pe32->OptionalHeader +
                                         Pe32->FileHeader.SizeOfOptionalHeader);
  NumberOfSections = (UINTN)Pe32->FileHeader.NumberOfSections;

  while (NumberOfSections--) {
    if (!CompareMem (Section->Name, ".compat", sizeof (Section->Name))) {
      //
      // Dereference the section contents to find the mixed mode entry point
      //
      PeCompat    = (PE_COMPAT_TYPE1 *)((UINTN)ImageBase + Section->VirtualAddress);
      PeCompatEnd = (UINTN)(VOID *)PeCompat + Section->Misc.VirtualSize;

      while (PeCompat->Type != 0 && (UINTN)(VOID *)PeCompat < PeCompatEnd) {
        if ((PeCompat->Type == 1) &&
            (PeCompat->Size >= sizeof (PE_COMPAT_TYPE1)) &&
            EFI_IMAGE_MACHINE_TYPE_SUPPORTED (PeCompat->MachineType))
        {
          return (EFI_IMAGE_ENTRY_POINT)((UINTN)ImageBase + PeCompat->EntryPoint);
        }

        PeCompat = (PE_COMPAT_TYPE1 *)((UINTN)PeCompat + PeCompat->Size);
        ASSERT ((UINTN)(VOID *)PeCompat < PeCompatEnd);
      }
    }

    Section++;
  }

  return NULL;
}

STATIC
EFI_STATUS
EFIAPI
RegisterImage (
  IN      EDKII_PECOFF_IMAGE_EMULATOR_PROTOCOL  *This,
  IN      EFI_PHYSICAL_ADDRESS                  ImageBase,
  IN      UINT64                                ImageSize,
  IN  OUT EFI_IMAGE_ENTRY_POINT                 *EntryPoint
  )
{
  EFI_IMAGE_ENTRY_POINT  CompatEntryPoint;

  CompatEntryPoint = GetCompatEntryPoint (ImageBase);
  if (CompatEntryPoint == NULL) {
    return EFI_UNSUPPORTED;
  }

  *EntryPoint = CompatEntryPoint;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
UnregisterImage (
  IN  EDKII_PECOFF_IMAGE_EMULATOR_PROTOCOL  *This,
  IN  EFI_PHYSICAL_ADDRESS                  ImageBase
  )
{
  return EFI_SUCCESS;
}

STATIC EDKII_PECOFF_IMAGE_EMULATOR_PROTOCOL  mCompatLoaderPeCoffEmuProtocol = {
  IsImageSupported,
  RegisterImage,
  UnregisterImage,
  EDKII_PECOFF_IMAGE_EMULATOR_VERSION,
  EFI_IMAGE_MACHINE_X64
};

EFI_STATUS
EFIAPI
CompatImageLoaderDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return gBS->InstallProtocolInterface (
                &ImageHandle,
                &gEdkiiPeCoffImageEmulatorProtocolGuid,
                EFI_NATIVE_INTERFACE,
                &mCompatLoaderPeCoffEmuProtocol
                );
}
