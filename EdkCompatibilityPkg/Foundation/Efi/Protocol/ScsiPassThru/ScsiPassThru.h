/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ScsiPassThru.h

Abstract:

  SCSI Pass Through protocol.

--*/

#ifndef __SCSI_PT_H__
#define __SCSI_PT_H__

#define EFI_SCSI_PASS_THRU_PROTOCOL_GUID \
  { \
    0xa59e8fcf, 0xbda0, 0x43bb, {0x90, 0xb1, 0xd3, 0x73, 0x2e, 0xca, 0xa8, 0x77} \
  }

//
// Forward reference for pure ANSI compatability
//
EFI_FORWARD_DECLARATION (EFI_SCSI_PASS_THRU_PROTOCOL);

#define EFI_SCSI_PASS_THRU_ATTRIBUTES_PHYSICAL    0x0001
#define EFI_SCSI_PASS_THRU_ATTRIBUTES_LOGICAL     0x0002
#define EFI_SCSI_PASS_THRU_ATTRIBUTES_NONBLOCKIO  0x0004

//
// SCSI Host Adapter Status definition
//
#define EFI_SCSI_STATUS_HOST_ADAPTER_OK                     0x00
#define EFI_SCSI_STATUS_HOST_ADAPTER_TIMEOUT_COMMAND        0x09  // timeout when processing the command
#define EFI_SCSI_STATUS_HOST_ADAPTER_TIMEOUT                0x0b  // timeout when waiting for the command processing
#define EFI_SCSI_STATUS_HOST_ADAPTER_MESSAGE_REJECT         0x0d  // a message reject was received when processing command
#define EFI_SCSI_STATUS_HOST_ADAPTER_BUS_RESET              0x0e  // a bus reset was detected
#define EFI_SCSI_STATUS_HOST_ADAPTER_PARITY_ERROR           0x0f
#define EFI_SCSI_STATUS_HOST_ADAPTER_REQUEST_SENSE_FAILED   0x10  // the adapter failed in issuing request sense command
#define EFI_SCSI_STATUS_HOST_ADAPTER_SELECTION_TIMEOUT      0x11  // selection timeout
#define EFI_SCSI_STATUS_HOST_ADAPTER_DATA_OVERRUN_UNDERRUN  0x12  // data overrun or data underrun
#define EFI_SCSI_STATUS_HOST_ADAPTER_BUS_FREE               0x13  // Unexepected bus free
#define EFI_SCSI_STATUS_HOST_ADAPTER_PHASE_ERROR            0x14  // Target bus phase sequence failure
#define EFI_SCSI_STATUS_HOST_ADAPTER_OTHER                  0x7f

//
// SCSI Target Status definition
//
#define EFI_SCSI_STATUS_TARGET_GOOD                       0x00
#define EFI_SCSI_STATUS_TARGET_CHECK_CONDITION            0x02  // check condition
#define EFI_SCSI_STATUS_TARGET_CONDITION_MET              0x04  // condition met
#define EFI_SCSI_STATUS_TARGET_BUSY                       0x08  // busy
#define EFI_SCSI_STATUS_TARGET_INTERMEDIATE               0x10  // intermediate
#define EFI_SCSI_STATUS_TARGET_INTERMEDIATE_CONDITION_MET 0x14  // intermediate-condition met
#define EFI_SCSI_STATUS_TARGET_RESERVATION_CONFLICT       0x18  // reservation conflict
#define EFI_SCSI_STATUS_TARGET_COMMOND_TERMINATED         0x22  // command terminated
#define EFI_SCSI_STATUS_TARGET_QUEUE_FULL                 0x28  // queue full
typedef struct {
  UINT64  Timeout;
  VOID    *DataBuffer;
  VOID    *SenseData;
  VOID    *Cdb;
  UINT32  TransferLength;
  UINT8   CdbLength;
  UINT8   DataDirection;
  UINT8   HostAdapterStatus;
  UINT8   TargetStatus;
  UINT8   SenseDataLength;
} EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET;

typedef struct {
  CHAR16  *ControllerName;
  CHAR16  *ChannelName;
  UINT32  AdapterId;
  UINT32  Attributes;
  UINT32  IoAlign;
} EFI_SCSI_PASS_THRU_MODE;

typedef
EFI_STATUS
(EFIAPI *EFI_SCSI_PASS_THRU_PASSTHRU) (
  IN EFI_SCSI_PASS_THRU_PROTOCOL                          * This,
  IN UINT32                                               Target,
  IN UINT64                                               Lun,
  IN OUT EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET           * Packet,
  IN EFI_EVENT                                            Event   OPTIONAL
  )
