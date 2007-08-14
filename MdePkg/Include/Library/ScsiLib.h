/** @file
  Common Libarary for SCSI

  Copyright (c) 2006 - 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _SCSI_LIB_H
#define _SCSI_LIB_H

#include <Protocol/ScsiIo.h>

//
// the time unit is 100ns, since the SCSI I/O defines timeout in 100ns unit.
//
#define EFI_SCSI_STALL_1_MICROSECOND  10
#define EFI_SCSI_STALL_1_MILLISECOND  10000
#define EFI_SCSI_STALL_1_SECOND       10000000

//
// this macro cannot be directly used by the gBS->Stall(),
// since the value output by this macro is in 100ns unit,
// not 1us unit (1us = 1000ns)
//
#define EfiScsiStallSeconds(a)  (a) * EFI_SCSI_STALL_1_SECOND


/**
  Function test the ready status of the SCSI unit.

  @param[in]     ScsiIo             A pointer to SCSI IO protocol.
  @param[in]     Timeout            The length of timeout period.
  @param[out]    SenseData          A pointer to output sense data.
  @param[out]    SenseDataLength    The length of output sense data.
  @param[out]    HostAdapterStatus  The status of Host Adapter.
  @param[out]    TargetStatus       The status of the target.

  @retval EFI_SUCCESS           The status of the unit is tested successfully.
  @retval EFI_BAD_BUFFER_SIZE   The SCSI Request Packet was executed, 
                                but the entire DataBuffer could not be transferred.
                                The actual number of bytes transferred is returned
                                in InTransferLength.
  @retval EFI_NOT_READY         The SCSI Request Packet could not be sent because 
                                there are too many SCSI Command Packets already 
                                queued.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to send 
                                the SCSI Request Packet.
  @retval EFI_INVALID_PARAMETER The contents of CommandPacket are invalid.  
  @retval EFI_UNSUPPORTED       The command described by the SCSI Request Packet
                                is not supported by the SCSI initiator(i.e., SCSI 
                                Host Controller).
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the SCSI 
                                Request Packet to execute.

**/
EFI_STATUS
ScsiTestUnitReadyCommand (
  IN  EFI_SCSI_IO_PROTOCOL  *ScsiIo,
  IN  UINT64                Timeout,
  OUT VOID                  *SenseData,
  OUT UINT8                 *SenseDataLength,
  OUT UINT8                 *HostAdapterStatus,
  OUT UINT8                 *TargetStatus
  );


/**
  Function to submit SCSI inquiry command.

  @param[in]     ScsiIo             SCSI IO Protocol to use
  @param[in]     Timeout            The length of timeout period.
  @param[out]    SenseData          A pointer to output sense data.
  @param[in,out] SenseDataLength    The length of output sense data.
  @param[out]    HostAdapterStatus  The status of Host Adapter.
  @param[out]    TargetStatus       The status of the target.
  @param[in,out] InquirydataBuffer  A pointer to inquiry data buffer.
  @param[in,out] InquiryDataLength  The length of inquiry data buffer.
  @param[in]     EnableVitalProductData  Boolean to enable Vital Product Data.

  @retval EFI_SUCCESS         The status of the unit is tested successfully.
  @retval EFI_BAD_BUFFER_SIZE The SCSI Request Packet was executed, 
                              but the entire DataBuffer could not be transferred.
                              The actual number of bytes transferred is returned
                              in TransferLength.
  @retval EFI_NOT_READY   The SCSI Request Packet could not be sent because 
                          there are too many SCSI Command Packets already 
                          queued.
  @retval EFI_DEVICE_ERROR  A device error occurred while attempting to send 
                            the SCSI Request Packet.
  @retval EFI_INVALID_PARAMETER The contents of CommandPacket are invalid.  
  @retval EFI_UNSUPPORTED The command described by the SCSI Request Packet
                          is not supported by the SCSI initiator(i.e., SCSI 
                          Host Controller).
  @retval EFI_TIMEOUT     A timeout occurred while waiting for the SCSI 
                          Request Packet to execute.

**/
EFI_STATUS
ScsiInquiryCommand (
  IN  EFI_SCSI_IO_PROTOCOL  *ScsiIo,
  IN  UINT64                Timeout,
  IN  VOID                  *SenseData,
  IN OUT UINT8              *SenseDataLength,
  OUT UINT8                 *HostAdapterStatus,
  OUT UINT8                 *TargetStatus,
  IN OUT VOID               *InquiryDataBuffer,
  IN OUT UINT32             *InquiryDataLength,
  IN  BOOLEAN               EnableVitalProductData
  );


