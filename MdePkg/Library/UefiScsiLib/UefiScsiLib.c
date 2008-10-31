/** @file
  UEFI SCSI Library implementation

  Copyright (c) 2006 - 2008, Intel Corporation.<BR>
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/


#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/ScsiLib.h>
#include <Library/BaseMemoryLib.h>

#include <IndustryStandard/Scsi.h>

//
// Max bytes needed to represent ID of a SCSI device
//
#define EFI_SCSI_TARGET_MAX_BYTES (0x10)

//
// bit5..7 are for Logical unit number
// 11100000b (0xe0)
//
#define EFI_SCSI_LOGICAL_UNIT_NUMBER_MASK 0xe0

//
// Scsi Command Length six or ten
//
#define EFI_SCSI_OP_LENGTH_SIX 0x6
#define EFI_SCSI_OP_LENGTH_TEN 0xa

/**
  Function test the ready status of the SCSI unit.

  Submit SCSI test unit ready command with SCSI request packet specified by this scsi command, TimeOut
  and SenseData, then get the status of the target Scsi unit.

  If SenseDataLength is NULL, then ASSERT().

  If HostAdapterStatus is NULL, then ASSERT().

  If TargetStatus is NULL, then ASSERT().

  @param[in]     ScsiIo             A pointer to SCSI IO protocol.
  @param[in]     Timeout            The length of timeout period.
  @param[in]     SenseData          A pointer to output sense data.
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
  @retval EFI_INVALID_PARAMETER ScsiIo is NULL.
  @retval EFI_UNSUPPORTED       The command described by the SCSI Request Packet
                                is not supported by the SCSI initiator(i.e., SCSI 
                                Host Controller).
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the SCSI 
                                Request Packet to execute.

**/
EFI_STATUS
EFIAPI
ScsiTestUnitReadyCommand (
  IN  EFI_SCSI_IO_PROTOCOL  *ScsiIo,
  IN  UINT64                Timeout,
  IN  VOID                  *SenseData,
  OUT UINT8                 *SenseDataLength,
  OUT UINT8                 *HostAdapterStatus,
  OUT UINT8                 *TargetStatus
  )
{
  EFI_SCSI_IO_SCSI_REQUEST_PACKET CommandPacket;
  UINT64                          Lun;
  UINT8                           *Target;
  UINT8                           TargetArray[EFI_SCSI_TARGET_MAX_BYTES];
  EFI_STATUS                      Status;
  UINT8                           Cdb[EFI_SCSI_OP_LENGTH_SIX];

  ASSERT (SenseDataLength != NULL);
  ASSERT (HostAdapterStatus != NULL);
  ASSERT (TargetStatus != NULL);
  
  if (ScsiIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&CommandPacket, sizeof (EFI_SCSI_IO_SCSI_REQUEST_PACKET));
  ZeroMem (Cdb, EFI_SCSI_OP_LENGTH_SIX);

  CommandPacket.Timeout         = Timeout;
  CommandPacket.InDataBuffer    = NULL;
  CommandPacket.InTransferLength= 0;
  CommandPacket.OutDataBuffer    = NULL;
  CommandPacket.OutTransferLength= 0;
  CommandPacket.SenseData       = SenseData;
  CommandPacket.Cdb             = Cdb;
  //
  // Fill Cdb for Test Unit Ready Command
  //
  Target = &TargetArray[0];
  ScsiIo->GetDeviceLocation (ScsiIo, &Target, &Lun);

  Cdb[0]                        = EFI_SCSI_OP_TEST_UNIT_READY;
  Cdb[1]                        = (UINT8) (Lun & EFI_SCSI_LOGICAL_UNIT_NUMBER_MASK);
  CommandPacket.CdbLength       = (UINT8) EFI_SCSI_OP_LENGTH_SIX;
  CommandPacket.SenseDataLength = *SenseDataLength;

  Status                        = ScsiIo->ExecuteScsiCommand (ScsiIo, &CommandPacket, NULL);

  *HostAdapterStatus            = CommandPacket.HostAdapterStatus;
  *TargetStatus                 = CommandPacket.TargetStatus;
  *SenseDataLength              = CommandPacket.SenseDataLength;

  return Status;
}