/*++

  Routine Description:
    Sends a SCSI Request Packet to a SCSI device that is attached to 
    the SCSI channel. This function supports both blocking I/O and 
    non-blocking I/O.  The blocking I/O functionality is required, 
    and the non-blocking I/O functionality is optional.

  Arguments:
    This                  - Protocol instance pointer.
    Target                - The Target ID of the SCSI device to 
                            send the SCSI Request Packet.
    Lun                   - The LUN of the SCSI device to send the 
                            SCSI Request Packet. 
    Packet                - A pointer to the SCSI Request Packet to send 
                            to the SCSI device specified by Target and Lun.
    Event                 - If non-blocking I/O is not supported then Event 
                            is ignored, and blocking I/O is performed.  
                            If Event is NULL, then blocking I/O is performed.
                            If Event is not NULL and non blocking I/O is 
                            supported, then non-blocking I/O is performed, 
                            and Event will be signaled when the SCSI Request 
                            Packet completes

  Returns:
    EFI_SUCCESSThe        - SCSI Request Packet was sent by the host, and 
                            TransferLength bytes were transferred to/from 
                            DataBuffer.See HostAdapterStatus, TargetStatus,
                            SenseDataLength,and SenseData in that order 
                            for additional status information.
    EFI_BAD_BUFFER_SIZE   - The SCSI Request Packet was executed, but the 
                            entire DataBuffer could not be transferred.
                            The actual number of bytes transferred is returned
                            in TransferLength. See HostAdapterStatus, 
                            TargetStatus, SenseDataLength, and SenseData in 
                            that order for additional status information.
    EFI_NOT_READY          - The SCSI Request Packet could not be sent because
                            there are too many SCSI Request Packets already 
                            queued.  The caller may retry again later.
    EFI_DEVICE_ERROR      - A device error occurred while attempting to send 
                            the SCSI Request Packet. See HostAdapterStatus, 
                            TargetStatus, SenseDataLength, and SenseData in 
                            that order for additional status information.
    EFI_INVALID_PARAMETER  - Target, Lun, or the contents of ScsiRequestPacket
                            are invalid. The SCSI Request Packet was not sent,
                            so no additional status information is available.
    EFI_UNSUPPORTED        - The command described by the SCSI Request Packet 
                            is not supported by the host adapter. The SCSI 
                            Request Packet was not sent, so no additional 
                            status information is available.
    EFI_TIMEOUT            - A timeout occurred while waiting for the SCSI 
                            Request Packet to execute. See HostAdapterStatus,
                            TargetStatus, SenseDataLength, and SenseData in 
                            that order for additional status information.

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SCSI_PASS_THRU_GET_NEXT_DEVICE) (
  IN EFI_SCSI_PASS_THRU_PROTOCOL            * This,
  IN OUT UINT32                             *Target,
  IN OUT UINT64                             *Lun
  )
/*++

  Routine Description:
    Used to retrieve the list of legal Target IDs for SCSI devices 
    on a SCSI channel.

  Arguments:
    This                  - Protocol instance pointer.
    Target                - On input, a pointer to the Target ID of a 
                            SCSI device present on the SCSI channel.
                            On output, a pointer to the Target ID of 
                            the next SCSI device present on a SCSI channel.
                            An input value of 0xFFFFFFFF retrieves the 
                            Target ID of the first SCSI device present on 
                            a SCSI channel.
    Lun                   - On input, a pointer to the LUN of a SCSI device
                            present on the SCSI channel.On output, a pointer
                            to the LUN of the next SCSI device present on a 
                            SCSI channel. 
  Returns:
    EFI_SUCCESS            - The Target ID of the next SCSI device on the SCSI
                            channel was returned in Target and Lun.
    EFI_NOT_FOUND          - There are no more SCSI devices on this SCSI channel.
    EFI_INVALID_PARAMETER  - Target is not 0xFFFFFFFF, and Target and Lun were 
                            not returned on a previous call to GetNextDevice().
--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SCSI_PASS_THRU_BUILD_DEVICE_PATH) (
  IN EFI_SCSI_PASS_THRU_PROTOCOL            * This,
  IN     UINT32                             Target,
  IN     UINT64                             Lun,
  IN OUT EFI_DEVICE_PATH_PROTOCOL           **DevicePath
  )
/*++

  Routine Description:
    Used to allocate and build a device path node for a SCSI device 
    on a SCSI channel.

  Arguments:
    This                  - Protocol instance pointer.
    Target                - The Target ID of the SCSI device for which
                            a device path node is to be allocated and built.
    Lun                    - The LUN of the SCSI device for which a device 
                            path node is to be allocated and built.
    DevicePath            - A pointer to a single device path node that 
                            describes the SCSI device specified by 
                            Target and Lun. This function is responsible 
                            for allocating the buffer DevicePath with the boot
                            service AllocatePool().  It is the caller's 
                            responsibility to free DevicePath when the caller
                            is finished with DevicePath.    
  Returns:
    EFI_SUCCESS            - The device path node that describes the SCSI device
                            specified by Target and Lun was allocated and 
                            returned in DevicePath.
    EFI_NOT_FOUND          - The SCSI devices specified by Target and Lun does
                            not exist on the SCSI channel.
    EFI_INVALID_PARAMETER  - DevicePath is NULL.
    EFI_OUT_OF_RESOURCES  - There are not enough resources to allocate 
                            DevicePath.
--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SCSI_PASS_THRU_GET_TARGET_LUN) (
  IN EFI_SCSI_PASS_THRU_PROTOCOL            * This,
  IN  EFI_DEVICE_PATH_PROTOCOL              * DevicePath,
  OUT UINT32                                *Target,
  OUT UINT64                                *Lun
  )
/*++

  Routine Description:
    Used to translate a device path node to a Target ID and LUN.

  Arguments:
    This                  - Protocol instance pointer.
    DevicePath            - A pointer to the device path node that 
                            describes a SCSI device on the SCSI channel.
    Target                - A pointer to the Target ID of a SCSI device 
                            on the SCSI channel. 
    Lun                    - A pointer to the LUN of a SCSI device on 
                            the SCSI channel.    
  Returns:
    EFI_SUCCESS            - DevicePath was successfully translated to a 
                            Target ID and LUN, and they were returned 
                            in Target and Lun.
    EFI_INVALID_PARAMETER  - DevicePath is NULL.
    EFI_INVALID_PARAMETER  - Target is NULL.
    EFI_INVALID_PARAMETER  - Lun is NULL.
    EFI_UNSUPPORTED        - This driver does not support the device path 
                            node type in DevicePath.
    EFI_NOT_FOUND          - A valid translation from DevicePath to a 
                            Target ID and LUN does not exist.
--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SCSI_PASS_THRU_RESET_CHANNEL) (
  IN EFI_SCSI_PASS_THRU_PROTOCOL             * This
  )
/*++

  Routine Description:
    Resets a SCSI channel.This operation resets all the 
    SCSI devices connected to the SCSI channel.

  Arguments:
    This                  - Protocol instance pointer.
    
  Returns:
    EFI_SUCCESS            - The SCSI channel was reset.
    EFI_UNSUPPORTED        - The SCSI channel does not support 
                            a channel reset operation.
    EFI_DEVICE_ERROR      - A device error occurred while 
                            attempting to reset the SCSI channel.
    EFI_TIMEOUT            - A timeout occurred while attempting 
                            to reset the SCSI channel.
--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SCSI_PASS_THRU_RESET_TARGET) (
  IN EFI_SCSI_PASS_THRU_PROTOCOL             * This,
  IN UINT32                                  Target,
  IN UINT64                                  Lun
  )
/*++

  Routine Description:
    Resets a SCSI device that is connected to a SCSI channel.

  Arguments:
    This                  - Protocol instance pointer.
    Target                - The Target ID of the SCSI device to reset. 
    Lun                    - The LUN of the SCSI device to reset.
        
  Returns:
    EFI_SUCCESS            - The SCSI device specified by Target and 
                            Lun was reset.
    EFI_UNSUPPORTED        - The SCSI channel does not support a target
                            reset operation.
    EFI_INVALID_PARAMETER  - Target or Lun are invalid.
    EFI_DEVICE_ERROR      - A device error occurred while attempting 
                            to reset the SCSI device specified by Target 
                            and Lun.
    EFI_TIMEOUT            - A timeout occurred while attempting to reset 
                            the SCSI device specified by Target and Lun.
--*/
;

struct _EFI_SCSI_PASS_THRU_PROTOCOL {
  EFI_SCSI_PASS_THRU_MODE               *Mode;
  EFI_SCSI_PASS_THRU_PASSTHRU           PassThru;
  EFI_SCSI_PASS_THRU_GET_NEXT_DEVICE    GetNextDevice;
  EFI_SCSI_PASS_THRU_BUILD_DEVICE_PATH  BuildDevicePath;
  EFI_SCSI_PASS_THRU_GET_TARGET_LUN     GetTargetLun;
  EFI_SCSI_PASS_THRU_RESET_CHANNEL      ResetChannel;
  EFI_SCSI_PASS_THRU_RESET_TARGET       ResetTarget;
};

extern EFI_GUID gEfiScsiPassThruProtocolGuid;

#endif
