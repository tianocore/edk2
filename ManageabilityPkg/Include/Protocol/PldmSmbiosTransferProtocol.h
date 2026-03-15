/** @file
  Protocol of EDKII PLDM SMBIOS Transfer Protocol.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL_H_
#define EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL_H_

#include <IndustryStandard/Pldm.h>

typedef struct  _EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL;

#define EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL_GUID \
  { \
    0xFA431C3C, 0x816B, 0x4B32, 0xA3, 0xE0, 0xAD, 0x9B, 0x7F, 0x64, 0x27, 0x2E \
  }

#define EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL_VERSION_MAJOR  1
#define EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL_VERSION_MINOR  0
#define EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL_VERSION        ((EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL_VERSION_MAJOR << 8) |\
                                                       EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL_VERSION_MINOR)

/**
  This function sets PLDM SMBIOS transfer source and destination
  PLDM terminus ID.

  @param [in]   This           EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL instance.
  @param [in]   SourceId       PLDM source teminus ID.
                               Set to PLDM_TERMINUS_ID_UNASSIGNED means use
                               platform default PLDM terminus ID.
                               (gManageabilityPkgTokenSpaceGuid.PcdPldmSourceTerminusId)
  @param [in]   DestinationId  PLDM destination teminus ID.
                               Set to PLDM_TERMINUS_ID_UNASSIGNED means use
                               platform default PLDM terminus ID.
                               (gManageabilityPkgTokenSpaceGuid.PcdPldmDestinationEndpointId)

  @retval       EFI_SUCCESS            Get SMBIOS table metadata Successfully.
  @retval       EFI_INVALID_PARAMETER  Invalid value of source or destination
                                       PLDM terminus ID.
**/
typedef
EFI_STATUS
(EFIAPI *PLDM_GET_SMBIOS_TRANSFER_TERMINUS_ID)(
  IN  UINT8  SourceId,
  IN  UINT8  DestinationId
  );

/**
  This function gets SMBIOS table metadata.

  @param [in]   This         EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL instance.
  @param [out]  Buffer       Buffer to receive the SMBIOS table metadata.

  @retval       EFI_SUCCESS            Get SMBIOS table metadata Successfully.
  @retval       EFI_UNSUPPORTED        The function is unsupported by this
                                       driver instance.
  @retval       Other values           Fail to get SMBIOS table metadata.
**/
typedef
EFI_STATUS
(EFIAPI *PLDM_GET_SMBIOS_STRUCTURE_TABLE_METADATA)(
  IN  EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL   *This,
  OUT PLDM_SMBIOS_STRUCTURE_TABLE_METADATA  *Buffer
  );

/**
  This function sets SMBIOS table metadata.

  @param [in]   This         EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL instance.
  @param [in]   Buffer       Pointer to SMBIOS table metadata.

  @retval       EFI_SUCCESS            Set SMBIOS table metadata Successfully.
  @retval       EFI_UNSUPPORTED        The function is unsupported by this
                                       driver instance.
  @retval       Other values           Fail to set SMBIOS table metadata.
**/
typedef
EFI_STATUS
(EFIAPI *PLDM_SET_SMBIOS_STRUCTURE_TABLE_METADATA)(
  IN  EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL   *This,
  IN  PLDM_SMBIOS_STRUCTURE_TABLE_METADATA  *Buffer
  );

/**
  This function gets SMBIOS structure table.

  @param [in]   This        EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL instance.
  @param [out]  Buffer      Pointer to the returned SMBIOS structure table.
                            Caller has to free this memory block when it
                            is no longer needed.
  @param [out]  BufferSize  Size of the returned message payload in buffer.

  @retval       EFI_SUCCESS            Gets SMBIOS structure table successfully.
  @retval       EFI_UNSUPPORTED        The function is unsupported by this
                                       driver instance.
  @retval       Other values           Fail to get SMBIOS structure table.
**/
typedef
EFI_STATUS
(EFIAPI *PLDM_GET_SMBIOS_STRUCTURE_TABLE)(
  IN   EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL   *This,
  OUT  UINT8                                 **Buffer,
  OUT  UINT32                                *BufferSize
  );