/**
  Function to submit SCSI inquiry command.

  Submit SCSI inquiry command with the SCSI request packet specified by this SCSI command and input
  parameters, then return the status of Scsi unit execution.

  If SenseDataLength is NULL, then ASSERT().

  If HostAdapterStatus is NULL, then ASSERT().

  If TargetStatus is NULL, then ASSERT().

  If InquiryDataLength is NULL, then ASSERT().

  @param[in]     ScsiIo             SCSI IO Protocol to use
  @param[in]     Timeout            The length of timeout period.
  @param[in]     SenseData          A pointer to output sense data.
  @param[in, out] SenseDataLength    The length of output sense data.
  @param[out]    HostAdapterStatus  The status of Host Adapter.
  @param[out]    TargetStatus       The status of the target.
  @param[in, out] InquiryDataBuffer  A pointer to inquiry data buffer.
  @param[in, out] InquiryDataLength  The length of inquiry data buffer.
  @param[in]     EnableVitalProductData  Boolean to enable Vital Product Data.

  @retval EFI_SUCCESS                Command is executed successfully.
  @retval EFI_BAD_BUFFER_SIZE        The SCSI Request Packet was executed, 
                                     but the entire DataBuffer could not be transferred.
                                     The actual number of bytes transferred is returned
                                     in TransferLength.
  @retval EFI_NOT_READY              The SCSI Request Packet could not be sent because 
                                     there are too many SCSI Command Packets already 
                                     queued.
  @retval EFI_DEVICE_ERROR           A device error occurred while attempting to send 
                                     the SCSI Request Packet.
  @retval EFI_INVALID_PARAMETER      ScsiIo is NULL.
  @retval EFI_UNSUPPORTED            The command described by the SCSI Request Packet
                                     is not supported by the SCSI initiator(i.e., SCSI 
                                     Host Controller).
  @retval EFI_TIMEOUT                A timeout occurred while waiting for the SCSI 
                                     Request Packet to execute.

**/
EFI_STATUS
EFIAPI
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
  )
{
  EFI_SCSI_IO_SCSI_REQUEST_PACKET CommandPacket;
  UINT64                          Lun;
  UINT8                           *Target;
  UINT8                           TargetArray[EFI_SCSI_TARGET_MAX_BYTES];
  EFI_STATUS                      Status;
  UINT8                           Cdb[EFI_SCSI_OP_LENGTH_SIX];

  ASSERT (SenseDataLength != NULL);
  ASSERT (HostAdapterStatus != NULL);
  ASSERT (TargetStatus != NULL);
  ASSERT (InquiryDataLength != NULL);
  
  if (ScsiIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&CommandPacket, sizeof (EFI_SCSI_IO_SCSI_REQUEST_PACKET));
  ZeroMem (Cdb, EFI_SCSI_OP_LENGTH_SIX);

  CommandPacket.Timeout         = Timeout;
  CommandPacket.InDataBuffer    = InquiryDataBuffer;
  CommandPacket.InTransferLength= *InquiryDataLength;
  CommandPacket.SenseData       = SenseData;
  CommandPacket.SenseDataLength = *SenseDataLength;
  CommandPacket.Cdb             = Cdb;

  Target = &TargetArray[0];
  ScsiIo->GetDeviceLocation (ScsiIo, &Target, &Lun);

  Cdb[0]  = EFI_SCSI_OP_INQUIRY;
  Cdb[1]  = (UINT8) (Lun & EFI_SCSI_LOGICAL_UNIT_NUMBER_MASK);
  if (EnableVitalProductData) {
    Cdb[1] |= 0x01;
  }

  if (*InquiryDataLength > 0xff) {
    *InquiryDataLength = 0xff;
  }

  Cdb[4]                      = (UINT8) (*InquiryDataLength);
  CommandPacket.CdbLength     = (UINT8) EFI_SCSI_OP_LENGTH_SIX;
  CommandPacket.DataDirection = EFI_SCSI_DATA_IN;

  Status                      = ScsiIo->ExecuteScsiCommand (ScsiIo, &CommandPacket, NULL);

  *HostAdapterStatus          = CommandPacket.HostAdapterStatus;
  *TargetStatus               = CommandPacket.TargetStatus;
  *SenseDataLength            = CommandPacket.SenseDataLength;
  *InquiryDataLength          = CommandPacket.InTransferLength;

  return Status;
}


