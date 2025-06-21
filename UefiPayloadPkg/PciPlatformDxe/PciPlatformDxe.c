/** @file
  Implementation of the gEfiPciPlatformProtocol to support loading
  PCI Option ROMs when full PCI enumeration is skipped.

Copyright (c) 2025, 9elements GmbH. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "PciPlatformDxe.h"
#include <Bus/Pci/PciBusDxe/PciBus.h>
#include <Bus/Pci/PciBusDxe/PciOptionRomSupport.h>

EFI_STATUS
EFIAPI
PciPlatformNotify (
  IN EFI_PCI_PLATFORM_PROTOCOL                      *This,
  IN EFI_HANDLE                                     HostBridge,
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE  Phase,
  IN EFI_PCI_EXECUTION_PHASE                        ExecPhase
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
PciPlatformPrepController (
  IN EFI_PCI_PLATFORM_PROTOCOL                     *This,
  IN EFI_HANDLE                                    HostBridge,
  IN EFI_HANDLE                                    RootBridge,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS   PciAddress,
  IN EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE  Phase,
  IN EFI_PCI_EXECUTION_PHASE                       ExecPhase
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Gets the PCI device's option ROM from a platform-specific location.

  The GetPciRom() function gets the PCI device's option ROM from a platform-specific location.
  The option ROM will be loaded into memory. This member function is used to return an image
  that is packaged as a PCI 2.2 option ROM. The image may contain both legacy and EFI option
  ROMs. See the UEFI 2.0 Specification for details. This member function can be used to return
  option ROM images for embedded controllers. Option ROMs for embedded controllers are typically
  stored in platform-specific storage, and this member function can retrieve it from that storage
  and return it to the PCI bus driver. The PCI bus driver will call this member function before
  scanning the ROM that is attached to any controller, which allows to scan the PCI card for
  ROM image and retrieve it.

  @param[in]  This        The pointer to the EFI_PCI_PLATFORM_PROTOCOL instance.
  @param[in]  PciHandle   The handle of the PCI device.
  @param[out] RomImage    If the call succeeds, the pointer to the pointer to the option ROM image.
                          Otherwise, this field is undefined. The memory for RomImage is allocated
                          by EFI_PCI_PLATFORM_PROTOCOL.GetPciRom() using the EFI Boot Service AllocatePool().
                          It is the caller's responsibility to free the memory using the EFI Boot Service
                          FreePool(), when the caller is done with the option ROM.
  @param[out] RomSize     If the call succeeds, a pointer to the size of the option ROM size. Otherwise,
                          this field is undefined.

  @retval EFI_SUCCESS            The option ROM was available for this device and loaded into memory.
  @retval EFI_NOT_FOUND          No option ROM was available for this device.
  @retval EFI_OUT_OF_RESOURCES   No memory was available to load the option ROM.
  @retval EFI_DEVICE_ERROR       An error occurred in obtaining the option ROM.

**/
EFI_STATUS
EFIAPI
PciGetPciRom (
  IN  CONST EFI_PCI_PLATFORM_PROTOCOL  *This,
  IN        EFI_HANDLE                 PciHandle,
  OUT       VOID                       **RomImage,
  OUT       UINTN                      *RomSize
  )
{
  EFI_STATUS                Status;
  IN EFI_PCI_IO_PROTOCOL    *PciIo;
  UINTN                     PciSegment;
  UINTN                     PciBus;
  UINTN                     PciDevice;
  UINTN                     PciFunction;
  UINTN                     RomBarIndex;
  UINT32                    Buffer;
  UINT32                    AllOnes;
  PCI_IO_DEVICE             *PciIoDevice;
  UINT8                     Indicator;
  UINT16                    OffsetPcir;
  UINT32                    RomBarOffset;
  UINT32                    RomBar;
  BOOLEAN                   FirstCheck;
  UINT64                    RomImageSize;
  UINT32                    LegacyImageLength;
  UINT8                     *RomInMemory;
  UINT8                     CodeType;
  PCI_EXPANSION_ROM_HEADER  RomHeader;
  PCI_DATA_STRUCTURE        RomPcir;

  //
  // Check to see if RomImage is NULL
  //
  if (RomImage == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check to see if RomSize is NULL
  //
  if (RomSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *RomImage = NULL;
  *RomSize  = 0;

  Status = gBS->HandleProtocol (
                  PciHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to open gEfiPciIoProtocolGuid\n", __func__));
    return EFI_UNSUPPORTED;
  }

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (PciIo);

  //
  // Get the location of the PCI device
  //
  PciIo->GetLocation (
           PciIo,
           &PciSegment,
           &PciBus,
           &PciDevice,
           &PciFunction
           );

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: PCI @ [%02x|%02x|%02x|%02x] Searching Option ROM...\n",
    __func__,
    PciSegment,
    PciBus,
    PciDevice,
    PciFunction
    ));

  //
  // Default ROM is at 0x30
  //
  RomBarIndex = PCI_EXPANSION_ROM_BASE;

  if (IS_PCI_BRIDGE (&PciIoDevice->Pci)) {
    //
    // If is ppb ROM is at 0x38
    //
    RomBarIndex = PCI_BRIDGE_ROMBAR;
  }

  //
  // Backup ROM BAR
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciWidthUint32,
                        RomBarIndex,
                        1,
                        &Buffer
                        );
  if (EFI_ERROR (Status)) {
    goto CloseAndReturn;
  }

  //
  // The bit0 is 0 to prevent the enabling of the Rom address decoder
  //
  AllOnes = 0xfffffffe;

  Status = PciIo->Pci.Write (
                        PciIo,
                        EfiPciWidthUint32,
                        RomBarIndex,
                        1,
                        &AllOnes
                        );
  if (EFI_ERROR (Status)) {
    goto CloseAndReturn;
  }

  //
  // Read back
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciWidthUint32,
                        RomBarIndex,
                        1,
                        &AllOnes
                        );
  if (EFI_ERROR (Status)) {
    goto CloseAndReturn;
  }

  //
  // Bits [1, 10] are reserved
  //
  AllOnes &= 0xFFFFF800;
  if ((AllOnes == 0) || (AllOnes == 0xFFFFF800)) {
    DEBUG ((DEBUG_VERBOSE, "%a: No Option ROM found\n", __func__));
    return EFI_NOT_FOUND;
  }

  *RomSize = (~AllOnes) + 1;

  //
  // Restore BAR and enable it
  //
  RomBar = Buffer | 1;
  Status = PciIo->Pci.Write (
                        PciIo,
                        EfiPciWidthUint32,
                        RomBarIndex,
                        1,
                        &RomBar
                        );
  if (EFI_ERROR (Status)) {
    goto CloseAndReturn;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: ROM BAR enabled at 0x%x with size %d\n",
    __func__,
    Buffer &~1,
    *RomSize
    ));

  //
  // Set pointer to ROM BAR
  //
  RomBar = Buffer & 0xFFFFF800;

  RomBarOffset      = RomBar;
  FirstCheck        = TRUE;
  LegacyImageLength = 0;
  RomImageSize      = 0;

  //
  // Expect to find a PCI ROM header at offset 0.
  // The ROM header contains a pointer to the PCI Data structure.
  // The PCI Data structure holds the image size.
  // At the end of the image another Option ROM might be found
  // (one for legacy BIOS and one for EFI).
  //
  do {
    Status = PciIoDevice->PciRootBridgeIo->Mem.Read (
                                                 PciIoDevice->PciRootBridgeIo,
                                                 EfiPciWidthUint8,
                                                 RomBarOffset,
                                                 sizeof (PCI_EXPANSION_ROM_HEADER),
                                                 (UINT8 *)&RomHeader
                                                 );
    if (EFI_ERROR (Status)) {
      goto RestoreBar;
    }

    if (RomHeader.Signature != PCI_EXPANSION_ROM_HEADER_SIGNATURE) {
      RomBarOffset = RomBarOffset + 512;
      if (FirstCheck) {
        break;
      } else {
        RomImageSize = RomImageSize + 512;
        continue;
      }
    }

    FirstCheck = FALSE;
    OffsetPcir = RomHeader.PcirOffset;
    //
    // If the pointer to the PCI Data Structure is invalid, no further images can be located.
    // The PCI Data Structure must be DWORD aligned.
    //
    if ((OffsetPcir == 0) ||
        ((OffsetPcir & 3) != 0) ||
        (RomImageSize + OffsetPcir + sizeof (PCI_DATA_STRUCTURE) > *RomSize))
    {
      break;
    }

    Status = PciIoDevice->PciRootBridgeIo->Mem.Read (
                                                 PciIoDevice->PciRootBridgeIo,
                                                 EfiPciWidthUint8,
                                                 RomBarOffset + OffsetPcir,
                                                 sizeof (PCI_DATA_STRUCTURE),
                                                 (UINT8 *)&RomPcir
                                                 );
    if (EFI_ERROR (Status)) {
      goto RestoreBar;
    }

    //
    // If a valid signature is not present in the PCI Data Structure, no further images can be located.
    //
    if (RomPcir.Signature != PCI_DATA_STRUCTURE_SIGNATURE) {
      break;
    }

    if (RomImageSize + RomPcir.ImageLength * 512 > *RomSize) {
      break;
    }

    if (RomPcir.CodeType == PCI_CODE_TYPE_PCAT_IMAGE) {
      CodeType          = PCI_CODE_TYPE_PCAT_IMAGE;
      LegacyImageLength = ((UINT32)((EFI_LEGACY_EXPANSION_ROM_HEADER *)&RomHeader)->Size512) * 512;
    }

    Indicator    = RomPcir.Indicator;
    RomImageSize = RomImageSize + RomPcir.ImageLength * 512;
    RomBarOffset = RomBarOffset + RomPcir.ImageLength * 512;
  } while (((Indicator & 0x80) == 0x00) && ((RomBarOffset - RomBar) < *RomSize));

  //
  // Some Legacy Cards do not report the correct ImageLength so used the maximum
  // of the legacy length and the PCIR Image Length
  //
  if (CodeType == PCI_CODE_TYPE_PCAT_IMAGE) {
    RomImageSize = MAX (RomImageSize, LegacyImageLength);
  }

  if (RomImageSize > 0) {
    //
    // Allocate buffer to return
    //
    RomInMemory = (UINT8 *)AllocatePool ((UINT32)RomImageSize);
    if (RomInMemory == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto RestoreBar;
    }

    //
    // Copy ROM image into memory
    //
    Status = PciIoDevice->PciRootBridgeIo->Mem.Read (
                                                 PciIoDevice->PciRootBridgeIo,
                                                 EfiPciWidthUint8,
                                                 RomBar,
                                                 (UINT32)RomImageSize,
                                                 RomInMemory
                                                 );
    if (EFI_ERROR (Status)) {
      goto RestoreBar;
    }
  } else {
    Status = EFI_NOT_FOUND;
    goto RestoreBar;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: PCI @ [%02x|%02x|%02x|%02x]: Found Option ROM at 0x%x, length 0x%x\n",
    __func__,
    PciSegment,
    PciBus,
    PciDevice,
    PciFunction,
    RomBar,
    RomImageSize
    ));

  PciIoDevice->EmbeddedRom    = TRUE;
  PciIoDevice->PciIo.RomSize  = RomImageSize;
  PciIoDevice->PciIo.RomImage = RomInMemory;

  *RomImage = RomInMemory;
  *RomSize  = RomImageSize;

  Status = EFI_SUCCESS;

RestoreBar:
  //
  // Restore BAR
  //
  PciIo->Pci.Write (
               PciIo,
               EfiPciWidthUint32,
               RomBarIndex,
               1,
               &RomBar
               );

CloseAndReturn:
  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
         PciHandle,
         &gEfiPciIoProtocolGuid,
         PciIo,
         PciHandle
         );

  return Status;
}

EFI_STATUS
EFIAPI
PciGetPlatformPolicy (
  IN  CONST EFI_PCI_PLATFORM_PROTOCOL  *This,
  OUT       EFI_PCI_PLATFORM_POLICY    *PciPolicy
  )
{
  if (PciPolicy == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *PciPolicy = 0;

  return EFI_SUCCESS;
}

EFI_PCI_PLATFORM_PROTOCOL  mPciPlatformProtocol = {
  PciPlatformNotify,
  PciPlatformPrepController,
  PciGetPlatformPolicy,
  PciGetPciRom,
};

/**
  The Entry Point for Option ROM driver.

  It installs DriverBinding.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InstallPciPlatformProtocol (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  DriverHandle = NULL;

  Status = gBS->InstallProtocolInterface (
                  &DriverHandle,
                  &gEfiPciPlatformProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPciPlatformProtocol
                  );

  return Status;
}