/**
  Function to submit SCSI mode sense 10 command.

  @param[in]     ScsiIo             A pointer to SCSI IO protocol.
  @param[in]     Timeout            The length of timeout period.
  @param[out]    SenseData          A pointer to output sense data.
  @param[in,out] SenseDataLength    The length of output sense data.
  @param[out]    HostAdapterStatus  The status of Host Adapter.
  @param[out]    TargetStatus       The status of the target.
  @param[in]     DataBuffer         A pointer to input data buffer.
  @param[in,out] DataLength         The length of input data buffer.
  @param[in]     DBDField           The DBD Field (Optional).
  @param[in]     PageControl        Page Control.
  @param[in]     PageCode           Page code.

  @retval EFI_SUCCESS               The status of the unit is tested successfully.
  @retval EFI_WARN_BUFFER_TOO_SMALL The SCSI Request Packet was executed, 
                                    but the entire DataBuffer could not be transferred.
                                    The actual number of bytes transferred is returned
                                    in TransferLength.
  @retval EFI_NOT_READY             The SCSI Request Packet could not be sent because 
                                    there are too many SCSI Command Packets already 
                                    queued.
  @retval EFI_DEVICE_ERROR          A device error occurred while attempting to send 
                                    the SCSI Request Packet.
  @retval EFI_INVALID_PARAMETER     The contents of CommandPacket are invalid.  
  @retval EFI_UNSUPPORTED           The command described by the SCSI Request Packet
                                    is not supported by the SCSI initiator(i.e., SCSI 
                                    Host Controller).
  @retval EFI_TIMEOUT               A timeout occurred while waiting for the SCSI 
                                    Request Packet to execute.

**/
EFI_STATUS
ScsiModeSense10Command (
  IN  EFI_SCSI_IO_PROTOCOL    *ScsiIo,
  IN  UINT64                  Timeout,
  IN  VOID                    *SenseData,
  IN OUT UINT8                *SenseDataLength,
  OUT UINT8                   *HostAdapterStatus,
  OUT UINT8                   *TargetStatus,
  IN  VOID                    *DataBuffer,
  IN OUT UINT32               *DataLength,
  IN  UINT8                   DBDField, OPTIONAL
  IN  UINT8                   PageControl,
  IN  UINT8                   PageCode
  );



/**
  Function to submit SCSI request sense command.
  ScsiIo               - A pointer to SCSI IO protocol.
  Timeout              - The length of timeout period.
  SenseData            - A pointer to output sense data.
  SenseDataLength      - The length of output sense data.
  HostAdapterStatus    - The status of Host Adapter.
  TargetStatus         - The status of the target.

  @param[in]     ScsiIo             SCSI IO Protocol to use
  @param[in]     Timeout            TODO:
  @param[out]    SenseData          TODO:
  @param[in,out] SenseDataLength    TODO:
  @param[out]    HostAdapterStatus  TODO:
  @param[out]    TargetStatus       TODO:

  @retval EFI_SUCCESS        Valid data returned
  @retval EFI_SUCCESS        The status of the unit is tested successfully.
  @retval EFI_WARN_BUFFER_TOO_SMALL The SCSI Request Packet was executed, 
                                    but the entire DataBuffer could not be transferred.
                                    The actual number of bytes transferred is returned
                                    in TransferLength.
  @retval EFI_NOT_READY             The SCSI Request Packet could not be sent because 
                                    there are too many SCSI Command Packets already 
                                    queued.
  @retval EFI_DEVICE_ERROR          A device error occurred while attempting to send 
                                    the SCSI Request Packet.
  @retval EFI_INVALID_PARAMETER     The contents of CommandPacket are invalid.  
  @retval EFI_UNSUPPORTED           The command described by the SCSI Request Packet
                                    is not supported by the SCSI initiator(i.e., SCSI 
                                    Host Controller).
  @retval EFI_TIMEOUT               A timeout occurred while waiting for the SCSI 
                                    Request Packet to execute.

**/
EFI_STATUS
ScsiRequestSenseCommand (
  IN  EFI_SCSI_IO_PROTOCOL  *ScsiIo,
  IN  UINT64                Timeout,
  IN  VOID                  *SenseData,
  IN OUT UINT8              *SenseDataLength,
  OUT UINT8                 *HostAdapterStatus,
  OUT UINT8                 *TargetStatus
  );


/**
  Function to submit read capacity command.

  @param[in]     ScsiIo             A pointer to SCSI IO protocol.
  @param[in]     Timeout            The length of timeout period.
  @param[out]    SenseData          A pointer to output sense data.
  @param[in,out] SenseDataLength    The length of output sense data.
  @param[out]    HostAdapterStatus  The status of Host Adapter.
  @param[out]    TargetStatus       The status of the target.
  @param[out]    DataBuffer         A pointer to a data buffer.
  @param[in,out] DataLength         The length of data buffer.
  @param[in]     PMI                Partial medium indicator.

  @retval  EFI_SUCCESS                The status of the unit is tested successfully.
  @retval  EFI_WARN_BUFFER_TOO_SMALL  The SCSI Request Packet was executed, 
                                      but the entire DataBuffer could not be transferred.
                                      The actual number of bytes transferred is returned
                                      in TransferLength.
  @retval  EFI_NOT_READY              The SCSI Request Packet could not be sent because 
                                      there are too many SCSI Command Packets already 
                                      queued.
  @retval  EFI_DEVICE_ERROR           A device error occurred while attempting to send 
                                      the SCSI Request Packet.
  @retval  EFI_INVALID_PARAMETER      The contents of CommandPacket are invalid.  
  @retval  EFI_UNSUPPORTED            The command described by the SCSI Request Packet
                                      is not supported by the SCSI initiator(i.e., SCSI 
                                      Host Controller).
  @retval  EFI_TIMEOUT                A timeout occurred while waiting for the SCSI 
                                      Request Packet to execute.

**/
EFI_STATUS
ScsiReadCapacityCommand (
  IN  EFI_SCSI_IO_PROTOCOL  *ScsiIo,
  IN  UINT64                Timeout,
  IN  VOID                  *SenseData,
  IN OUT UINT8              *SenseDataLength,
  OUT UINT8                 *HostAdapterStatus,
  OUT UINT8                 *TargetStatus,
  OUT VOID                  *DataBuffer,
  IN OUT UINT32             *DataLength,
  IN  BOOLEAN               PMI
  );


