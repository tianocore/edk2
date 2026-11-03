/** @file

  Copyright (c) 2026, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard
**/

#pragma once

#include <IndustryStandard/McNetworkHostInterface.h>
#include <IndustryStandard/MctpHostInterface.h>

typedef struct MchiProtocolSpecificDataOps  MCHI_PROTOCOL_SPECIFIC_DATA_OPS;
typedef struct MchiNetworkDeviceDescOps     MCHI_NETWORK_DEVICE_DESC_OPS;
typedef struct MchiDataOps                  MCHI_DATA_OPS;

typedef enum MchiDataOpsIdx {
  /// MCHI data operation for MCTP (0x00-0x3F)
  MchiMctpDataOpsIdx,
  /// MCHI data operation for Newtwork (0x40)
  MchiNetworkDataOpsIdx,
  /// MCHI data operation for OEM (0xF0)
  MchiOemDataOpsIdx,
} MCHI_DATA_OPS_IDX;

typedef struct MchiProtocolDataInfo {
  /// List Entry.
  LIST_ENTRY                         Entry;

  /// Unique Token for Protocol information.
  CM_OBJECT_TOKEN                    Token;

  /// Relevant CM object.
  VOID                               *CmObject;

  /// Protocol Type.
  MC_HOST_INTERFACE_PROTOCOL_TYPE    ProtocolType;

  /// Protocol Data Size.
  UINTN                              DataSize;
} MCHI_PROTOCOL_DATA_INFO;

/** Get MCHI_PROTOCOL_DATA_INFO associated with Token.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       Token                Protocol DataToken.
  @param [in]       ProtocolType         Protocol Type.
  @param [out]      ProtocolInfo         Protocol Info.

  @retval EFI_SUCCESS
  @retval Others                         Failed to get MCHI_PROTOCOL_DATA_INFO
**/
typedef
EFI_STATUS
(EFIAPI *GET_MCHI_PROTOCOL_DATA_INFO)(
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN            CM_OBJECT_TOKEN                                        Token,
  IN            MC_HOST_INTERFACE_PROTOCOL_TYPE                        ProtocolType,
  OUT           MCHI_PROTOCOL_DATA_INFO                                **ProtocolInfo
  );

/** Get size of MCHI protocol specific data.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       ProtocolInfo         ProtocolInfo.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
typedef
EFI_STATUS
(EFIAPI *GET_SIZE_OF_MCHI_PROTOCOL_SPECIFIC_DATA)(
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      MCHI_PROTOCOL_DATA_INFO                                      *ProtocolInfo
  );

/** Add MCHI protocol specific data into buffer.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       ProtocolInfo         ProtocolInfo.
  @param [in]       StrTable             String Table.
  @param [out]      Buffer               Buffer.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
typedef
EFI_STATUS
(EFIAPI *ADD_MCHI_PROTOCOL_SPECIFIC_DATA)(
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      CONST MCHI_PROTOCOL_DATA_INFO                                *ProtocolInfo,
  IN      STRING_TABLE                                         *CONST  StrTable,
  OUT     VOID                                                         *Buffer
  );

/** Get MCHI Network Device descriptor CmObject associated with Token.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       Token                Device Descriptor Token.
  @param [in]       DeviceType           Device Type.
  @param [out]      CmObject             CM object associated device descriptor.
  @param [out]      Count                Number of CM object.

  @retval EFI_SUCCESS
  @retval Others                         Failed to get CM object
**/
typedef
EFI_STATUS
(EFIAPI *GET_MCHI_NETWORK_DEVICE_DESC_CM_OBJ)(
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      CM_OBJECT_TOKEN                                              Token,
  IN      MCHI_NETWORK_DEVICE_TYPE                                     DeviceType,
  OUT     VOID                                                         **CmObject,
  OUT     UINT32                                                       *Count
  );

