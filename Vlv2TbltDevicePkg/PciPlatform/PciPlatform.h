/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

    PciPlatform.h

Abstract:

--*/
#ifndef PCI_PLATFORM_H_
#define PCI_PLATFORM_H_


#include <PiDxe.h>
#include "Platform.h"

//
// Produced Protocols
//
#include <Protocol/PciPlatform.h>

#define IGD_DID_II                     0x0BE1
#define IGD_DID_0BE4                   0x0BE4
#define IGD_DID_VLV_A0                 0x0F31
#define OPROM_DID_OFFSET               0x46

typedef struct {
  EFI_GUID  FileName;
  UINTN     Segment;
  UINTN     Bus;
  UINTN     Device;
  UINTN     Function;
  UINT16    VendorId;
  UINT16    DeviceId;
  UINT8     Flag;
} PCI_OPTION_ROM_TABLE;

EFI_STATUS
EFIAPI
PhaseNotify (
  IN  EFI_PCI_PLATFORM_PROTOCOL                     *This,
  IN  EFI_HANDLE                                    HostBridge,
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE Phase,
  IN  EFI_PCI_CHIPSET_EXECUTION_PHASE               ChipsetPhase
  );


EFI_STATUS
EFIAPI
PlatformPrepController (
  IN EFI_PCI_PLATFORM_PROTOCOL                      *This,
  IN   EFI_HANDLE                                   HostBridge,
  IN  EFI_HANDLE                                    RootBridge,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS   PciAddress,
  IN  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE  Phase,
  IN  EFI_PCI_CHIPSET_EXECUTION_PHASE               ChipsetPhase
  );

EFI_STATUS
EFIAPI    
GetPlatformPolicy (
  IN CONST EFI_PCI_PLATFORM_PROTOCOL                      *This,
  OUT EFI_PCI_PLATFORM_POLICY                       *PciPolicy
  );

EFI_STATUS
EFIAPI
GetPciRom (
  IN CONST EFI_PCI_PLATFORM_PROTOCOL                      *This,
  IN EFI_HANDLE                                     PciHandle,
  OUT  VOID                                         **RomImage,
  OUT  UINTN                                        *RomSize
  );

#endif


