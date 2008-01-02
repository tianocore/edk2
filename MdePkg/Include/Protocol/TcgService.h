/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  TcgService.h

Abstract:

  TCG Service Protocol as defined in TCG_EFI_Protocol_1_20_Final

  See http://trustedcomputinggroup.org for the latest specification

--*/

#ifndef _TCG_SERVICE_PROTOCOL_H_
#define _TCG_SERVICE_PROTOCOL_H_

#include <Uefi/UefiTcgPlatform.h>

#define EFI_TCG_PROTOCOL_GUID  \
  {0xf541796d, 0xa62e, 0x4954, { 0xa7, 0x75, 0x95, 0x84, 0xf6, 0x1b, 0x9c, 0xdd } } 

typedef struct _EFI_TCG_PROTOCOL EFI_TCG_PROTOCOL;

//
// Set structure alignment to 1-byte
//
#pragma pack (push, 1)

typedef struct {
  UINT8  Major;
  UINT8  Minor;
  UINT8  RevMajor;
  UINT8  RevMinor;
} TCG_VERSION;

typedef struct _TCG_EFI_BOOT_SERVICE_CAPABILITY {
  UINT8          Size;                // Size of this structure
  TCG_VERSION    StructureVersion;    
  TCG_VERSION    ProtocolSpecVersion;
  UINT8          HashAlgorithmBitmap; // Hash algorithms  
                                      // this protocol is capable of : 01=SHA-1
  BOOLEAN        TPMPresentFlag;      // 00h = TPM not present
  BOOLEAN        TPMDeactivatedFlag;  // 01h = TPM currently deactivated
} TCG_EFI_BOOT_SERVICE_CAPABILITY;

typedef UINT32   TCG_ALGORITHM_ID;

//
// Restore original structure alignment
//
#pragma pack (pop)

typedef
EFI_STATUS
(EFIAPI *EFI_TCG_STATUS_CHECK) (
  IN      EFI_TCG_PROTOCOL          *This,
  OUT     TCG_EFI_BOOT_SERVICE_CAPABILITY
                                    *ProtocolCapability,
  OUT     UINT32                    *TCGFeatureFlags,
  OUT     EFI_PHYSICAL_ADDRESS      *EventLogLocation,
  OUT     EFI_PHYSICAL_ADDRESS      *EventLogLastEntry
  );

typedef
EFI_STATUS
(EFIAPI *EFI_TCG_HASH_ALL) (
  IN      EFI_TCG_PROTOCOL          *This,
  IN      UINT8                     *HashData,
  IN      UINT64                    HashDataLen,
  IN      TCG_ALGORITHM_ID          AlgorithmId,
  IN OUT  UINT64                    *HashedDataLen,
  IN OUT  UINT8                     **HashedDataResult
  );

typedef
EFI_STATUS
(EFIAPI *EFI_TCG_LOG_EVENT) (
  IN      EFI_TCG_PROTOCOL          *This,
  IN      TCG_PCR_EVENT             *TCGLogData,
  IN OUT  UINT32                    *EventNumber,
  IN      UINT32                    Flags
  );

typedef
EFI_STATUS
(EFIAPI *EFI_TCG_PASS_THROUGH_TO_TPM) (
  IN      EFI_TCG_PROTOCOL          *This,
  IN      UINT32                    TpmInputParamterBlockSize,
  IN      UINT8                     *TpmInputParamterBlock,
  IN      UINT32                    TpmOutputParameterBlockSize,
  IN      UINT8                     *TpmOutputParameterBlock
  );

typedef
EFI_STATUS
(EFIAPI *EFI_TCG_HASH_LOG_EXTEND_EVENT) (
  IN      EFI_TCG_PROTOCOL          *This,
  IN      UINT8                     *HashData,
  IN      UINT64                    HashDataLen,
  IN      TCG_ALGORITHM_ID          AlgorithmId,
  IN OUT  TCG_PCR_EVENT             *TCGLogData,
  IN OUT  UINT32                    *EventNumber,
     OUT  EFI_PHYSICAL_ADDRESS      *EventLogLastEntry
  );

typedef struct _EFI_TCG_PROTOCOL {
  EFI_TCG_STATUS_CHECK              StatusCheck;
  EFI_TCG_HASH_ALL                  HashAll;
  EFI_TCG_LOG_EVENT                 LogEvent;
  EFI_TCG_PASS_THROUGH_TO_TPM       PassThroughToTpm;
  EFI_TCG_HASH_LOG_EXTEND_EVENT     HashLogExtendEvent;
} EFI_TCG_PROTOCOL;

extern EFI_GUID gEfiTcgProtocolGuid;

#endif
