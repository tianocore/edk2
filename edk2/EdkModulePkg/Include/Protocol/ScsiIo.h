/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ScsiIo.h

Abstract:

  SCSI I/O protocol.

--*/

#ifndef __SCSI_IO_H__
#define __SCSI_IO_H__

#define EFI_SCSI_IO_PROTOCOL_GUID \
  { 0x403cd195, 0xf233, 0x48ec, {0x84, 0x55, 0xb2, 0xe5, 0x2f, 0x1d, 0x9e, 0x2  } }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_SCSI_IO_PROTOCOL  EFI_SCSI_IO_PROTOCOL;

//
// SCSI Host Adapter Status definition
//
#define EFI_SCSI_IO_STATUS_HOST_ADAPTER_OK                     0x00
#define EFI_SCSI_IO_STATUS_HOST_ADAPTER_TIMEOUT_COMMAND        0x09    // timeout when processing the command
#define EFI_SCSI_IO_STATUS_HOST_ADAPTER_TIMEOUT                0x0b    // timeout when waiting for the command processing
#define EFI_SCSI_IO_STATUS_HOST_ADAPTER_MESSAGE_REJECT         0x0d    // a message reject was received when processing command
#define EFI_SCSI_IO_STATUS_HOST_ADAPTER_BUS_RESET              0x0e    // a bus reset was detected
#define EFI_SCSI_IO_STATUS_HOST_ADAPTER_PARITY_ERROR           0x0f
#define EFI_SCSI_IO_STATUS_HOST_ADAPTER_REQUEST_SENSE_FAILED   0x10    // the adapter failed in issuing request sense command
#define EFI_SCSI_IO_STATUS_HOST_ADAPTER_SELECTION_TIMEOUT      0x11    // selection timeout
#define EFI_SCSI_IO_STATUS_HOST_ADAPTER_DATA_OVERRUN_UNDERRUN  0x12    // data overrun or data underrun
#define EFI_SCSI_IO_STATUS_HOST_ADAPTER_BUS_FREE               0x13    // Unexepected bus free
#define EFI_SCSI_IO_STATUS_HOST_ADAPTER_PHASE_ERROR            0x14    // Target bus phase sequence failure
#define EFI_SCSI_IO_STATUS_HOST_ADAPTER_OTHER                  0x7f


//
// SCSI Target Status definition
//
#define EFI_SCSI_IO_STATUS_TARGET_GOOD                         0x00
#define EFI_SCSI_IO_STATUS_TARGET_CHECK_CONDITION              0x02    // check condition
#define EFI_SCSI_IO_STATUS_TARGET_CONDITION_MET                0x04    // condition met
#define EFI_SCSI_IO_STATUS_TARGET_BUSY                         0x08    // busy
#define EFI_SCSI_IO_STATUS_TARGET_INTERMEDIATE                 0x10    // intermediate
#define EFI_SCSI_IO_STATUS_TARGET_INTERMEDIATE_CONDITION_MET   0x14    // intermediate-condition met
#define EFI_SCSI_IO_STATUS_TARGET_RESERVATION_CONFLICT         0x18    // reservation conflict
#define EFI_SCSI_IO_STATUS_TARGET_COMMOND_TERMINATED           0x22    // command terminated
#define EFI_SCSI_IO_STATUS_TARGET_QUEUE_FULL                   0x28    // queue full

typedef struct {
  UINT64      Timeout;
  VOID        *DataBuffer;
  VOID        *SenseData;
  VOID        *Cdb;
  UINT32      TransferLength;
  UINT8       CdbLength;
  UINT8       DataDirection;
  UINT8       HostAdapterStatus;
  UINT8       TargetStatus;
  UINT8       SenseDataLength;
}EFI_SCSI_IO_SCSI_REQUEST_PACKET;

typedef
EFI_STATUS
(EFIAPI *EFI_SCSI_IO_PROTOCOL_GET_DEVICE_TYPE) (
  IN EFI_SCSI_IO_PROTOCOL     *This,
  OUT UINT8                           *DeviceType
  )