/** Get size of Device descriptor data.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CmObject             CM object.
  @param [out]      Size                 Size of Device descriptor data.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
typedef
EFI_STATUS
(EFIAPI *GET_SIZE_OF_MCHI_NETWORK_DEVICE_DESC_DATA)(
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      CONST VOID                                                   *CmObject,
  OUT     UINTN                                                        *Size
  );

/** Add MCHI Network Device specific data into buffer.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CmObject             CM object.
  @param [in]       StrTable             String Table.
  @param [out]      Buffer               Buffer.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
typedef
EFI_STATUS
(EFIAPI *ADD_MCHI_NETWORK_DEVICE_DESC_DATA)(
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      CONST VOID                                                   *CmObject,
  IN      STRING_TABLE                                         *CONST  StrTable,
  OUT     VOID                                                         *Buffer
  );

/** Get MCHI Data CM objects.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager.
                                         Protocol Interface.
  @param [in]       Token                MCHI Data Token.
  @param [in]       InterfaceType        Interface Type.
  @param [out]      CmObject             MCHI Data CM object.
  @param [out]      Count                Number of MCHI Data CM object.

  @retval EFI_SUCCESS
  @retval Others                         Failed to initialise
**/
typedef
EFI_STATUS
(EFIAPI *GET_MCHI_DATA_CM_OBJ)(
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CM_OBJECT_TOKEN                                     Token,
  IN      MC_HOST_INTERFACE_TYPE                              InterfaceType,
  OUT     VOID                                                **CmObject,
  OUT     UINT32                                              *Count
  );

/** Get size of MCHI Data.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager.
                                         Protocol Interface.
  @param [in]       InterfaceType        Interface Type.
  @param [in]       CmObject             MCHI Data CM object.
  @param [out]      Size                 Size of MCHI Data.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
typedef
EFI_STATUS
(EFIAPI *GET_SIZE_OF_MCHI_DATA)(
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      MC_HOST_INTERFACE_TYPE                                       InterfaceType,
  IN      CONST VOID                                                   *CmObject,
  OUT     UINTN                                                        *Size
  );

/** Add MCHI Data.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       InterfaceType        Interface Type.
  @param [in]       CmObject             CM object of Processor Specific Data.
  @param [in]       StrTable             String Table.
  @param [out]      Buffer               Buffer.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
typedef
EFI_STATUS
(EFIAPI *ADD_MCHI_DATA)(
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      MC_HOST_INTERFACE_TYPE                                       InterfaceType,
  IN      CONST VOID                                                   *CmObject,
  IN      STRING_TABLE                                         *CONST  StrTable,
  OUT     VOID                                                         *Buffer
  );

/** A structure that describes operation for MCHI protocol specific
    data operation.
**/
typedef struct MchiProtocolSpecificDataOps {
  /// MCHI protocol Type.
  MC_HOST_INTERFACE_PROTOCOL_TYPE            ProtocolType;

  /// Get MCHI Protocol info.
  GET_MCHI_PROTOCOL_DATA_INFO                GetMchiProtocolDataInfo;

  /// Get size of MCHI Protocol specific data
  GET_SIZE_OF_MCHI_PROTOCOL_SPECIFIC_DATA    GetSizeofMchiProtocolSpecificData;

  /// Add Error source to HEST
  ADD_MCHI_PROTOCOL_SPECIFIC_DATA            AddMchiProtocolSpecificData;
} MCHI_PROTOCOL_SPECIFIC_DATA_OPS;

/** A structure that describes operation for MCHI Network device descriptor
    operation.
**/
typedef struct MchiNetworkDeviceDescOps {
  /// MCHI Network Device Type.
  MCHI_NETWORK_DEVICE_TYPE                     DeviceType;

  /// Get MCHI Network
  GET_MCHI_NETWORK_DEVICE_DESC_CM_OBJ          GetMchiNetworkDeviceDescCmObj;

  /// Get size of MCHI Network device descriptor data
  GET_SIZE_OF_MCHI_NETWORK_DEVICE_DESC_DATA    GetSizeofMchiNetworkDeviceDescData;

  /// Add Error source to HEST
  ADD_MCHI_NETWORK_DEVICE_DESC_DATA            AddMchiNetworkDeviceDescData;

  /// Device Descriptor CmObject
  VOID                                         *CmObject;

  /// Size of MCHI Network device descriptor.
  UINTN                                        DataSize;
} MCHI_NETWORK_DEVICE_DESC_OPS;

/** A structure that describes MCHI data operation..
*/
typedef struct MchiDataOps {
  /// MCHI data ops index.
  MCHI_DATA_OPS_IDX        MchiDataOpsIdx;

  /// Get CM objects for MCHI Data.
  GET_MCHI_DATA_CM_OBJ     GetMchiDataCmObj;

  /// Get size of MCHI Data.
  GET_SIZE_OF_MCHI_DATA    GetSizeofMchiData;

  /// Add Error source to HEST
  ADD_MCHI_DATA            AddMchiData;

  /// MCHI Data CM object.
  VOID                     *CmObject;
} MCHI_DATA_OPS;
