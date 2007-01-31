/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    ScsiBus.h

Abstract:

    Header file for SCSI Bus Driver.

Revision History
++*/

// TODO: fix comment to end with --*/
#ifndef _SCSI_BUS_H
#define _SCSI_BUS_H


#include <IndustryStandard/scsi.h>
//
// 1000 * 1000 * 10
//
#define ONE_SECOND_TIMER      10000000  

#define SCSI_IO_DEV_SIGNATURE EFI_SIGNATURE_32 ('s', 'c', 'i', 'o')

typedef struct {
  UINT32                      Signature;

  EFI_HANDLE                  Handle;
  EFI_SCSI_IO_PROTOCOL        ScsiIo;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
  EFI_SCSI_PASS_THRU_PROTOCOL *ScsiPassThru;

  UINT32                      Pun;
  UINT64                      Lun;
  UINT8                       ScsiDeviceType;
  UINT8                       ScsiVersion;
  BOOLEAN                     RemovableDevice;
} SCSI_IO_DEV;

#define SCSI_IO_DEV_FROM_THIS(a)  CR (a, SCSI_IO_DEV, ScsiIo, SCSI_IO_DEV_SIGNATURE)

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL  gScsiBusDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gScsiBusComponentName;

EFI_STATUS
EFIAPI
SCSIBusDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
SCSIBusDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
SCSIBusDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  );

//
// EFI Component Name Functions
//
EFI_STATUS
EFIAPI
ScsiBusComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

EFI_STATUS
EFIAPI
ScsiBusComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

EFI_STATUS
EFIAPI
ScsiGetDeviceType (
  IN  EFI_SCSI_IO_PROTOCOL     *This,
  OUT UINT8                    *DeviceType
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  DeviceType  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
ScsiResetBus (
  IN  EFI_SCSI_IO_PROTOCOL     *This
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
ScsiResetDevice (
  IN  EFI_SCSI_IO_PROTOCOL     *This
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
ScsiExecuteSCSICommand (
  IN  EFI_SCSI_IO_PROTOCOL                 *This,
  IN OUT  EFI_SCSI_IO_SCSI_REQUEST_PACKET  *CommandPacket,
  IN  EFI_EVENT                            Event
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This          - TODO: add argument description
  CommandPacket - TODO: add argument description
  Event         - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ScsiScanCreateDevice (
  EFI_DRIVER_BINDING_PROTOCOL   *This,
  EFI_HANDLE                    Controller,
  UINT32                        Pun,
  UINT64                        Lun,
  EFI_SCSI_PASS_THRU_PROTOCOL   *ScsiPassThru,
  EFI_DEVICE_PATH_PROTOCOL      *ParentDevicePath
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This              - TODO: add argument description
  Controller        - TODO: add argument description
  Pun               - TODO: add argument description
  Lun               - TODO: add argument description
  ScsiPassThru      - TODO: add argument description
  ParentDevicePath  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
DiscoverScsiDevice (
  SCSI_IO_DEV   *ScsiIoDevice
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiIoDevice  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
GetLunList (
  EFI_SCSI_PASS_THRU_PROTOCOL *ScsiPassThru,
  UINT32                      Target,
  UINT64                      **LunArray,
  UINTN                       *NumberOfLuns
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiPassThru  - TODO: add argument description
  Target        - TODO: add argument description
  LunArray      - TODO: add argument description
  NumberOfLuns  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ScsiBusSubmitReportLunCommand (
  EFI_SCSI_PASS_THRU_PROTOCOL   *ScsiPassThru,
  UINT32                        Target,
  UINTN                         AllocationLength,
  VOID                          *Buffer,
  EFI_SCSI_SENSE_DATA           *SenseData,
  UINT8                         *SenseDataLength,
  UINT8                         *HostAdapterStatus,
  UINT8                         *TargetStatus
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiPassThru      - TODO: add argument description
  Target            - TODO: add argument description
  AllocationLength  - TODO: add argument description
  Buffer            - TODO: add argument description
  SenseData         - TODO: add argument description
  SenseDataLength   - TODO: add argument description
  HostAdapterStatus - TODO: add argument description
  TargetStatus      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;
#endif