/**
  Function to submit SCSI mode sense 10 command.

  Submit SCSI Mode Sense (10) command with the SCSI request packet specified by this SCSI command and
  the input parameters, then return the status of Scsi unit execution.

  If SenseDataLength is NULL, then ASSERT().

  If HostAdapterStatus is NULL, then ASSERT().

  If TargetStatus is NULL, then ASSERT().

  If DataLength is NULL, then ASSERT().

  @param[in]     ScsiIo             A pointer to SCSI IO protocol.
  @param[in]     Timeout            The length of timeout period.
  @param[in]     SenseData          A pointer to output sense data.
  @param[in, out] SenseDataLength    The length of output sense data.
  @param[out]    HostAdapterStatus  The status of Host Adapter.
  @param[out]    TargetStatus       The status of the target.
  @param[in]     DataBuffer         A pointer to input data buffer.
  @param[in, out] DataLength         The length of input data buffer.
  @param[in]     DBDField           The DBD Field (Optional).
  @param[in]     PageControl        Page Control.
  @param[in]     PageCode           Page code.

  @retval EFI_SUCCESS               The status of the unit is tested successfully.
  @retval EFI_BAD_BUFFER_SIZE       The SCSI Request Packet was executed, 
                                    but the entire DataBuffer could not be transferred.
                                    The actual number of bytes transferred is returned
                                    in TransferLength.
  @retval EFI_NOT_READY             The SCSI Request Packet could not be sent because 
                                    there are too many SCSI Command Packets already 
                                    queued.
  @retval EFI_DEVICE_ERROR          A device error occurred while attempting to send 
                                    the SCSI Request Packet.
  @retval EFI_INVALID_PARAMETER     ScsiIo is NULL. 
  @retval EFI_UNSUPPORTED           The command described by the SCSI Request Packet
                                    is not supported by the SCSI initiator(i.e., SCSI 
                                    Host Controller).
  @retval EFI_TIMEOUT               A timeout occurred while waiting for the SCSI 
                                    Request Packet to execute.

**/
EFI_STATUS
EFIAPI
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
  )
{
  EFI_SCSI_IO_SCSI_REQUEST_PACKET CommandPacket;
  UINT64                          Lun;
  UINT8                           *Target;
  UINT8                           TargetArray[EFI_SCSI_TARGET_MAX_BYTES];
  EFI_STATUS                      Status;
  UINT8                           Cdb[EFI_SCSI_OP_LENGTH_TEN];

  ASSERT (SenseDataLength != NULL);
  ASSERT (HostAdapterStatus != NULL);
  ASSERT (TargetStatus != NULL);
  ASSERT (DataLength != NULL);
  
  if (ScsiIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&CommandPacket, sizeof (EFI_SCSI_IO_SCSI_REQUEST_PACKET));
  ZeroMem (Cdb, EFI_SCSI_OP_LENGTH_TEN);

  CommandPacket.Timeout         = Timeout;
  CommandPacket.InDataBuffer    = DataBuffer;
  CommandPacket.SenseData       = SenseData;
  CommandPacket.InTransferLength= *DataLength;
  CommandPacket.Cdb             = Cdb;
  //
  // Fill Cdb for Mode Sense (10) Command
  //
  Target = &TargetArray[0];
  ScsiIo->GetDeviceLocation (ScsiIo, &Target, &Lun);

  Cdb[0]                        = EFI_SCSI_OP_MODE_SEN10;
  //
  // DBDField is in Cdb[1] bit3 of (bit7..0)
  //
  Cdb[1]                        = (UINT8) ((Lun & EFI_SCSI_LOGICAL_UNIT_NUMBER_MASK) + ((DBDField << 3) & 0x08));
  //
  // PageControl is in Cdb[2] bit7..6, PageCode is in Cdb[2] bit5..0
  //
  Cdb[2]                        = (UINT8) ((PageControl & 0xc0) | (PageCode & 0x3f));
  Cdb[7]                        = (UINT8) (*DataLength >> 8);
  Cdb[8]                        = (UINT8) (*DataLength);

  CommandPacket.CdbLength       = EFI_SCSI_OP_LENGTH_TEN;
  CommandPacket.DataDirection   = EFI_SCSI_DATA_IN;
  CommandPacket.SenseDataLength = *SenseDataLength;

  Status                        = ScsiIo->ExecuteScsiCommand (ScsiIo, &CommandPacket, NULL);

  *HostAdapterStatus            = CommandPacket.HostAdapterStatus;
  *TargetStatus                 = CommandPacket.TargetStatus;
  *SenseDataLength              = CommandPacket.SenseDataLength;
  *DataLength                   = CommandPacket.InTransferLength;

  return Status;
}


