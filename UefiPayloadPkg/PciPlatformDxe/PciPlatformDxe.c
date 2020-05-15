/** @file
  This driver will probe for the Option Rom and provide a pointer to
  the activated BAR if found.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "PciPlatformDxe.h"
#include <Bus/Pci/PciBusDxe/PciBus.h>
#include <Bus/Pci/PciBusDxe/PciOptionRomSupport.h>

//
// The driver should only start on one graphics controller.
// So a global flag is used to remember that the driver is already started.
//
EFI_HANDLE      mDriverHandle = NULL;

/**
  The notification from the PCI bus enumerator to the platform that it is
  about to enter a certain phase during the enumeration process.

  @param[in] This           The pointer to the EFI_PCI_PLATFORM_PROTOCOL instance.
  @param[in] HostBridge     The handle of the host bridge controller.
  @param[in] Phase          The phase of the PCI bus enumeration.
  @param[in] ExecPhase      Defines the execution phase of the PCI chipset driver.

  @retval EFI_SUCCESS   The function completed successfully.

**/
EFI_STATUS
EFIAPI
PciPlatformNotify(
  IN EFI_PCI_PLATFORM_PROTOCOL                      *This,
  IN EFI_HANDLE                                     HostBridge,
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE  Phase,
  IN EFI_PCI_EXECUTION_PHASE                        ExecPhase
  )
{
  return EFI_SUCCESS;
}


