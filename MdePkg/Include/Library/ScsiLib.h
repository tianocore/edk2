/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    ScsiLib.h

 Abstract:

   Common Libarary for SCSI

 Revision History

--*/

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

EFI_STATUS
SubmitTestUnitReadyCommand (
  IN  EFI_SCSI_IO_PROTOCOL  *ScsiIo,
  IN  UINT64                Timeout,
  OUT VOID                  *SenseData,
  OUT UINT8                 *SenseDataLength,
  OUT UINT8                 *HostAdapterStatus,
  OUT UINT8                 *TargetStatus
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiIo            - TODO: add argument description
  Timeout           - TODO: add argument description
  SenseData         - TODO: add argument description
  SenseDataLength   - TODO: add argument description
  HostAdapterStatus - TODO: add argument description
  TargetStatus      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
SubmitInquiryCommand (
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
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiIo                  - TODO: add argument description
  Timeout                 - TODO: add argument description
  SenseData               - TODO: add argument description
  SenseDataLength         - TODO: add argument description
  HostAdapterStatus       - TODO: add argument description
  TargetStatus            - TODO: add argument description
  InquiryDataBuffer       - TODO: add argument description
  InquiryDataLength       - TODO: add argument description
  EnableVitalProductData  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
SubmitModeSense10Command (
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
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiIo            - TODO: add argument description
  Timeout           - TODO: add argument description
  SenseData         - TODO: add argument description
  SenseDataLength   - TODO: add argument description
  HostAdapterStatus - TODO: add argument description
  TargetStatus      - TODO: add argument description
  DataBuffer        - TODO: add argument description
  DataLength        - TODO: add argument description
  DBDField          - TODO: add argument description
  PageControl       - TODO: add argument description
  PageCode          - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
SubmitRequestSenseCommand (
  IN  EFI_SCSI_IO_PROTOCOL  *ScsiIo,
  IN  UINT64                Timeout,
  IN  VOID                  *SenseData,
  IN OUT UINT8              *SenseDataLength,
  OUT UINT8                 *HostAdapterStatus,
  OUT UINT8                 *TargetStatus
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiIo            - TODO: add argument description
  Timeout           - TODO: add argument description
  SenseData         - TODO: add argument description
  SenseDataLength   - TODO: add argument description
  HostAdapterStatus - TODO: add argument description
  TargetStatus      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

//
// Commands for direct access command
//
EFI_STATUS
SubmitReadCapacityCommand (
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
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiIo            - TODO: add argument description
  Timeout           - TODO: add argument description
  SenseData         - TODO: add argument description
  SenseDataLength   - TODO: add argument description
  HostAdapterStatus - TODO: add argument description
  TargetStatus      - TODO: add argument description
  DataBuffer        - TODO: add argument description
  DataLength        - TODO: add argument description
  PMI               - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
SubmitRead10Command (
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
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiIo            - TODO: add argument description
  Timeout           - TODO: add argument description
  SenseData         - TODO: add argument description
  SenseDataLength   - TODO: add argument description
  HostAdapterStatus - TODO: add argument description
  TargetStatus      - TODO: add argument description
  DataBuffer        - TODO: add argument description
  DataLength        - TODO: add argument description
  StartLba          - TODO: add argument description
  SectorSize        - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
SubmitWrite10Command (
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
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiIo            - TODO: add argument description
  Timeout           - TODO: add argument description
  SenseData         - TODO: add argument description
  SenseDataLength   - TODO: add argument description
  HostAdapterStatus - TODO: add argument description
  TargetStatus      - TODO: add argument description
  DataBuffer        - TODO: add argument description
  DataLength        - TODO: add argument description
  StartLba          - TODO: add argument description
  SectorSize        - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

#endif