/**
  Function to submit SCSI request sense command.

  Submit SCSI request sense command with the SCSI requested packet specified by this
  SCSI command, TimeOut and SenseData, and then return the status of scsi unit execution.

  If SenseDataLength is NULL, then ASSERT().
  
  If HostAdapterStatus is NULL, then ASSERT().
  
  If TargetStatus is NULL, then ASSERT().

  @param[in]       ScsiIo               A pointer to SCSI IO protocol.
  @param[in]       Timeout              The length of timeout period.
  @param[in]       SenseData            A pointer to output sense data.
  @param[in, out]   SenseDataLength      The length of output sense data.
  @param[out]      HostAdapterStatus    The status of Host Adapter.
  @param[out]      TargetStatus         The status of the target.

  @retval EFI_SUCCESS                   Command is executed successfully.
  @retval EFI_NOT_READY                 The SCSI Request Packet could not be sent because there are
                                        too many SCSI Command Packets already queued.
  @retval EFI_DEVICE_ERROR              A device error occurred while attempting to send SCSI Request Packet.
  @retval EFI_UNSUPPORTED               The command described by the SCSI Request Packet is not supported by
                                        the SCSI initiator(i.e., SCSI  Host Controller)
  @retval EFI_TIMEOUT                   A timeout occurred while waiting for the SCSI Request Packet to execute.
  @retval EFI_INVALID_PARAMETER         ScsiIo is NULL.

**/
EFI_STATUS
EFIAPI
ScsiRequestSenseCommand (
  IN  EFI_SCSI_IO_PROTOCOL  *ScsiIo,
  IN  UINT64                Timeout,
  IN  VOID                  *SenseData,
  IN OUT UINT8              *SenseDataLength,
  OUT UINT8                 *HostAdapterStatus,
  OUT UINT8                 *TargetStatus
  )
{
  EFI_SCSI_IO_SCSI_REQUEST_PACKET CommandPacket;
  UINT64                          Lun;
  UINT8                           *Target;
  UINT8                           TargetArray[EFI_SCSI_TARGET_MAX_BYTES];
  EFI_STATUS                      Status;
  UINT8                           Cdb[EFI_SCSI_OP_LENGTH_SIX];

  ASSERT (SenseDataLength != NULL);
  ASSERT (HostAdapterStatus != NULL);
  ASSERT (TargetStatus != NULL);
  
  if (ScsiIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&CommandPacket, sizeof (EFI_SCSI_IO_SCSI_REQUEST_PACKET));
  ZeroMem (Cdb, EFI_SCSI_OP_LENGTH_SIX);

  CommandPacket.Timeout         = Timeout;
  CommandPacket.InDataBuffer    = SenseData;
  CommandPacket.SenseData       = NULL;
  CommandPacket.InTransferLength= *SenseDataLength;
  CommandPacket.Cdb             = Cdb;
  //
  // Fill Cdb for Request Sense Command
  //
  Target = &TargetArray[0];
  ScsiIo->GetDeviceLocation (ScsiIo, &Target, &Lun);

  Cdb[0]                        = EFI_SCSI_OP_REQUEST_SENSE;
  Cdb[1]                        = (UINT8) (Lun & EFI_SCSI_LOGICAL_UNIT_NUMBER_MASK);
  Cdb[4]                        = (UINT8) (*SenseDataLength);

  CommandPacket.CdbLength       = (UINT8) EFI_SCSI_OP_LENGTH_SIX;
  CommandPacket.DataDirection   = EFI_SCSI_DATA_IN;
  CommandPacket.SenseDataLength = 0;

  Status                        = ScsiIo->ExecuteScsiCommand (ScsiIo, &CommandPacket, NULL);

  *HostAdapterStatus            = CommandPacket.HostAdapterStatus;
  *TargetStatus                 = CommandPacket.TargetStatus;
  *SenseDataLength              = (UINT8) CommandPacket.InTransferLength;

  return Status;
}


