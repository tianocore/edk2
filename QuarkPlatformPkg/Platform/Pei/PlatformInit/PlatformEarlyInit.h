/** @file
The header file of Platform PEIM.

Copyright (c) 2013 - 2019 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef __PLATFORM_EARLY_INIT_H__
#define __PLATFORM_EARLY_INIT_H__

#define PEI_STALL_RESOLUTION            1
#define STALL_PEIM_SIGNATURE   SIGNATURE_32('p','p','u','s')

typedef struct {
  UINT32                      Signature;
  EFI_FFS_FILE_HEADER         *FfsHeader;
  EFI_PEI_NOTIFY_DESCRIPTOR   StallNotify;
} STALL_CALLBACK_STATE_INFORMATION;

#define STALL_PEIM_FROM_THIS(a) CR (a, STALL_CALLBACK_STATE_INFORMATION, StallNotify, STALL_PEIM_SIGNATURE)

//
// USB Phy Registers
//
#define USB2_GLOBAL_PORT  0x4001
#define USB2_PLL1         0x7F02
#define USB2_PLL2         0x7F03
#define USB2_COMPBG       0x7F04

/**
  Peform the boot mode determination logic
  If the box is closed, then
    1. If it's first time to boot, it's boot with full config .
    2. If the ChassisIntrution is selected, force to be a boot with full config
    3. Otherwise it's boot with no change.

  @param  PeiServices General purpose services available to every PEIM.

  @param  BootMode The detected boot mode.

  @retval EFI_SUCCESS if the boot mode could be set
**/
EFI_STATUS
UpdateBootMode (
  IN  EFI_PEI_SERVICES     **PeiServices,
  OUT EFI_BOOT_MODE        *BootMode
  );

/**
  This function reset the entire platform, including all processor and devices, and
  reboots the system.

  @param  PeiServices General purpose services available to every PEIM.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
PlatformResetSystem (
  IN CONST EFI_PEI_SERVICES          **PeiServices
  );

/**
  This function will be called when MRC is done.

  @param  PeiServices General purpose services available to every PEIM.

  @param  NotifyDescriptor Information about the notify event..

  @param  Ppi The notify context.

  @retval EFI_SUCCESS If the function completed successfully.
**/
EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN EFI_PEI_SERVICES                     **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR            *NotifyDescriptor,
  IN VOID                                 *Ppi
  );

/**
  This is the callback function notified by FvFileLoader PPI, it depends on FvFileLoader PPI to load
  the PEIM into memory.

  @param  PeiServices General purpose services available to every PEIM.
  @param  NotifyDescriptor The context of notification.
  @param  Ppi The notify PPI.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
FvFileLoaderPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

/**
  This function provides a blocking stall for reset at least the given number of microseconds
  stipulated in the final argument.

  @param  PeiServices General purpose services available to every PEIM.

  @param  this Pointer to the local data for the interface.

  @param  Microseconds number of microseconds for which to stall.

  @retval EFI_SUCCESS the function provided at least the required stall.
**/
EFI_STATUS
EFIAPI
Stall (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN CONST EFI_PEI_STALL_PPI  *This,
  IN UINTN                    Microseconds
  );

/**
  This function initialize recovery functionality by installing the recovery PPI.

  @param  PeiServices General purpose services available to every PEIM.

  @retval EFI_SUCCESS if the interface could be successfully installed.
**/
EFI_STATUS
EFIAPI
PeimInitializeRecovery (
  IN EFI_PEI_SERVICES     **PeiServices
  );

/**
  This function
    1. Calling MRC to initialize memory.
    2. Install EFI Memory.
    3. Create HOB of system memory.

  @param  PeiServices Pointer to the PEI Service Table

  @retval EFI_SUCCESS If it completes successfully.

**/
EFI_STATUS
MemoryInit (
  IN EFI_PEI_SERVICES          **PeiServices
  );