/**
  The notification from the PCI bus enumerator to the platform for each PCI
  controller at several predefined points during PCI controller initialization.

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
PciPlatformPrepController(
  IN EFI_PCI_PLATFORM_PROTOCOL                     *This,
  IN EFI_HANDLE                                    HostBridge,
  IN EFI_HANDLE                                    RootBridge,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS   PciAddress,
  IN EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE  Phase,
  IN EFI_PCI_EXECUTION_PHASE                       ExecPhase
  )
{
  return EFI_SUCCESS;
}

/**
  Gets the PCI device's option ROM.

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
  EFI_STATUS                           Status;
  IN EFI_PCI_IO_PROTOCOL               *PciIo;
  UINTN                                RomBarIndex;
  UINT32                               Buffer;
  UINT32                               AllOnes;
  PCI_IO_DEVICE                        *PciIoDevice;
  UINT8                                Indicator;
  UINT16                               OffsetPcir;
  UINT32                               RomBarOffset;
  UINT32                               RomBar;
  BOOLEAN                              FirstCheck;
  PCI_EXPANSION_ROM_HEADER             *RomHeader;
  PCI_DATA_STRUCTURE                   *RomPcir;
  UINT64                               RomImageSize;
  UINT32                               LegacyImageLength;
  UINT8                                *RomInMemory;
  UINT8                                CodeType;

  if (!RomImage || !RomSize) {
    return EFI_INVALID_PARAMETER;
  }

  *RomImage = NULL;
  *RomSize = 0;

  Status = gBS->HandleProtocol (
                  PciHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  DEBUG_CODE (
    {
      UINTN                                PciSegment;
      UINTN                                PciBus;
      UINTN                                PciDevice;
      UINTN                                PciFunction;

      //
      // Get the location of the PCI device
      //
      Status = PciIo->GetLocation (
              PciIo,
              &PciSegment,
              &PciBus,
              &PciDevice,
              &PciFunction
              );

      if (!EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "%a: Searching Option ROM on PCI @ [%02x|%02x|%02x] ..",
          __FUNCTION__,
          PciBus,
          PciDevice,
          PciFunction));
      }
    }
  );

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (PciIo);

  //
  // Offset is 0x30 if is not ppb
  //
  RomBarIndex = PCI_EXPANSION_ROM_BASE;

  if (IS_PCI_BRIDGE (&PciIoDevice->Pci)) {
    //
    // If is ppb, 0x38
    //
    RomBarIndex = PCI_BRIDGE_ROMBAR;
  }

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
    DEBUG ((DEBUG_INFO, "error accessing PCI device\n"));
    goto CloseAndReturn;
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
    DEBUG ((DEBUG_INFO, "error accessing PCI device\n"));
    goto CloseAndReturn;
  }

  //
  // Read back
  //
  Status = PciIo->Pci.Read(
                          PciIo,
                          EfiPciWidthUint32,
                          RomBarIndex,
                          1,
                          &AllOnes
                          );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "error accessing PCI device\n"));
    goto CloseAndReturn;
  }

  //
  // Bits [1, 10] are reserved
  //
  AllOnes &= 0xFFFFF800;
  if ((AllOnes == 0) || (AllOnes == 0xFFFFF800)) {
    DEBUG ((DEBUG_INFO, "nothing found\n"));
    Status = EFI_NOT_FOUND;
    goto CloseAndReturn;
  }

  *RomSize = (~AllOnes) + 1;

  DEBUG ((DEBUG_INFO, "found one of size %d\n", *RomSize));

  //
  // Restore BAR and enable it
  //
  Buffer |= 1;
  Status = PciIo->Pci.Write (
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

  // FIXME: Use gEfiPciRootBridgeIoProtocolGuid
  RomBar = (UINT32) Buffer &~1;

  RomBarOffset  = RomBar;
  FirstCheck    = TRUE;
  LegacyImageLength = 0;
  RomImageSize = 0;

  do {
    // FIXME: Use gEfiPciRootBridgeIoProtocolGuid
    CopyMem(RomHeader, (VOID *)(UINTN)RomBarOffset, sizeof (PCI_EXPANSION_ROM_HEADER));

    DEBUG ((DEBUG_INFO, "%a: RomHeader->Signature 0x%x\n", __FUNCTION__, RomHeader->Signature));

    if (RomHeader->Signature != PCI_EXPANSION_ROM_HEADER_SIGNATURE) {

      RomBarOffset = RomBarOffset + 512;
      if (FirstCheck) {
        break;
      } else {
        RomImageSize = RomImageSize + 512;
        continue;
      }
    }

    FirstCheck  = FALSE;
    OffsetPcir  = RomHeader->PcirOffset;
    //
    // If the pointer to the PCI Data Structure is invalid, no further images can be located.
    // The PCI Data Structure must be DWORD aligned.
    //
    if (OffsetPcir == 0 ||
        (OffsetPcir & 3) != 0 ||
        RomImageSize + OffsetPcir + sizeof (PCI_DATA_STRUCTURE) > *RomSize) {
      break;
    }
    // FIXME: Use gEfiPciRootBridgeIoProtocolGuid
    CopyMem(RomPcir, (VOID *)(UINTN)RomBarOffset + OffsetPcir, sizeof (PCI_DATA_STRUCTURE));

    DEBUG ((DEBUG_INFO, "%a: RomPcir->Signature 0x%x\n", __FUNCTION__, RomPcir->Signature));

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
      CodeType = PCI_CODE_TYPE_PCAT_IMAGE;
      LegacyImageLength = ((UINT32)((EFI_LEGACY_EXPANSION_ROM_HEADER *)RomHeader)->Size512) * 512;
    }
    Indicator     = RomPcir->Indicator;
    RomImageSize  = RomImageSize + RomPcir->ImageLength * 512;
    RomBarOffset  = RomBarOffset + RomPcir->ImageLength * 512;
  } while (((Indicator & 0x80) == 0x00) && ((RomBarOffset - RomBar) < *RomSize));

  //
  // Some Legacy Cards do not report the correct ImageLength so used the maximum
  // of the legacy length and the PCIR Image Length
  //
  if (CodeType == PCI_CODE_TYPE_PCAT_IMAGE) {
    RomImageSize = MAX (RomImageSize, LegacyImageLength);
  }

  if (RomImageSize > 0) {
    // FIXME: Use gEfiPciRootBridgeIoProtocolGuid
    RomInMemory = (VOID *)(UINTN)RomBar;
  }

  //
  // Free allocated memory
  //
  FreePool (RomHeader);
  FreePool (RomPcir);

  if (RomImageSize > 0) {
    *RomImage = RomInMemory;
    *RomSize = RomImageSize;
    DEBUG ((DEBUG_INFO, "%a: Found Option ROM at %p, length 0x%x\n", __FUNCTION__,
        RomInMemory, RomImageSize));

    Status = EFI_SUCCESS;
  } else {
    Status = EFI_NOT_FOUND;
  }

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
  if (PciPolicy == NULL)
    return EFI_INVALID_PARAMETER;

  *PciPolicy = 0;

  return EFI_SUCCESS;
}

EFI_PCI_PLATFORM_PROTOCOL                     mPciPlatformProtocol = {
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
  IN EFI_HANDLE                        ImageHandle,
  IN EFI_SYSTEM_TABLE                  *SystemTable
  )
{
  EFI_STATUS                           Status;

  Status = gBS->InstallProtocolInterface (
                  &mDriverHandle,
                  &gEfiPciPlatformProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPciPlatformProtocol
                  );

  return Status;
}