/**
  Function to submit read capacity command.

  Submit SCSI read capacity command with the SCSI request packet specified by this SCSI 
  command and the input parameters, and then return the status of Scsi unit execution.

  If SenseDataLength is NULL, then ASSERT().

  If HostAdapterStatus is NULL, then ASSERT().

  If TargetStatus is NULL, then ASSERT().

  If DataLength is NULL, then ASSERT().

  @param[in]     ScsiIo               A pointer to SCSI IO protocol.
  @param[in]     Timeout              The length of timeout period.
  @param[in]     SenseData            A pointer to output sense data.
  @param[in, out] SenseDataLength      The length of output sense data.
  @param[out]    HostAdapterStatus    The status of Host Adapter.
  @param[out]    TargetStatus         The status of the target.
  @param[out]    DataBuffer           A pointer to a data buffer.
  @param[in, out] DataLength           The length of data buffer.
  @param[in]     PMI                  Partial medium indicator.

  @retval  EFI_SUCCESS                Command is executed successfully.
  @retval  EFI_WARN_BUFFER_TOO_SMALL  The SCSI Request Packet was executed, but the entire DataBuffer could
                                      not be transferred. The actual number of bytes transferred is returned in DataLength.
  @retval  EFI_NOT_READY              The SCSI Request Packet could not be sent because there are too many 
                                      SCSI Command Packets already queued.
  @retval  EFI_DEVICE_ERROR           A device error occurred while attempting to send SCSI Request Packet.
  @retval  EFI_UNSUPPORTED            The command described by the SCSI Request Packet is not supported by 
                                      the SCSI initiator(i.e., SCSI  Host Controller)
  @retval  EFI_TIMEOUT                A timeout occurred while waiting for the SCSI Request Packet to execute.
  @retval  EFI_INVALID_PARAMETER      ScsiIo is NULL.

**/
EFI_STATUS
EFIAPI
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
  )
{
  EFI_SCSI_IO_SCSI_REQUEST_PACKET CommandPacket;
  UINT64                          Lun;
  UINT8                           *Target;
  UINT8                           TargetArray[EFI_SCSI_TARGET_MAX_BYTES];
  EFI_STATUS                      Status;
  UINT8                           Cdb[EFI_SCSI_OP_LENGTH_TEN];

  ASSERT (SenseDataLength != NULL);
  ASSERT (HostAdapterStatus != NULL);
  ASSERT (TargetStatus != NULL);
  ASSERT (DataLength != NULL);
  
  if (ScsiIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&CommandPacket, sizeof (EFI_SCSI_IO_SCSI_REQUEST_PACKET));
  ZeroMem (Cdb, EFI_SCSI_OP_LENGTH_TEN);

  CommandPacket.Timeout         = Timeout;
  CommandPacket.InDataBuffer    = DataBuffer;
  CommandPacket.SenseData       = SenseData;
  CommandPacket.InTransferLength= *DataLength;
  CommandPacket.Cdb             = Cdb;
  //
  // Fill Cdb for Read Capacity Command
  //
  Target = &TargetArray[0];
  ScsiIo->GetDeviceLocation (ScsiIo, &Target, &Lun);

  Cdb[0]  = EFI_SCSI_OP_READ_CAPACITY;
  Cdb[1]  = (UINT8) (Lun & EFI_SCSI_LOGICAL_UNIT_NUMBER_MASK);
  if (!PMI) {
    //
    // Partial medium indicator,if PMI is FALSE, the Cdb.2 ~ Cdb.5 MUST BE ZERO.
    //
    ZeroMem ((Cdb + 2), 4);
  } else {
    Cdb[8] |= 0x01;
  }

  CommandPacket.CdbLength       = EFI_SCSI_OP_LENGTH_TEN;
  CommandPacket.DataDirection   = EFI_SCSI_DATA_IN;
  CommandPacket.SenseDataLength = *SenseDataLength;

  Status                        = ScsiIo->ExecuteScsiCommand (ScsiIo, &CommandPacket, NULL);

  *HostAdapterStatus            = CommandPacket.HostAdapterStatus;
  *TargetStatus                 = CommandPacket.TargetStatus;
  *SenseDataLength              = CommandPacket.SenseDataLength;
  *DataLength                   = CommandPacket.InTransferLength;

  return Status;
}