/** Return info derived from Installing Memory by MemoryInit.

  @param[out]      RmuMainBaseAddressPtr   Return RmuMainBaseAddress to this location.
  @param[out]      SmramDescriptorPtr  Return start of Smram descriptor list to this location.
  @param[out]      NumSmramRegionsPtr  Return numbers of Smram regions to this location.

  @return Address of RMU shadow region at the top of available memory.
  @return List of Smram descriptors for each Smram region.
  @return Numbers of Smram regions.
**/
VOID
EFIAPI
InfoPostInstallMemory (
  OUT     UINT32                    *RmuMainBaseAddressPtr OPTIONAL,
  OUT     EFI_SMRAM_DESCRIPTOR      **SmramDescriptorPtr OPTIONAL,
  OUT     UINTN                     *NumSmramRegionsPtr OPTIONAL
  );

/**
  This function provides the implementation of AtaController PPI Enable Channel function.

  @param  PeiServices General purpose services available to every PEIM.
  @param  this Pointer to the local data for the interface.
  @param  ChannelMask This parameter is used to specify primary or slavery IDE channel.

  @retval EFI_SUCCESS  Procedure returned successfully.
**/

EFI_STATUS
EnableAtaChannel (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN PEI_ATA_CONTROLLER_PPI         *This,
  IN UINT8                          ChannelMask
  );

/**
  This function provides the implementation of AtaController PPI Get IDE channel Register Base Address

  @param  PeiServices      General purpose services available to every PEIM.
  @param  this             Pointer to the local data for the interface.
  @param  IdeRegsBaseAddr  Pointer to IDE_REGS_BASE_ADDR struct, which is used to record
                           IDE Command and Control regeisters Base Address.

  @retval EFI_SUCCESS  Procedure returned successfully.
**/

EFI_STATUS
GetIdeRegsBaseAddr (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN PEI_ATA_CONTROLLER_PPI         *This,
  IN IDE_REGS_BASE_ADDR             *IdeRegsBaseAddr
  );

VOID
EFIAPI
InitializeUSBPhy (
    VOID
   );

/**
  This function provides early platform initialisation.

  @param  PlatformInfo  Pointer to platform Info structure.

**/
VOID
EFIAPI
EarlyPlatformInit (
  VOID
  );

/**
  This function provides early platform GPIO initialisation.

  @param  PlatformType  Platform type for GPIO init.

**/
VOID
EFIAPI
EarlyPlatformLegacyGpioInit (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  );

/**
  Performs any early platform specific GPIO manipulation.

  @param  PlatformType  Platform type GPIO manipulation.

**/
VOID
EFIAPI
EarlyPlatformLegacyGpioManipulation (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  );

/**
  Performs any early platform specific GPIO Controller init & manipulation.

  @param  PlatformType  Platform type for GPIO init & manipulation.

**/
VOID
EFIAPI
EarlyPlatformGpioCtrlerInitAndManipulation (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  );

/**
  Performs any early platform init of SoC Ethernet Mac devices.

  @param  IohMac0Address  Mac address to program into Mac0 device.
  @param  IohMac1Address  Mac address to program into Mac1 device.

**/
VOID
EFIAPI
EarlyPlatformMacInit (
  IN CONST UINT8                          *IohMac0Address,
  IN CONST UINT8                          *IohMac1Address
  );

/**
  Find security headers using EFI_CAPSULE_VARIABLE_NAME variables and build Hobs.

  @param PeiServices  General purpose services available to every PEIM.

  @retval 0 if no security headers found.
  @return number of security header hobs built.
**/
UINTN
EFIAPI
FindCapsuleSecurityHeadersAndBuildHobs (
  IN EFI_PEI_SERVICES                     **PeiServices
  );

/**
  Build capsule security header hob.

  @param SecHdr  Pointer to security header.

  @retval NULL if failure to build HOB.
  @return pointer to built hob.
**/
VOID *
EFIAPI
BuildCapsuleSecurityHeaderHob (
  IN VOID                                 *SecHdr
  );

#endif
