/** @file
  Implementation to provide Option ROMs on platforms having
  a PCI resource allocator that includes the read-only Option ROM
  BAR into the parent PCI bridge MMIO window.

Copyright (c) 2022 9elements GmbH
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "PciPlatformDxe.h"
#include <Bus/Pci/PciBusDxe/PciBus.h>
#include <Bus/Pci/PciBusDxe/PciOptionRomSupport.h>

#define PCI_IO_DEVICE_ROM_BAR_INDEX(x) \
  (IS_PCI_BRIDGE (&(PCI_IO_DEVICE_FROM_PCI_IO_THIS (x))->Pci) ? \
    PCI_BRIDGE_ROMBAR: PCI_EXPANSION_ROM_BASE)

/**
  The notification from the PCI bus enumerator to the platform that it is
  about to enter a certain phase during the enumeration process.

  The PlatformNotify() function can be used to notify the platform driver so that
  it can perform platform-specific actions. No specific actions are required.
  Eight notification points are defined at this time. More synchronization points
  may be added as required in the future. The PCI bus driver calls the platform driver
  twice for every Phase-once before the PCI Host Bridge Resource Allocation Protocol
  driver is notified, and once after the PCI Host Bridge Resource Allocation Protocol
  driver has been notified.
  This member function may not perform any error checking on the input parameters. It
  also does not return any error codes. If this member function detects any error condition,
  it needs to handle those errors on its own because there is no way to surface any
  errors to the caller.

  @param[in] This           The pointer to the EFI_PCI_PLATFORM_PROTOCOL instance.
  @param[in] HostBridge     The handle of the host bridge controller.
  @param[in] Phase          The phase of the PCI bus enumeration.
  @param[in] ExecPhase      Defines the execution phase of the PCI chipset driver.

  @retval EFI_SUCCESS   The function completed successfully.

**/
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

/**
  The notification from the PCI bus enumerator to the platform for each PCI
  controller at several predefined points during PCI controller initialization.

  The PlatformPrepController() function can be used to notify the platform driver so that
  it can perform platform-specific actions. No specific actions are required.
  Several notification points are defined at this time. More synchronization points may be
  added as required in the future. The PCI bus driver calls the platform driver twice for
  every PCI controller-once before the PCI Host Bridge Resource Allocation Protocol driver
  is notified, and once after the PCI Host Bridge Resource Allocation Protocol driver has
  been notified.
  This member function may not perform any error checking on the input parameters. It also
  does not return any error codes. If this member function detects any error condition, it
  needs to handle those errors on its own because there is no way to surface any errors to
  the caller.

  @param[in] This           The pointer to the EFI_PCI_PLATFORM_PROTOCOL instance.
  @param[in] HostBridge     The associated PCI host bridge handle.
  @param[in] RootBridge     The associated PCI root bridge handle.
  @param[in] PciAddress     The address of the PCI device on the PCI bus.
  @param[in] Phase          The phase of the PCI controller enumeration.
  @param[in] ExecPhase      Defines the execution phase of the PCI chipset driver.

  @retval EFI_SUCCESS   The function completed successfully.

**/
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
  Returns the size of the ROM BAR.

  @param[in]  PciIo         A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param[out] RomSize       The variable to write the ROM BAR size to.
  @param[out] Address       The variable to write the ROM BAR MIO address to.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   A NULL pointer was provided.