/**
  Function to submit read 10 command.

  Submit SCSI read (10) command with the SCSI request packet specified by this SCSI command 
  and the input parameters, and then return the status of Scsi unit execution.

  If SenseDataLength is NULL, then ASSERT().

  If HostAdapterStatus is NULL, then ASSERT().

  If TargetStatus is NULL, then ASSERT().

  If DataLength is NULL, then ASSERT().

  @param[in]     ScsiIo               A pointer to SCSI IO protocol.
  @param[in]     Timeout              The length of timeout period.
  @param[in]     SenseData            A pointer to output sense data.
  @param[in, out] SenseDataLength      The length of output sense data.
  @param[out]    HostAdapterStatus    The status of Host Adapter.
  @param[out]    TargetStatus         The status of the target.
  @param[out]    DataBuffer           Read 10 command data.
  @param[in, out] DataLength           The length of data buffer.
  @param[in]     StartLba             The start address of LBA.
  @param[in]     SectorSize           The sector size.

  @retval  EFI_SUCCESS                Command is executed successfully.
  @retval  EFI_WARN_BUFFER_TOO_SMALL  The SCSI Request Packet was executed, but the entire DataBuffer could
                                      not be transferred. The actual number of bytes transferred is returned in DataLength.
  @retval  EFI_NOT_READY              The SCSI Request Packet could not be sent because there are too many 
                                      SCSI Command Packets already queued.
  @retval  EFI_DEVICE_ERROR           A device error occurred while attempting to send SCSI Request Packet.
  @retval  EFI_UNSUPPORTED            The command described by the SCSI Request Packet is not supported by 
                                      the SCSI initiator(i.e., SCSI  Host Controller)
  @retval  EFI_TIMEOUT                A timeout occurred while waiting for the SCSI Request Packet to execute.
  @retval  EFI_INVALID_PARAMETER      ScsiIo is NULL.

**/
EFI_STATUS
EFIAPI
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
  )
{
  EFI_SCSI_IO_SCSI_REQUEST_PACKET CommandPacket;
  UINT64                          Lun;
  UINT8                           *Target;
  UINT8                           TargetArray[EFI_SCSI_TARGET_MAX_BYTES];
  EFI_STATUS                      Status;
  UINT8                           Cdb[EFI_SCSI_OP_LENGTH_TEN];

  ASSERT (SenseDataLength != NULL);
  ASSERT (HostAdapterStatus != NULL);
  ASSERT (TargetStatus != NULL);
  ASSERT (DataLength != NULL);
  
  if (ScsiIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&CommandPacket, sizeof (EFI_SCSI_IO_SCSI_REQUEST_PACKET));
  ZeroMem (Cdb, EFI_SCSI_OP_LENGTH_TEN);

  CommandPacket.Timeout         = Timeout;
  CommandPacket.InDataBuffer    = DataBuffer;
  CommandPacket.SenseData       = SenseData;
  CommandPacket.InTransferLength= *DataLength;
  CommandPacket.Cdb             = Cdb;
  //
  // Fill Cdb for Read (10) Command
  //
  Target = &TargetArray[0];
  ScsiIo->GetDeviceLocation (ScsiIo, &Target, &Lun);

  Cdb[0]                        = EFI_SCSI_OP_READ10;
  Cdb[1]                        = (UINT8) (Lun & EFI_SCSI_LOGICAL_UNIT_NUMBER_MASK);
  Cdb[2]                        = (UINT8) (StartLba >> 24);
  Cdb[3]                        = (UINT8) (StartLba >> 16);
  Cdb[4]                        = (UINT8) (StartLba >> 8);
  Cdb[5]                        = (UINT8) (StartLba & 0xff);
  Cdb[7]                        = (UINT8) (SectorSize >> 8);
  Cdb[8]                        = (UINT8) (SectorSize & 0xff);

  CommandPacket.CdbLength       = EFI_SCSI_OP_LENGTH_TEN;
  CommandPacket.DataDirection   = EFI_SCSI_DATA_IN;
  CommandPacket.SenseDataLength = *SenseDataLength;

  Status                        = ScsiIo->ExecuteScsiCommand (ScsiIo, &CommandPacket, NULL);

  *HostAdapterStatus            = CommandPacket.HostAdapterStatus;
  *TargetStatus                 = CommandPacket.TargetStatus;
  *SenseDataLength              = CommandPacket.SenseDataLength;
  *DataLength                   = CommandPacket.InTransferLength;

  return Status;
}