/**
  This function sets SMBIOS structure table.

  @param [in]   This        EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL instance.

  @retval      EFI_SUCCESS            Successful
  @retval      EFI_UNSUPPORTED        The function is unsupported by this
                                      driver instance.
  @retval      Other values           Fail to set SMBIOS structure table.
**/
typedef
EFI_STATUS
(EFIAPI *PLDM_SET_SMBIOS_STRUCTURE_TABLE)(
  IN   EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL  *This
  );

/**
  This function gets particular type of SMBIOS structure.

  @param [in]   This                 EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL instance.
  @param [in]   TypeId               The type of SMBIOS structure.
  @param [in]   StructureInstanceId  The instance ID of particular type of SMBIOS structure.
  @param [out]  Buffer               Pointer to the returned SMBIOS structure.
                                     Caller has to free this memory block when it
                                     is no longer needed.
  @param [out]  BufferSize           Size of the returned message payload in buffer.

  @retval      EFI_SUCCESS           Gets particular type of SMBIOS structure successfully.
  @retval      EFI_UNSUPPORTED       The function is unsupported by this
                                     driver instance.
  @retval      Other values          Fail to set SMBIOS structure table.
**/
typedef
EFI_STATUS
(EFIAPI *PLDM_GET_SMBIOS_STRUCTURE_BY_TYPE)(
  IN   EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL  *This,
  IN   UINT8                                TypeId,
  IN   UINT16                               StructureInstanceId,
  OUT  UINT8                                **Buffer,
  OUT  UINT32                               *BufferSize
  );

/**
  This function gets particular handle of SMBIOS structure.

  @param [in]   This                 EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL instance.
  @param [in]   Handle               The handle of SMBIOS structure.
  @param [out]  Buffer               Pointer to the returned SMBIOS structure.
                                     Caller has to free this memory block when it
                                     is no longer needed.
  @param [out]  BufferSize           Size of the returned message payload in buffer.

  @retval      EFI_SUCCESS           Gets particular handle of SMBIOS structure successfully.
  @retval      EFI_UNSUPPORTED       The function is unsupported by this
                                     driver instance.
  @retval      Other values          Fail to set SMBIOS structure table.
**/
typedef
EFI_STATUS
(EFIAPI *PLDM_GET_SMBIOS_STRUCTURE_BY_HANDLE)(
  IN   EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL  *This,
  IN   UINT16                               Handle,
  OUT  UINT8                                **Buffer,
  OUT  UINT32                               *BufferSize
  );

//
// EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL
//
typedef struct {
  PLDM_GET_SMBIOS_TRANSFER_TERMINUS_ID        SetPldmSmbiosTransferTerminusId;
  PLDM_GET_SMBIOS_STRUCTURE_TABLE_METADATA    GetSmbiosStructureTableMetaData;
  PLDM_SET_SMBIOS_STRUCTURE_TABLE_METADATA    SetSmbiosStructureTableMetaData;
  PLDM_GET_SMBIOS_STRUCTURE_TABLE             GetSmbiosStructureTable;
  PLDM_SET_SMBIOS_STRUCTURE_TABLE             SetSmbiosStructureTable;
  PLDM_GET_SMBIOS_STRUCTURE_BY_TYPE           GetSmbiosStructureByType;
  PLDM_GET_SMBIOS_STRUCTURE_BY_HANDLE         GetSmbiosStructureByHandle;
} EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL_V1_0;

///
/// Definitions of EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL.
/// This is a union that can accommodate the new functionalities defined
/// in PLDM SMBIOS Transfer specification in the future.
/// The new added function must has its own EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL
/// structure with the incremental version number.
///   e.g., EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL_V1_1.
///
/// The new function must be added base on the last version of
/// EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL to keep the backward compatibility.
///
typedef union {
  EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL_V1_0    *Version1_0;
} EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL_FUNCTION;

struct _EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL {
  UINT16                                          ProtocolVersion;
  EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL_FUNCTION    Functions;
};

extern EFI_GUID  gEdkiiPldmSmbiosTransferProtocolGuid;

#endif // EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL_H_
