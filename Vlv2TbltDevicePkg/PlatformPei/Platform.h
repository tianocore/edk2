/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


**/


#ifndef __PEI_PLATFORM_H__
#define __PEI_PLATFORM_H__

#define PEI_STALL_RESOLUTION            1
#define STALL_PEIM_SIGNATURE   SIGNATURE_32('p','p','u','s')

typedef struct {
  UINT32                      Signature;
  EFI_FFS_FILE_HEADER         *FfsHeader;
  EFI_PEI_NOTIFY_DESCRIPTOR   StallNotify;
} STALL_CALLBACK_STATE_INFORMATION;

#define STALL_PEIM_FROM_THIS(a) CR (a, STALL_CALLBACK_STATE_INFORMATION, StallNotify, STALL_PEIM_SIGNATURE)

#ifdef NOCS_S3_SUPPORT

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
  IN CONST EFI_PEI_SERVICES     **PeiServices
  );
#endif

/**
  This function reset the entire platform, including all processor and devices, and
  reboots the system.

  @param  PeiServices General purpose services available to every PEIM.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
ResetSystem (
  IN CONST EFI_PEI_SERVICES          **PeiServices
  );

/**
  This function will be called when MRC is done.

  @param  PeiServices        General purpose services available to every PEIM.
  @param  NotifyDescriptor   Information about the notify event..
  @param  Ppi                The notify context.

  @retval EFI_SUCCESS        If the function completed successfully.
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

  @param  PeiServices       General purpose services available to every PEIM.
  @param  NotifyDescriptor  The context of notification.
  @param  Ppi               The notify PPI.

  @retval EFI_SUCCESS       if it completed successfully.
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

  @param  PeiServices    General purpose services available to every PEIM.
  @param  this Pointer   to the local data for the interface.
  @param  Microseconds   number of microseconds for which to stall.

  @retval EFI_SUCCESS    the function provided at least the required stall.
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

  @param  PeiServices  General purpose services available to every PEIM.

  @retval EFI_SUCCESS  If the interface could be successfully installed.
**/
EFI_STATUS
EFIAPI
InitializeRecovery (
  IN EFI_PEI_SERVICES     **PeiServices
  );

/**
  This function
    1. Calling MRC to initialize memory.
    2. Install EFI Memory.
    3. Capsule coalesce if capsule boot mode.
    4. Create HOB of system memory.

  @param  PeiServices Pointer to the PEI Service Table

  @retval EFI_SUCCESS If it completes successfully.

**/
EFI_STATUS
MemoryInit (
  IN EFI_PEI_SERVICES          **PeiServices
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

/**
  This function provides the implementation to properly setup both LM & PDM functionality.

  @param  PeiServices      General purpose services available to every PEIM.

  @retval EFI_SUCCESS  Procedure returned successfully.

**/
EFI_STATUS
ConfigureLM(
  IN EFI_PEI_SERVICES **PeiServices
  );

#include <Ppi/VlvMmioPolicy.h>

BOOLEAN
EFIAPI
IsFastBootEnabled (
  IN CONST EFI_PEI_SERVICES **PeiServices
  );

EFI_STATUS
PrioritizeBootMode (
  IN OUT EFI_BOOT_MODE    *CurrentBootMode,
  IN EFI_BOOT_MODE        NewBootMode
  );

EFI_STATUS
EFIAPI
CapsulePpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );
#endif