/**
  Function to submit SCSI write 10 command.

  Submit SCSI write (10) command with the SCSI request packet specified by this SCSI command and the
  input parameters, and then return the status of Scsi unit execution.

  If SenseDataLength is NULL, then ASSERT().

  If HostAdapterStatus is NULL, then ASSERT().

  If TargetStatus is NULL, then ASSERT().

  If DataLength is NULL, then ASSERT().

  @param[in]     ScsiIo               SCSI IO Protocol to use
  @param[in]     Timeout              The length of timeout period.
  @param[in]     SenseData            A pointer to output sense data.
  @param[in, out] SenseDataLength      The length of output sense data.
  @param[out]    HostAdapterStatus    The status of Host Adapter.
  @param[out]    TargetStatus         The status of the target.
  @param[out]    DataBuffer           A pointer to a data buffer.
  @param[in, out] DataLength           The length of data buffer.
  @param[in]     StartLba             The start address of LBA.
  @param[in]     SectorSize           The sector size.

  @retval  EFI_SUCCESS                Command is executed successfully.
  @retval  EFI_WARN_BUFFER_TOO_SMALL  The SCSI Request Packet was executed, but the entire DataBuffer could
                                      not be transferred. The actual number of bytes transferred is returned in DataLength.
  @retval  EFI_NOT_READY              The SCSI Request Packet could not be sent because there are too many 
                                      SCSI Command Packets already queued.
  @retval  EFI_DEVICE_ERROR           A device error occurred while attempting to send SCSI Request Packet.
  @retval  EFI_UNSUPPORTED            The command described by the SCSI Request Packet is not supported by 
                                      the SCSI initiator(i.e., SCSI  Host Controller)
  @retval  EFI_TIMEOUT                A timeout occurred while waiting for the SCSI Request Packet to execute.
  @retval  EFI_INVALID_PARAMETER      ScsiIo is NULL.

**/
EFI_STATUS
EFIAPI
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
  )
{
  EFI_SCSI_IO_SCSI_REQUEST_PACKET CommandPacket;
  UINT64                          Lun;
  UINT8                           *Target;
  UINT8                           TargetArray[EFI_SCSI_TARGET_MAX_BYTES];
  EFI_STATUS                      Status;
  UINT8                           Cdb[EFI_SCSI_OP_LENGTH_TEN];

  ASSERT (SenseDataLength != NULL);
  ASSERT (HostAdapterStatus != NULL);
  ASSERT (TargetStatus != NULL);
  ASSERT (DataLength != NULL);
  
  if (ScsiIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&CommandPacket, sizeof (EFI_SCSI_IO_SCSI_REQUEST_PACKET));
  ZeroMem (Cdb, EFI_SCSI_OP_LENGTH_TEN);

  CommandPacket.Timeout         = Timeout;
  CommandPacket.OutDataBuffer    = DataBuffer;
  CommandPacket.SenseData       = SenseData;
  CommandPacket.OutTransferLength= *DataLength;
  CommandPacket.Cdb             = Cdb;
  //
  // Fill Cdb for Write (10) Command
  //
  Target = &TargetArray[0];
  ScsiIo->GetDeviceLocation (ScsiIo, &Target, &Lun);

  Cdb[0]                        = EFI_SCSI_OP_WRITE10;
  Cdb[1]                        = (UINT8) (Lun & EFI_SCSI_LOGICAL_UNIT_NUMBER_MASK);
  Cdb[2]                        = (UINT8) (StartLba >> 24);
  Cdb[3]                        = (UINT8) (StartLba >> 16);
  Cdb[4]                        = (UINT8) (StartLba >> 8);
  Cdb[5]                        = (UINT8) StartLba;
  Cdb[7]                        = (UINT8) (SectorSize >> 8);
  Cdb[8]                        = (UINT8) SectorSize;

  CommandPacket.CdbLength       = EFI_SCSI_OP_LENGTH_TEN;
  CommandPacket.DataDirection   = EFI_SCSI_DATA_OUT;
  CommandPacket.SenseDataLength = *SenseDataLength;

  Status                        = ScsiIo->ExecuteScsiCommand (ScsiIo, &CommandPacket, NULL);

  *HostAdapterStatus            = CommandPacket.HostAdapterStatus;
  *TargetStatus                 = CommandPacket.TargetStatus;
  *SenseDataLength              = CommandPacket.SenseDataLength;
  *DataLength                   = CommandPacket.OutTransferLength;

  return Status;
}