/*++

  Routine Description:
    Retrieves the device type information of the SCSI Controller.
    
  Arguments:
    This                  - Protocol instance pointer.
    DeviceType            - A pointer to the device type information
                            retrieved from the SCSI Controller. 

  Returns:
    EFI_SUCCESS            - Retrieves the device type information successfully.
    EFI_INVALID_PARAMETER  - The DeviceType is NULL.
--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SCSI_IO_PROTOCOL_GET_DEVICE_LOCATION) (
  IN EFI_SCSI_IO_PROTOCOL   *This,
  OUT UINT32                        *Target,
  OUT UINT64                        *Lun
  )
/*++
  Routine Description:
    Retrieves the device location in the SCSI channel.
    
  Arguments:
    This                  - Protocol instance pointer.
    Target                - A pointer to the Target ID of a SCSI device 
                            on the SCSI channel. 
    Lun                    - A pointer to the LUN of the SCSI device on 
                            the SCSI channel.

  Returns:
    EFI_SUCCESS            - Retrieves the device location successfully.
    EFI_INVALID_PARAMETER  - The Target or Lun is NULL.
--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SCSI_IO_PROTOCOL_RESET_BUS) (
  IN EFI_SCSI_IO_PROTOCOL     *This
  )
/*++

  Routine Description:
    Resets the SCSI Bus that the SCSI Controller is attached to.
    
  Arguments:
    This                  - Protocol instance pointer.

  Returns:
    EFI_SUCCESS            - The SCSI bus is reset successfully.
    EFI_DEVICE_ERROR      - Errors encountered when resetting the SCSI bus.
    EFI_UNSUPPORTED        - The bus reset operation is not supported by the
                            SCSI Host Controller.
    EFI_TIMEOUT            - A timeout occurred while attempting to reset 
                            the SCSI bus.
--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SCSI_IO_PROTOCOL_RESET_DEVICE) (
  IN EFI_SCSI_IO_PROTOCOL     *This
  )
/*++

  Routine Description:
    Resets the SCSI Controller that the device handle specifies.
    
  Arguments:
    This                  - Protocol instance pointer.
    

  Returns:
    EFI_SUCCESS            - Reset the SCSI controller successfully.
    EFI_DEVICE_ERROR      - Errors are encountered when resetting the
                            SCSI Controller.
    EFI_UNSUPPORTED        - The SCSI bus does not support a device 
                            reset operation.
    EFI_TIMEOUT            - A timeout occurred while attempting to 
                            reset the SCSI Controller.
--*/
;


typedef
EFI_STATUS
(EFIAPI *EFI_SCSI_IO_PROTOCOL_EXEC_SCSI_CMD) (
  IN EFI_SCSI_IO_PROTOCOL           *This,
  IN OUT  EFI_SCSI_IO_SCSI_REQUEST_PACKET   *Packet,
  IN EFI_EVENT                              Event  OPTIONAL
  )
/*++

  Routine Description:
    Sends a SCSI Request Packet to the SCSI Controller for execution.
    
  Arguments:
    This                  - Protocol instance pointer.
    Packet                - The SCSI request packet to send to the SCSI 
                            Controller specified by the device handle.
    Event                  - If the SCSI bus where the SCSI device is attached
                            does not support non-blocking I/O, then Event is 
                            ignored, and blocking I/O is performed.  
                            If Event is NULL, then blocking I/O is performed.
                            If Event is not NULL and non-blocking I/O is 
                            supported, then non-blocking I/O is performed,
                            and Event will be signaled when the SCSI Request
                            Packet completes.
  Returns:
    EFI_SUCCESS            - The SCSI Request Packet was sent by the host 
                            successfully, and TransferLength bytes were 
                            transferred to/from DataBuffer.See 
                            HostAdapterStatus, TargetStatus, 
                            SenseDataLength, and SenseData in that order
                            for additional status information.
    EFI_WARN_BUFFER_TOO_SMALL  - The SCSI Request Packet was executed, 
                            but the entire DataBuffer could not be transferred.
                            The actual number of bytes transferred is returned
                            in TransferLength. See HostAdapterStatus, 
                            TargetStatus, SenseDataLength, and SenseData in 
                            that order for additional status information.
    EFI_NOT_READY          - The SCSI Request Packet could not be sent because 
                            there are too many SCSI Command Packets already 
                            queued.The caller may retry again later.
    EFI_DEVICE_ERROR      - A device error occurred while attempting to send 
                            the SCSI Request Packet. See HostAdapterStatus, 
                            TargetStatus, SenseDataLength, and SenseData in 
                            that order for additional status information.
    EFI_INVALID_PARAMETER  - The contents of CommandPacket are invalid.  
                            The SCSI Request Packet was not sent, so no 
                            additional status information is available.
    EFI_UNSUPPORTED        - The command described by the SCSI Request Packet
                            is not supported by the SCSI initiator(i.e., SCSI 
                            Host Controller). The SCSI Request Packet was not
                            sent, so no additional status information is 
                            available.
    EFI_TIMEOUT            - A timeout occurred while waiting for the SCSI 
                            Request Packet to execute. See HostAdapterStatus,
                            TargetStatus, SenseDataLength, and SenseData in 
                            that order for additional status information.
--*/
;

struct _EFI_SCSI_IO_PROTOCOL {
    EFI_SCSI_IO_PROTOCOL_GET_DEVICE_TYPE      GetDeviceType;
    EFI_SCSI_IO_PROTOCOL_GET_DEVICE_LOCATION  GetDeviceLocation;
    EFI_SCSI_IO_PROTOCOL_RESET_BUS            ResetBus;
    EFI_SCSI_IO_PROTOCOL_RESET_DEVICE         ResetDevice;
    EFI_SCSI_IO_PROTOCOL_EXEC_SCSI_CMD        ExecuteSCSICommand;    
};

extern EFI_GUID gEfiScsiIoProtocolGuid;

#endif