**/
STATIC
EFI_STATUS
EFIAPI
PciGetROMBar (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  OUT UINTN                *RomSize,
  OUT UINT32               *Address
  )
{
  UINT32      RomBarIndex;
  UINT32      Buffer;
  UINT32      AllOnes;
  EFI_STATUS  Status;

  if ((PciIo == NULL) || (RomSize == NULL) || (Address == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  RomBarIndex = PCI_IO_DEVICE_ROM_BAR_INDEX (PciIo);

  //
  // Backup BAR
  //

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciWidthUint32,
                        RomBarIndex,
                        1,
                        &Buffer
                        );
  if (EFI_ERROR (Status)) {
    return Status;
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
    return Status;
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
    return Status;
  }

  //
  // Bits [1, 10] are reserved
  //
  AllOnes &= 0xFFFFF800;
  if (AllOnes == 0xFFFFF800) {
    AllOnes = 0;
  }

  //
  // Restore BAR
  //
  Status = PciIo->Pci.Write (
                        PciIo,
                        EfiPciWidthUint32,
                        RomBarIndex,
                        1,
                        &Buffer
                        );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *RomSize = AllOnes;
  *Address = Buffer& 0xFFFFF800;

  return EFI_SUCCESS;
}

/**
  Toggles the MMIO decoding of the ROM BAR. It assumes that
  - the PCI device ROM BAR is valid
  - the PCI device ROM BAR is covered by the parent PCI bridge MMIO aperture
  and thus it can safely be enabled.

  @param[in] PciIo         A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param[in] Enable        Enable/disable ROM decode.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   A NULL pointer was provided.

**/
EFI_STATUS
EFIAPI
PciROMDecode (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN BOOLEAN              Enable
  )
{
  UINT32      RomBarIndex;
  UINT32      Buffer;
  EFI_STATUS  Status;

  if (PciIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  RomBarIndex = PCI_IO_DEVICE_ROM_BAR_INDEX (PciIo);

  //
  // Read BAR
  //

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciWidthUint32,
                        RomBarIndex,
                        1,
                        &Buffer
                        );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Enable) {
    Buffer |= 1;
  } else {
    Buffer &= 0xFFFFFFFE;
  }

  //
  // Write BAR
  //
  Status = PciIo->Pci.Write (
                        PciIo,
                        EfiPciWidthUint32,
                        RomBarIndex,
                        1,
                        &Buffer
                        );

  return Status;
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
  scanning the ROM that is attached to any controller, which allows a platform to specify a ROM
  image that is different from the ROM image on a PCI card.

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
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_IO_DEVICE             *PciIoDevice;
  UINT8                     Indicator;
  UINT16                    OffsetPcir;
  UINT32                    RomBarOffset;
  UINT32                    RomBar;
  BOOLEAN                   FirstCheck;
  PCI_EXPANSION_ROM_HEADER  *RomHeader;
  PCI_DATA_STRUCTURE        *RomPcir;
  UINT64                    RomImageSize;
  UINT32                    LegacyImageLength;
  UINT8                     CodeType;

  if ((This == NULL) || (RomImage == NULL) || (RomSize == NULL)) {
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
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to open gEfiPciIoProtocolGuid: %r\n",
      __FUNCTION__,
      Status
      ));
    return Status;
  }

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (PciIo);

  Status = PciGetROMBar (PciIo, RomSize, &RomBar);
  if (EFI_ERROR (Status)) {
    goto CloseAndReturn;
  }

  if (*RomSize == 0) {
    DEBUG ((DEBUG_INFO, "%a: No Option ROM found\n", __FUNCTION__));
    Status = EFI_NOT_FOUND;
    goto CloseAndReturn;
  }

  Status = PciROMDecode (PciIo, TRUE);
  if (EFI_ERROR (Status)) {
    goto CloseAndReturn;
  }

  //
  // Allocate memory for Rom header and PCIR
  //
  RomHeader = AllocatePool (sizeof (PCI_EXPANSION_ROM_HEADER));
  if (RomHeader == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto CloseAndReturn;
  }

  RomPcir = AllocatePool (sizeof (PCI_DATA_STRUCTURE));
  if (RomPcir == NULL) {
    FreePool (RomHeader);
    Status = EFI_OUT_OF_RESOURCES;
    goto CloseAndReturn;
  }

  RomBarOffset      = RomBar;
  FirstCheck        = TRUE;
  LegacyImageLength = 0;
  RomImageSize      = 0;
  CodeType          = 0xFF;

  do {
    PciIoDevice->PciRootBridgeIo->Mem.Read (
                                        PciIoDevice->PciRootBridgeIo,
                                        EfiPciWidthUint8,
                                        RomBarOffset,
                                        sizeof (PCI_EXPANSION_ROM_HEADER),
                                        (UINT8 *)RomHeader
                                        );

    if (RomHeader->Signature != PCI_EXPANSION_ROM_HEADER_SIGNATURE) {
      RomBarOffset = RomBarOffset + 512;
      if (FirstCheck) {
        break;
      } else {
        RomImageSize = RomImageSize + 512;
        continue;
      }
    }

    FirstCheck = FALSE;
    OffsetPcir = RomHeader->PcirOffset;
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

    PciIoDevice->PciRootBridgeIo->Mem.Read (
                                        PciIoDevice->PciRootBridgeIo,
                                        EfiPciWidthUint8,
                                        RomBarOffset + OffsetPcir,
                                        sizeof (PCI_DATA_STRUCTURE),
                                        (UINT8 *)RomPcir
                                        );

    //
    // If a valid signature is not present in the PCI Data Structure, no further images can be located.
    //
    if (RomPcir->Signature != PCI_DATA_STRUCTURE_SIGNATURE) {
      break;
    }

    if (RomImageSize + RomPcir->ImageLength * 512 > *RomSize) {
      break;
    }

    if (RomPcir->CodeType == PCI_CODE_TYPE_PCAT_IMAGE) {
      CodeType          = PCI_CODE_TYPE_PCAT_IMAGE;
      LegacyImageLength = ((UINT32)((EFI_LEGACY_EXPANSION_ROM_HEADER *)RomHeader)->Size512) * 512;
    }

    Indicator    = RomPcir->Indicator;
    RomImageSize = RomImageSize + RomPcir->ImageLength * 512;
    RomBarOffset = RomBarOffset + RomPcir->ImageLength * 512;
  } while (((Indicator & 0x80) == 0x00) && ((RomBarOffset - RomBar) < *RomSize));

  //
  // Some Legacy Cards do not report the correct ImageLength so used the maximum
  // of the legacy length and the PCIR Image Length
  //
  if (CodeType == PCI_CODE_TYPE_PCAT_IMAGE) {
    RomImageSize = MAX (RomImageSize, LegacyImageLength);
  }

  //
  // Free allocated memory
  //
  FreePool (RomHeader);
  FreePool (RomPcir);

  if (RomImageSize > 0) {
    Status    = EFI_SUCCESS;
    *RomImage = AllocatePool ((UINT32)RomImageSize);
    if (*RomImage == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      //
      // Copy Rom image into memory
      //
      PciIoDevice->PciRootBridgeIo->Mem.Read (
                                          PciIoDevice->PciRootBridgeIo,
                                          EfiPciWidthUint32,
                                          RomBar,
                                          (UINT32)RomImageSize/sizeof (UINT32),
                                          *RomImage
                                          );
      *RomSize = RomImageSize;
      DEBUG ((
        DEBUG_INFO,
        "%a: Found Option ROM at 0x%x, length 0x%x\n",
        __FUNCTION__,
        RomBar,
        RomImageSize
        ));
    }
  } else {
    Status = EFI_NOT_FOUND;
  }

CloseAndReturn:
  PciROMDecode (PciIo, FALSE);

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

/**
  Retrieves the platform policy regarding enumeration.

  The GetPlatformPolicy() function retrieves the platform policy regarding PCI
  enumeration. The PCI bus driver and the PCI Host Bridge Resource Allocation Protocol
  driver can call this member function to retrieve the policy.

  @param[in]  This        The pointer to the EFI_PCI_PLATFORM_PROTOCOL instance.
  @param[out] PciPolicy   The platform policy with respect to VGA and ISA aliasing.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   PciPolicy is NULL.

**/
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

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

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

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiPciPlatformProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPciPlatformProtocol
                  );

  return Status;
}