/**
  Function to submit read 10 command.

  @param[in]     ScsiIo             A pointer to SCSI IO protocol.
  @param[in]     Timeout            The length of timeout period.
  @param[out]    SenseData          A pointer to output sense data.
  @param[in,out] SenseDataLength    The length of output sense data.
  @param[out]    HostAdapterStatus  The status of Host Adapter.
  @param[out]    TargetStatus       The status of the target.
  @param[out]    DataBuffer         Read 10 command data.
  @param[in,out] DataLength         The length of data buffer.
  @param[in]     StartLba           The start address of LBA.
  @param[in]     SectorSize         The sector size.

  @retval EFI_SUCCESS               The status of the unit is tested successfully.
  @retval EFI_WARN_BUFFER_TOO_SMALL The SCSI Request Packet was executed, 
                                    but the entire DataBuffer could not be transferred.
                                    The actual number of bytes transferred is returned
                                    in TransferLength.
  @retval EFI_NOT_READY             The SCSI Request Packet could not be sent because 
                                    there are too many SCSI Command Packets already 
                                    queued.
  @retval EFI_DEVICE_ERROR          A device error occurred while attempting to send 
                                    the SCSI Request Packet.
  @retval EFI_INVALID_PARAMETER     The contents of CommandPacket are invalid.  
  @retval EFI_UNSUPPORTED           The command described by the SCSI Request Packet
                                    is not supported by the SCSI initiator(i.e., SCSI 
                                    Host Controller).
  @retval EFI_TIMEOUT               A timeout occurred while waiting for the SCSI 
                                    Request Packet to execute.

**/
EFI_STATUS
ScsiRead10Command (
  IN  EFI_SCSI_IO_PROTOCOL  *ScsiIo,
  IN  UINT64                Timeout,
  IN  VOID                  *SenseData,
  IN OUT UINT8              *SenseDataLength,
  OUT UINT8                 *HostAdapterStatus,
  OUT UINT8                 *TargetStatus,
  OUT VOID                  *DataBuffer,
  IN OUT UINT32             *DataLength,
  IN  UINT32                StartLba,
  IN  UINT32                SectorSize
  );


/**
  Function to submit SCSI write 10 command.

  @param[in]     ScsiIo             SCSI IO Protocol to use
  @param[in]     Timeout            The length of timeout period.
  @param[out]    SenseData          A pointer to output sense data.
  @param[in,out] SenseDataLength    The length of output sense data.
  @param[out]    HostAdapterStatus  The status of Host Adapter.
  @param[out]    TargetStatus       The status of the target.
  @param[out]    DataBuffer         A pointer to a data buffer.
  @param[in,out] DataLength         The length of data buffer.
  @param[in]     StartLba           The start address of LBA.
  @param[in]     SectorSize         The sector size.

  @retval EFI_SUCCESS               The status of the unit is tested successfully.
  @retval EFI_WARN_BUFFER_TOO_SMALL The SCSI Request Packet was executed, 
                                    but the entire DataBuffer could not be transferred.
                                    The actual number of bytes transferred is returned
                                    in InTransferLength.
  @retval EFI_NOT_READY             The SCSI Request Packet could not be sent because 
                                    there are too many SCSI Command Packets already 
                                    queued.
  @retval EFI_DEVICE_ERROR          A device error occurred while attempting to send 
                                    the SCSI Request Packet.
  @retval EFI_INVALID_PARAMETER     The contents of CommandPacket are invalid.  
  @retval EFI_UNSUPPORTED           The command described by the SCSI Request Packet
                                    is not supported by the SCSI initiator(i.e., SCSI 
                                    Host Controller).
  @retval EFI_TIMEOUT               A timeout occurred while waiting for the SCSI 
                                    Request Packet to execute.

**/
EFI_STATUS
ScsiWrite10Command (
  IN  EFI_SCSI_IO_PROTOCOL  *ScsiIo,
  IN  UINT64                Timeout,
  IN  VOID                  *SenseData,
  IN OUT UINT8              *SenseDataLength,
  OUT UINT8                 *HostAdapterStatus,
  OUT UINT8                 *TargetStatus,
  OUT VOID                  *DataBuffer,
  IN OUT UINT32             *DataLength,
  IN  UINT32                StartLba,
  IN  UINT32                SectorSize
  );


#endif
