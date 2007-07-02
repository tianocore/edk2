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

typedef struct _SCSI_TARGET_ID {
 union {
   UINT32  Scsi;
   UINT8   ExtScsi[4];   
 } ScsiId;
  UINT8   ExtScsiId[12];
}SCSI_TARGET_ID;


typedef struct {
   VOID   *Data1;
   VOID   *Data2;
} SCSI_EVENT_DATA;


typedef struct {
  UINT32                             Signature;
  EFI_HANDLE                         Handle;
  EFI_SCSI_IO_PROTOCOL               ScsiIo;
  EFI_DEVICE_PATH_PROTOCOL           *DevicePath;
  BOOLEAN                            ExtScsiSupport; 
  EFI_SCSI_PASS_THRU_PROTOCOL        *ScsiPassThru;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL    *ExtScsiPassThru;
  SCSI_TARGET_ID                     Pun;
  UINT64                             Lun;
  UINT8                              ScsiDeviceType;
  UINT8                              ScsiVersion;
  BOOLEAN                            RemovableDevice;
} SCSI_IO_DEV;

#define SCSI_IO_DEV_FROM_THIS(a)  CR (a, SCSI_IO_DEV, ScsiIo, SCSI_IO_DEV_SIGNATURE)

//
// SCSI Bus Controller device strcuture
//
#define EFI_SCSI_BUS_PROTOCOL_GUID \
  { \
    0x5261213D, 0x3A3D, 0x441E, 0xB3, 0xAF, 0x21, 0xD3, 0xF7, 0xA4, 0xCA, 0x17 \
  }

typedef struct _EFI_SCSI_BUS_PROTOCOL {
  UINT64  Reserved;
} EFI_SCSI_BUS_PROTOCOL;

#define SCSI_BUS_DEVICE_SIGNATURE  EFI_SIGNATURE_32 ('s', 'c', 's', 'i')


typedef struct _SCSI_BUS_DEVICE {
  UINTN                                 Signature;
  EFI_SCSI_BUS_PROTOCOL                 BusIdentify;
  BOOLEAN                               ExtScsiSupport; 
  EFI_SCSI_PASS_THRU_PROTOCOL           *ScsiInterface;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL       *ExtScsiInterface;
  EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
} SCSI_BUS_DEVICE;

#define SCSI_BUS_CONTROLLER_DEVICE_FROM_THIS(a)  CR (a, SCSI_BUS_DEVICE, BusIdentify, SCSI_BUS_DEVICE_SIGNATURE)

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

  Retrieves the device type information of the SCSI Controller.
    
Arguments:

  This                  - Protocol instance pointer.
  DeviceType            - A pointer to the device type information
                            retrieved from the SCSI Controller. 

Returns:

  EFI_SUCCESS           - Retrieves the device type information successfully.
  EFI_INVALID_PARAMETER - The DeviceType is NULL.

--*/
;

EFI_STATUS
EFIAPI
ScsiResetBus (
  IN  EFI_SCSI_IO_PROTOCOL     *This
  )
/*++

Routine Description:

  Resets the SCSI Bus that the SCSI Controller is attached to.
    
Arguments:

  This                  - Protocol instance pointer.

Returns:

  EFI_SUCCESS           - The SCSI bus is reset successfully.
  EFI_DEVICE_ERROR      - Errors encountered when resetting the SCSI bus.
  EFI_UNSUPPORTED       - The bus reset operation is not supported by the
                          SCSI Host Controller.
  EFI_TIMEOUT           - A timeout occurred while attempting to reset 
                          the SCSI bus.

--*/
;

EFI_STATUS
EFIAPI
ScsiResetDevice (
  IN  EFI_SCSI_IO_PROTOCOL     *This
  )
/*++

Routine Description:

  Resets the SCSI Controller that the device handle specifies.
    
Arguments:

  This                  - Protocol instance pointer.
    
Returns:

  EFI_SUCCESS           - Reset the SCSI controller successfully.
  EFI_DEVICE_ERROR      - Errors are encountered when resetting the
                          SCSI Controller.
  EFI_UNSUPPORTED       - The SCSI bus does not support a device 
                          reset operation.
  EFI_TIMEOUT           - A timeout occurred while attempting to 
                          reset the SCSI Controller.

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

  Sends a SCSI Request Packet to the SCSI Controller for execution.
    
Arguments:

  This                  - Protocol instance pointer.
  Packet                - The SCSI request packet to send to the SCSI 
                          Controller specified by the device handle.
  Event                 - If the SCSI bus where the SCSI device is attached
                          does not support non-blocking I/O, then Event is 
                          ignored, and blocking I/O is performed.  
                          If Event is NULL, then blocking I/O is performed.
                          If Event is not NULL and non-blocking I/O is 
                          supported, then non-blocking I/O is performed,
                          and Event will be signaled when the SCSI Request
                          Packet completes.
Returns:

  EFI_SUCCESS           - The SCSI Request Packet was sent by the host 
                          successfully, and TransferLength bytes were 
                          transferred to/from DataBuffer.See 
                          HostAdapterStatus, TargetStatus, 
                          SenseDataLength, and SenseData in that order
                          for additional status information.
  EFI_BAD_BUFFER_SIZE   - The SCSI Request Packet was executed, 
                          but the entire DataBuffer could not be transferred.
                          The actual number of bytes transferred is returned
                          in TransferLength. See HostAdapterStatus, 
                          TargetStatus, SenseDataLength, and SenseData in 
                          that order for additional status information.
  EFI_NOT_READY         - The SCSI Request Packet could not be sent because 
                          there are too many SCSI Command Packets already 
                          queued.The caller may retry again later.
  EFI_DEVICE_ERROR      - A device error occurred while attempting to send 
                          the SCSI Request Packet. See HostAdapterStatus, 
                          TargetStatus, SenseDataLength, and SenseData in 
                          that order for additional status information.
  EFI_INVALID_PARAMETER - The contents of CommandPacket are invalid.  
                          The SCSI Request Packet was not sent, so no 
                          additional status information is available.
  EFI_UNSUPPORTED       - The command described by the SCSI Request Packet
                          is not supported by the SCSI initiator(i.e., SCSI 
                          Host Controller). The SCSI Request Packet was not
                          sent, so no additional status information is 
                          available.
  EFI_TIMEOUT           - A timeout occurred while waiting for the SCSI 
                          Request Packet to execute. See HostAdapterStatus,
                          TargetStatus, SenseDataLength, and SenseData in 
                          that order for additional status information.

--*/
;

EFI_STATUS
ScsiScanCreateDevice (
  EFI_DRIVER_BINDING_PROTOCOL   *This,
  EFI_HANDLE                    Controller,
  SCSI_TARGET_ID                *TargetId,
  UINT64                        Lun,
  SCSI_BUS_DEVICE              *ScsiBusDev
  )
/*++

Routine Description:

  Scan SCSI Bus to discover the device, and attach ScsiIoProtocol to it.

Arguments:

  This              - Protocol instance pointer
  Controller        - Controller handle
  Pun               - The Pun of the SCSI device on the SCSI channel.
  Lun               - The Lun of the SCSI device on the SCSI channel.
  ScsiBusDev        - The pointer of SCSI_BUS_DEVICE

Returns:

  EFI_SUCCESS       - Successfully to discover the device and attach ScsiIoProtocol to it.
  EFI_OUT_OF_RESOURCES - Fail to discover the device.

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
