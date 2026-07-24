/** @file
  SMBIOS Type44 Table Generator.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SmbiosStringTableLib.h>

// Module specific include files.
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>
#include <IndustryStandard/SmBios.h>

#include "SmbiosType42Generator.h"

/**
  SMBIOS Type 42 Generator

  Requirements:
    The following Configuration Manager Object(s) are used by this Generator:
     - EArchCommonObjCmRef
     - EArchCommonObjMchiInfo
     - EArchCommonObjMchiMctpDataInfo
     - EArchCommonObjMchiNetworkDataInfo
     - EArchCommonObjMchiProtocolInfo
     - EArchCommonObjMchiProtocolMctpDataInfo
     - EArchCommonObjMchiProtocolRedfishOverIpDataInfo
     - EArchCommonObjMchiNetworkDeviceDescUsbInfo
     - EArchCommonObjMchiNetworkDeviceDescPciInfo
*/

/**
  This macro expands to a function that retrieves the Error source infomation
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjCmRef,
  CM_ARCH_COMMON_OBJ_REF
  );

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjMchiInfo,
  CM_ARCH_COMMON_MCHI_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjMchiMctpDataInfo,
  CM_ARCH_COMMON_MCHI_MCTP_DATA_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjMchiNetworkDataInfo,
  CM_ARCH_COMMON_MCHI_NETWORK_DATA_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjMchiProtocolInfo,
  CM_ARCH_COMMON_MCHI_PROTOCOL_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjMchiProtocolMctpDataInfo,
  CM_ARCH_COMMON_MCHI_PROTOCOL_MCTP_DATA_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjMchiProtocolRedfishOverIpDataInfo,
  CM_ARCH_COMMON_MCHI_PROTOCOL_REDFISH_OVER_IP_DATA_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjMchiNetworkDeviceDescUsbInfo,
  CM_ARCH_COMMON_MCHI_NETWORK_DEVICE_DESC_USB_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjMchiNetworkDeviceDescPciInfo,
  CM_ARCH_COMMON_MCHI_NETWORK_DEVICE_DESC_PCI_INFO
  );

/*
 * Currently, the string table is only used for Network Host interface
 * with Redfish USBv2 device type for Serial Number.
 */
#define SMBIOS_TYPE42_MAX_STRINGS  (1)

/*
 * List of MCHI Protocol for current Host interface.
 */
LIST_ENTRY  mMchiProtocolList = INITIALIZE_LIST_HEAD_VARIABLE (mMchiProtocolList);

/** Find the MCHI Protocol Entry associated with Token.

  @param [in]       Token                 Protocol Data Token.

  @retval   MCHI_PROTOCOL_DATA_INFO       Entry associated with Token.
  @retval   NULL                          No Entry.
**/
STATIC
MCHI_PROTOCOL_DATA_INFO *
EFIAPI
FindMchiProtocolDataInfo (
  IN  CM_OBJECT_TOKEN  Token
  )
{
  LIST_ENTRY               *Entry;
  MCHI_PROTOCOL_DATA_INFO  *ProtocolInfo;

  ASSERT (Token != CM_NULL_TOKEN);

  for (Entry = GetFirstNode (&mMchiProtocolList);
       Entry != &mMchiProtocolList;
       Entry = GetNextNode (&mMchiProtocolList, Entry))
  {
    ProtocolInfo = BASE_CR (Entry, MCHI_PROTOCOL_DATA_INFO, Entry);

    if (ProtocolInfo->Token == Token) {
      return ProtocolInfo;
    }
  }

  return NULL;
}

/** Insert the MCHI Protocol Data Entry.

  @param [in]       Token                 Protocol Data Token.
  @param [in]       ProtocolType          Protocol Type.
  @param [in]       CmObject              CM object associated with Token.
  @param [out]      ProtocolInfo          Protocol Info.

  @retval   MCHI_PROTOCOL_DATA_INFO       Entry associated with Token.
  @retval   NULL                          No Entry.
**/
STATIC
EFI_STATUS
EFIAPI
InsertMchiProtocolDataInfo (
  IN  CM_OBJECT_TOKEN                  Token           OPTIONAL,
  IN  MC_HOST_INTERFACE_PROTOCOL_TYPE  ProtocolType,
  IN  VOID                             *CmObject       OPTIONAL,
  OUT MCHI_PROTOCOL_DATA_INFO          **ProtocolInfo
  )
{
  MCHI_PROTOCOL_DATA_INFO  *Entry;

  ASSERT (ProtocolInfo != NULL);

  Entry = AllocateZeroPool (sizeof (MCHI_PROTOCOL_DATA_INFO));
  if (Entry == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate ProtocolInfo\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  Entry->Token        = Token;
  Entry->ProtocolType = ProtocolType;
  Entry->CmObject     = CmObject;

  InsertTailList (&mMchiProtocolList, &Entry->Entry);

  *ProtocolInfo = Entry;

  return EFI_SUCCESS;
}

/** Cleanup MCHI Protocol List.
**/
STATIC
VOID
EFIAPI
CleanupMchiProtocolList (
  VOID
  )
{
  LIST_ENTRY  *Entry;
  LIST_ENTRY  *Next;

  for (Entry = GetFirstNode (&mMchiProtocolList),
       Next = GetNextNode (&mMchiProtocolList, Entry);
       !IsListEmpty (&mMchiProtocolList);
       Entry = Next, Next = GetNextNode (&mMchiProtocolList, Entry))
  {
    RemoveEntryList (Entry);
    FreePool (Entry);
  }
}

/** Get IPMI MCHI_PROTOCOL_DATA_INFO.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       Token                Protocol DataToken.
  @param [in]       ProtocolType         Protocol Type.
  @param [out]      ProtocolInfo         Protocol Info.

  @retval EFI_SUCCESS
  @retval Others                         Failed to get MCHI_PROTOCOL_DATA_INFO
**/
STATIC
EFI_STATUS
EFIAPI
GetIpmiProtocolDataInfo (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN            CM_OBJECT_TOKEN                               Token,
  IN            MC_HOST_INTERFACE_PROTOCOL_TYPE               ProtocolType,
  OUT           MCHI_PROTOCOL_DATA_INFO                       **ProtocolInfo
  )
{
  EFI_STATUS  Status;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Token == CM_NULL_TOKEN);
  ASSERT (ProtocolType == MCHostInterfaceProtocolTypeIPMI);
  ASSERT (ProtocolInfo != NULL);

  Status = InsertMchiProtocolDataInfo (
             CM_NULL_TOKEN,
             ProtocolType,
             NULL,
             ProtocolInfo
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to create MCHI_PROTOCOL_DATA_INFO. Token=0x%lx\n",
      __func__,
      Token
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

/** Get MCTP MCHI_PROTOCOL_DATA_INFO.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       Token                Protocol DataToken.
  @param [in]       ProtocolType         Protocol Type.
  @param [out]      ProtocolInfo         Protocol Info.

  @retval EFI_SUCCESS
  @retval Others                         Failed to get MCHI_PROTOCOL_DATA_INFO
**/
STATIC
EFI_STATUS
EFIAPI
GetMctpProtocolDataInfo (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN            CM_OBJECT_TOKEN                               Token,
  IN            MC_HOST_INTERFACE_PROTOCOL_TYPE               ProtocolType,
  OUT           MCHI_PROTOCOL_DATA_INFO                       **ProtocolInfo
  )
{
  EFI_STATUS                                   Status;
  MCHI_PROTOCOL_DATA_INFO                      *Entry;
  CM_ARCH_COMMON_MCHI_PROTOCOL_MCTP_DATA_INFO  *MctpInfo;
  UINT32                                       Count;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Token != CM_NULL_TOKEN);
  ASSERT (ProtocolInfo != NULL);

  Entry = FindMchiProtocolDataInfo (Token);
  if (Entry != NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Protocol Token should be unique in one host interface. Token=0x%lx\n",
      __func__,
      Token
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = GetEArchCommonObjMchiProtocolMctpDataInfo (
             CfgMgrProtocol,
             Token,
             &MctpInfo,
             &Count
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get MCTP protocol info. Status=%r Token=0x%lx\n",
      __func__,
      Status,
      Token
      ));
    return Status;
  }

  if (Count != 1) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Protocol Token should be unique. Token=0x%lx\n",
      __func__,
      Token
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = InsertMchiProtocolDataInfo (
             Token,
             ProtocolType,
             MctpInfo,
             ProtocolInfo
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to create MCHI_PROTOCOL_DATA_INFO. Token=0x%lx\n",
      __func__,
      Token
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

/** Get Redfish Over Ip MCHI_PROTOCOL_DATA_INFO.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       Token                Protocol DataToken.
  @param [in]       ProtocolType         Protocol Type.
  @param [out]      ProtocolInfo         Protocol Info.

  @retval EFI_SUCCESS
  @retval Others                         Failed to get MCHI_PROTOCOL_DATA_INFO
**/
STATIC
EFI_STATUS
EFIAPI
GetRedfishOverIpProtocolDataInfo (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN            CM_OBJECT_TOKEN                               Token,
  IN            MC_HOST_INTERFACE_PROTOCOL_TYPE               ProtocolType,
  OUT           MCHI_PROTOCOL_DATA_INFO                       **ProtocolInfo
  )
{
  EFI_STATUS                                              Status;
  MCHI_PROTOCOL_DATA_INFO                                 *Entry;
  CM_ARCH_COMMON_MCHI_PROTOCOL_REDFISH_OVER_IP_DATA_INFO  *RopInfo;
  UINT32                                                  Count;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Token != CM_NULL_TOKEN);
  ASSERT (ProtocolInfo != NULL);

  Entry = FindMchiProtocolDataInfo (Token);
  if (Entry != NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Protocol Token should be unique in one host interface. Token=0x%lx\n",
      __func__,
      Token
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = GetEArchCommonObjMchiProtocolRedfishOverIpDataInfo (
             CfgMgrProtocol,
             Token,
             &RopInfo,
             &Count
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get Redfish over IP protocol info. Status=%r Token=0x%lx\n",
      __func__,
      Status,
      Token
      ));
    return Status;
  }

  if (Count != 1) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Protocol Token should be unique. Token=0x%lx\n",
      __func__,
      Token
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = InsertMchiProtocolDataInfo (
             Token,
             ProtocolType,
             RopInfo,
             ProtocolInfo
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to create MCHI_PROTOCOL_DATA_INFO. Token=0x%lx\n",
      __func__,
      Token
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

/** Get size protocol specific data of IPMI.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       ProtocolInfo         ProtocolInfo.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSizeofIpmiProtocolSpecificData (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      MCHI_PROTOCOL_DATA_INFO                             *ProtocolInfo
  )
{
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (ProtocolInfo != NULL);

  /*
   * SMBIOS 3.9.0 specification mentions the IPMI specification Appendix C1.
   * But, It doesn't say anything for 42 protocol specific data.
   * So, return 0.
   */
  ProtocolInfo->DataSize = 0;

  return EFI_SUCCESS;
}

/** Get size protocol specific data of MCTP.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       ProtocolInfo         ProtocolInfo.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSizeofMctpProtocolSpecificData (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      MCHI_PROTOCOL_DATA_INFO                             *ProtocolInfo
  )
{
  CM_ARCH_COMMON_MCHI_PROTOCOL_MCTP_DATA_INFO  *MctpInfo;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (ProtocolInfo != NULL);
  ASSERT (ProtocolInfo->CmObject != NULL);

  MctpInfo = ProtocolInfo->CmObject;

  if (MctpInfo->Version != MCTP_PROTOCOL_VERSION_INFO (2, 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid Version for MCTP protocol Info. Token=0x%lx, Version: 0x%x\n",
      __func__,
      ProtocolInfo->Token,
      MctpInfo->Version
      ));
    return EFI_INVALID_PARAMETER;
  }

  if ((MctpInfo->Characteristics & MCTP_CHARACTERISTIC_RESERVED_BITS) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid Characteristics value: 0x%x\n",
      __func__,
      MctpInfo->Characteristics
      ));
    return EFI_INVALID_PARAMETER;
  }

  ProtocolInfo->DataSize = sizeof (MCTP_PROTOCOL_DATA);

  return EFI_SUCCESS;
}

/** Get size protocol specific data of Redfish over IP.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       ProtocolInfo         ProtocolInfo.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSizeofRedfishOverIpProtocolSpecificData (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      MCHI_PROTOCOL_DATA_INFO                             *ProtocolInfo
  )
{
  CM_ARCH_COMMON_MCHI_PROTOCOL_REDFISH_OVER_IP_DATA_INFO  *RopInfo;
  UINTN                                                   DataSize;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (ProtocolInfo != NULL);
  ASSERT (ProtocolInfo->CmObject != NULL);

  RopInfo = ProtocolInfo->CmObject;

  if ((RopInfo->HostIpAssignType == RedfishIpTypeUnknown) ||
      (RopInfo->HostIpAssignType >= RedfishIpTypeMax) ||
      (RopInfo->HostIpAddressFormat == RedfishIpFormatUnknown) ||
      (RopInfo->HostIpAddressFormat >= RedfishIpFormatMax) ||
      (RopInfo->ServiceIpDiscoveryType == RedfishIpTypeUnknown) ||
      (RopInfo->ServiceIpDiscoveryType >= RedfishIpTypeMax) ||
      (RopInfo->ServiceIpAddressFormat == RedfishIpFormatUnknown) ||
      (RopInfo->ServiceIpAddressFormat >= RedfishIpFormatMax))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid Redfish over IP protocol information. Token=0x%lx\n",
      __func__,
      ProtocolInfo->Token
      ));
    return EFI_INVALID_PARAMETER;
  }

  DataSize = AsciiStrLen (RopInfo->Hostname);
  if (DataSize != 0) {
    DataSize++;  // for null-terminated.
  }

  DataSize += sizeof (REDFISH_PROTOCOL_OVER_IP_DATA);

  ProtocolInfo->DataSize = DataSize;

  return EFI_SUCCESS;
}

/** Add MCHI protocol specific data into buffer.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       ProtocolInfo         ProtocolInfo.
  @param [in]       StrTable             String Table.
  @param [out]      Buffer               Buffer.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddIpmiProtocolSpecificData (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      CONST MCHI_PROTOCOL_DATA_INFO                                *ProtocolInfo,
  IN      STRING_TABLE                                         *CONST  StrTable,
  OUT     VOID                                                         *Buffer
  )
{
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (ProtocolInfo != NULL);

  /*
   * SMBIOS 3.9.0 specification mentions the IPMI specification Appendix C1.
   * But, It doesn't say anything for 42 protocol specific data.
   * So, do nothing.
   */
  return EFI_SUCCESS;
}

/** Add MCHI protocol specific data into buffer.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       ProtocolInfo         ProtocolInfo.
  @param [in]       StrTable             String Table.
  @param [out]      Buffer               Buffer.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddMctpProtocolSpecificData (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      CONST MCHI_PROTOCOL_DATA_INFO                                *ProtocolInfo,
  IN      STRING_TABLE                                         *CONST  StrTable,
  OUT     VOID                                                         *Buffer
  )
{
  CM_ARCH_COMMON_MCHI_PROTOCOL_MCTP_DATA_INFO  *MctpInfo;
  MCTP_PROTOCOL_DATA                           *MctpData;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (ProtocolInfo != NULL);
  ASSERT (ProtocolInfo->CmObject != NULL);
  ASSERT (Buffer != NULL);

  MctpInfo = ProtocolInfo->CmObject;
  MctpData = Buffer;

  if ((MctpInfo->Characteristics & MCTP_CHARACTERISTIC_HAS_ACPI_DSDT_DEVICE) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: HAS_ACPI_DSDT_DEVICE characteristic doesn't support in generator currently.\n",
      __func__
      ));
    return EFI_INVALID_PARAMETER;
  }

  MctpData->Version         = MctpInfo->Version;
  MctpData->LinkLayerType   = MctpInfo->LinkLayerType;
  MctpData->Instance        = MctpInfo->Instance;
  MctpData->Characteristics = MctpInfo->Characteristics;

  return EFI_SUCCESS;
}

/** Add MCHI protocol specific data into buffer.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       ProtocolInfo         ProtocolInfo.
  @param [in]       StrTable             String Table.
  @param [out]      Buffer               Buffer.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddRedfishOverIpProtocolSpecificData (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      CONST MCHI_PROTOCOL_DATA_INFO                                *ProtocolInfo,
  IN      STRING_TABLE                                         *CONST  StrTable,
  OUT     VOID                                                         *Buffer
  )
{
  CM_ARCH_COMMON_MCHI_PROTOCOL_REDFISH_OVER_IP_DATA_INFO  *RopInfo;
  REDFISH_PROTOCOL_OVER_IP_DATA                           *RopData;
  VOID                                                    *Hostname;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (ProtocolInfo != NULL);
  ASSERT (ProtocolInfo->CmObject != NULL);
  ASSERT (Buffer != NULL);

  RopInfo  = ProtocolInfo->CmObject;
  RopData  = Buffer;
  Hostname = (RopData + 1);

  CopyGuid (&RopData->ServiceUuid, &RopInfo->ServiceUuid);
  RopData->HostIpAssignType    = RopInfo->HostIpAssignType;
  RopData->HostIpAddressFormat = RopInfo->HostIpAddressFormat;

  if ((RopData->HostIpAssignType == RedfishIpTypeStatic) ||
      (RopData->HostIpAssignType == RedfishIpTypeAutoConfigure))
  {
    CopyMem (
      RopData->HostIpAddress,
      RopInfo->HostIpAddress,
      sizeof (RopData->HostIpAddress)
      );

    CopyMem (
      RopData->HostIpMask,
      RopInfo->HostIpMask,
      sizeof (RopData->HostIpMask)
      );
  }

  RopData->ServiceIpDiscoveryType = RopInfo->ServiceIpDiscoveryType;
  RopData->ServiceIpAddressFormat = RopInfo->ServiceIpAddressFormat;

  if ((RopData->ServiceIpDiscoveryType == RedfishIpTypeStatic) ||
      (RopData->ServiceIpDiscoveryType == RedfishIpTypeAutoConfigure))
  {
    CopyMem (
      RopData->ServiceIpAddress,
      RopInfo->ServiceIpAddress,
      sizeof (RopData->ServiceIpAddress)
      );

    CopyMem (
      RopData->ServiceIpMask,
      RopInfo->ServiceIpMask,
      sizeof (RopData->ServiceIpMask)
      );

    RopData->ServiceIpPort = RopInfo->ServiceIpPort;
    RopData->ServiceVlanId = RopInfo->ServiceVlanId;
  }

  RopData->HostnameLength = (UINT8)AsciiStrLen (RopInfo->Hostname);
  if (RopData->HostnameLength > 0) {
    CopyMem (Hostname, RopInfo->Hostname, RopData->HostnameLength);
  }

  return EFI_SUCCESS;
}

/** MCHI protocol operation to get/add protocol specific data for
    SMBIOS Type 42 record.

    See the  DSP0256 version 2.0.0. (https://www.dmtf.org/dsp/DSP0256)
**/
STATIC MCHI_PROTOCOL_SPECIFIC_DATA_OPS  mMchiProtocolOps[] = {
  {
    MCHostInterfaceProtocolTypeIPMI,
    GetIpmiProtocolDataInfo,
    GetSizeofIpmiProtocolSpecificData,
    AddIpmiProtocolSpecificData,
  },
  {
    MCHostInterfaceProtocolTypeMCTP,
    GetMctpProtocolDataInfo,
    GetSizeofMctpProtocolSpecificData,
    AddMctpProtocolSpecificData,
  },
  {
    MCHostInterfaceProtocolTypeRedfishOverIP,
    GetRedfishOverIpProtocolDataInfo,
    GetSizeofRedfishOverIpProtocolSpecificData,
    AddRedfishOverIpProtocolSpecificData,
  },
};

/** Get MCHI Protocol Data operation.

  @param [in]       ProtocolType         Protocol Type.

  @retval MCHI_PROTOCOL_SPECIFIC_DATA_OPS
  @retval NULL                           No operation associated with ProtocolType.
**/
STATIC
MCHI_PROTOCOL_SPECIFIC_DATA_OPS *
EFIAPI
GetMchiProtocolOps (
  IN MC_HOST_INTERFACE_PROTOCOL_TYPE  ProtocolType
  )
{
  UINTN  Idx;

  for (Idx = 0; Idx < ARRAY_SIZE (mMchiProtocolOps); Idx++) {
    if (mMchiProtocolOps[Idx].ProtocolType == ProtocolType) {
      return &mMchiProtocolOps[Idx];
    }
  }

  return NULL;
}

/** Get MCHI Network USB Device descriptor CmObject associated with Token.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       Token                Device Descriptor Token.
  @param [in]       DeviceType           Device Type.
  @param [out]      CmObject             CM object associated device descriptor.
  @param [out]      Count                Number of CM object.

  @retval EFI_SUCCESS
  @retval Others                         Failed to get CM object
**/
STATIC
EFI_STATUS
EFIAPI
GetMchiNetworkUsbDeviceDescCmObj (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CM_OBJECT_TOKEN                                     Token,
  IN      MCHI_NETWORK_DEVICE_TYPE                            DeviceType,
  OUT     VOID                                                **CmObject,
  OUT     UINT32                                              *Count
  )
{
  EFI_STATUS                                        Status;
  CM_ARCH_COMMON_MCHI_NETWORK_DEVICE_DESC_USB_INFO  *UsbInfo;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Token != CM_NULL_TOKEN);
  ASSERT (CmObject != NULL);
  ASSERT (Count != NULL);

  Status = GetEArchCommonObjMchiNetworkDeviceDescUsbInfo (
             CfgMgrProtocol,
             Token,
             &UsbInfo,
             Count
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get MCHI Network USB device dsec info. Status=%r Token=0x%lx\n",
      __func__,
      Status,
      Token
      ));
    return Status;
  }

  *CmObject = (VOID *)UsbInfo;

  return EFI_SUCCESS;
}

/** Get MCHI Network PCI/PCIe Device descriptor CmObject associated with Token.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       Token                Device Descriptor Token.
  @param [in]       DeviceType           Device Type.
  @param [out]      CmObject             CM object associated device descriptor.
  @param [out]      Count                Number of CM object.

  @retval EFI_SUCCESS
  @retval Others                         Failed to get CM object
**/
STATIC
EFI_STATUS
EFIAPI
GetMchiNetworkPciDeviceDescCmObj (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CM_OBJECT_TOKEN                                     Token,
  IN      MCHI_NETWORK_DEVICE_TYPE                            DeviceType,
  OUT     VOID                                                **CmObject,
  OUT     UINT32                                              *Count
  )
{
  EFI_STATUS                                        Status;
  CM_ARCH_COMMON_MCHI_NETWORK_DEVICE_DESC_PCI_INFO  *PciInfo;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Token != CM_NULL_TOKEN);
  ASSERT (CmObject != NULL);
  ASSERT (Count != NULL);

  Status = GetEArchCommonObjMchiNetworkDeviceDescPciInfo (
             CfgMgrProtocol,
             Token,
             &PciInfo,
             Count
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get MCHI Network USB device dsec info. Status=%r Token=0x%lx\n",
      __func__,
      Status,
      Token
      ));
    return Status;
  }

  *CmObject = (VOID *)PciInfo;

  return EFI_SUCCESS;
}

/** Get size of MCHI Network Usb Device descriptor data.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CmObject             CM object.
  @param [out]      Size                 Size of Device descriptor data.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSizeofMchiNetworkUsbDeviceDescData (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST VOID                                          *CmObject,
  OUT     UINTN                                               *Size
  )
{
  EFI_STATUS                                              Status;
  CONST CM_ARCH_COMMON_MCHI_NETWORK_DEVICE_DESC_USB_INFO  *UsbInfo;
  UINTN                                                   ByteSize;
  CHAR16                                                  TmpUnicodeStr[SMBIOS_MAX_STRING_SIZE_REDUCED];

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (CmObject != NULL);
  ASSERT (Size != NULL);

  UsbInfo = CmObject;

  switch (UsbInfo->Version) {
    case 1:
      Status = AsciiStrToUnicodeStrS (
                 UsbInfo->SerialNumberStr,
                 TmpUnicodeStr,
                 SMBIOS_MAX_STRING_SIZE_REDUCED
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to convert to unicode string. Status=%r\n",
          __func__,
          Status
          ));
        return Status;
      }

      // exclude last NULL character.
      ByteSize = StrSize (TmpUnicodeStr) - sizeof (CHAR16);
      if (ByteSize < 2) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Serial number string byte size must be at least more that 2.\n",
          __func__
          ));
        return EFI_INVALID_PARAMETER;
      }

      ///
      /// BytesSize is the size of Serial number unicode string witout
      /// NULL terminator.
      ///
      *Size = sizeof (MCHI_NETWORK_DEVICE_DESC_USB_DATA) + ByteSize;
      break;
    case 2:
      if ((UsbInfo->Characteristic & MCHI_NETWORK_DEVICE_CHARACTERISTIC_RESERVED_BITS) != 0) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Invalid Characteristic value: 0x%x\n",
          __func__,
          UsbInfo->Characteristic
          ));
        return EFI_INVALID_PARAMETER;
      }

      if (((UsbInfo->Characteristic & MCHI_NETWORK_DEVICE_CHARACTERISTIC_CREDENTIAL_VIA_IPMI_SUPPORT) != 0) &&
          (UsbInfo->IpmiToken == CM_NULL_TOKEN))
      {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Invalid IpmiToken for credential.\n",
          __func__
          ));
        return EFI_INVALID_PARAMETER;
      }

      *Size  = sizeof (MCHI_NETWORK_DEVICE_DESC_USB_V2_DATA);
      Status = EFI_SUCCESS;
      break;
    default:
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid version. Version=0x%lx\n",
        __func__,
        UsbInfo->Version
        ));
      Status = EFI_INVALID_PARAMETER;
  }

  return Status;
}

/** Get size of MCHI Network PCI/PCIe Device descriptor data.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CmObject             CM object.
  @param [out]      Size                 Size of Device descriptor data.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSizeofMchiNetworkPciDeviceDescData (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST VOID                                          *CmObject,
  OUT     UINTN                                               *Size
  )
{
  EFI_STATUS                                              Status;
  CONST CM_ARCH_COMMON_MCHI_NETWORK_DEVICE_DESC_PCI_INFO  *PciInfo;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (CmObject != NULL);
  ASSERT (Size != NULL);

  PciInfo = CmObject;

  switch (PciInfo->Version) {
    case 1:
      *Size  = sizeof (MCHI_NETWORK_DEVICE_DESC_PCI_DATA);
      Status = EFI_SUCCESS;
      break;
    case 2:
      if ((PciInfo->Characteristic & MCHI_NETWORK_DEVICE_CHARACTERISTIC_RESERVED_BITS) != 0) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Invalid Characteristic value: 0x%x\n",
          __func__,
          PciInfo->Characteristic
          ));
        return EFI_INVALID_PARAMETER;
      }

      if (((PciInfo->Characteristic & MCHI_NETWORK_DEVICE_CHARACTERISTIC_CREDENTIAL_VIA_IPMI_SUPPORT) != 0) &&
          (PciInfo->IpmiToken == CM_NULL_TOKEN))
      {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Invalid IpmiToken for credential.\n",
          __func__
          ));
        return EFI_INVALID_PARAMETER;
      }

      *Size  = sizeof (MCHI_NETWORK_DEVICE_DESC_PCI_V2_DATA);
      Status = EFI_SUCCESS;
      break;
    default:
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid version. Version=0x%lx\n",
        __func__,
        PciInfo->Version
        ));
      Status = EFI_INVALID_PARAMETER;
  }

  return Status;
}

/** Add MCHI Network USB Device specific data into buffer.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CmObject             CM object.
  @param [in]       StrTable             String Table.
  @param [out]      Buffer               Buffer.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddMchiNetworkUsbDeviceDescData (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      CONST VOID                                                   *CmObject,
  IN      STRING_TABLE                                         *CONST  StrTable,
  OUT     VOID                                                         *Buffer
  )
{
  EFI_STATUS                                              Status;
  CONST CM_ARCH_COMMON_MCHI_NETWORK_DEVICE_DESC_USB_INFO  *UsbInfo;
  MCHI_NETWORK_DEVICE_DESC_USB_DATA                       *UsbData;
  MCHI_NETWORK_DEVICE_DESC_USB_V2_DATA                    *Usbv2Data;
  UINT8                                                   UnicodeStrRef;
  CHAR16                                                  UnicodeStr[SMBIOS_MAX_STRING_SIZE_REDUCED];
  SMBIOS_HANDLE                                           Type38Handle;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (CmObject != NULL);
  ASSERT (Buffer != NULL);

  UsbInfo = CmObject;
  Status  = EFI_SUCCESS;

  switch (UsbInfo->Version) {
    case 1:
      UsbData = Buffer;

      UsbData->VendorId  = UsbInfo->VendorId;
      UsbData->ProductId = UsbInfo->ProductId;
      // Unicode String. Always 0x03 according to specification.
      UsbData->SerialNumberDescType = 0x03;

      Status = AsciiStrToUnicodeStrS (
                 UsbInfo->SerialNumberStr,
                 UnicodeStr,
                 SMBIOS_MAX_STRING_SIZE_REDUCED
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to convert to unicode string. Status=%r\n",
          __func__,
          Status
          ));
        return Status;
      }

      // exclude last NULL character.
      UsbData->SerialNumberDescLength = (UINT8)(StrSize (UnicodeStr) - sizeof (CHAR16));
      CopyMem ((VOID *)(UsbData + 1), UnicodeStr, UsbData->SerialNumberDescLength);
      break;
    case 2:
      ASSERT (StrTable != NULL);

      Usbv2Data = Buffer;

      /// Legnth includes size of *DeviceType* field.
      Usbv2Data->Length         = sizeof (MCHI_NETWORK_DEVICE_DESC_USB_V2_DATA) + sizeof (UINT8);
      Usbv2Data->VendorId       = UsbInfo->VendorId;
      Usbv2Data->ProductId      = UsbInfo->ProductId;
      Usbv2Data->Characteristic = UsbInfo->Characteristic;

      CopyMem (
        Usbv2Data->MacAddress,
        UsbInfo->MacAddress,
        sizeof (Usbv2Data->MacAddress)
        );

      Status = StringTableAddString (
                 StrTable,
                 UsbInfo->SerialNumberStr,
                 &UnicodeStrRef
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to add string table. Status=%r\n",
          __func__,
          Status
          ));
        return Status;
      }

      Usbv2Data->SerialNumberRef = UnicodeStrRef;

      if (UsbInfo->IpmiToken != CM_NULL_TOKEN) {
        Type38Handle = FindSmbiosHandleEx (
                         CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType38),
                         UsbInfo->IpmiToken
                         );
        if (Type38Handle == SMBIOS_HANDLE_INVALID) {
          DEBUG ((
            DEBUG_ERROR,
            "%a: Failed to find out Type38Handle. Token: 0x%lx\n",
            __func__,
            UsbInfo->IpmiToken
            ));
          return EFI_NOT_FOUND;
        }
      } else {
        Type38Handle = SMBIOS_HANDLE_INVALID;
      }

      Usbv2Data->CredentialBootStrappingHandle = Type38Handle;
      break;
    default:
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid version. Version=0x%lx\n",
        __func__,
        UsbInfo->Version
        ));
      Status = EFI_INVALID_PARAMETER;
  }

  return Status;
}

/** Add MCHI Network PCI Device specific data into buffer.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CmObject             CM object.
  @param [in]       StrTable             String Table.
  @param [out]      Buffer               Buffer.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddMchiNetworkPciDeviceDescData (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      CONST VOID                                                   *CmObject,
  IN      STRING_TABLE                                         *CONST  StrTable,
  OUT     VOID                                                         *Buffer
  )
{
  EFI_STATUS                                              Status;
  CONST CM_ARCH_COMMON_MCHI_NETWORK_DEVICE_DESC_PCI_INFO  *PciInfo;
  MCHI_NETWORK_DEVICE_DESC_PCI_DATA                       *PciData;
  MCHI_NETWORK_DEVICE_DESC_PCI_V2_DATA                    *Pciv2Data;
  SMBIOS_HANDLE                                           Type38Handle;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (CmObject != NULL);
  ASSERT (Buffer != NULL);

  PciInfo = CmObject;
  Status  = EFI_SUCCESS;

  switch (PciInfo->Version) {
    case 1:
      PciData = Buffer;

      PciData->VendorId          = PciInfo->VendorId;
      PciData->DeviceId          = PciInfo->DeviceId;
      PciData->SubSystemVendorId = PciInfo->SubSystemVendorId;
      PciData->SubSystemId       = PciInfo->SubSystemId;
      break;
    case 2:
      Pciv2Data = Buffer;

      /// Legnth includes size of *DeviceType* field.
      Pciv2Data->Length            = sizeof (MCHI_NETWORK_DEVICE_DESC_PCI_V2_DATA) + sizeof (UINT8);
      Pciv2Data->VendorId          = PciInfo->VendorId;
      Pciv2Data->DeviceId          = PciInfo->DeviceId;
      Pciv2Data->SubSystemVendorId = PciInfo->SubSystemVendorId;
      Pciv2Data->SubSystemId       = PciInfo->SubSystemId;
      Pciv2Data->Segment           = PciInfo->Segment;
      Pciv2Data->Bus               = PciInfo->Bus;
      Pciv2Data->Function          = PciInfo->Function;
      Pciv2Data->Characteristic    = PciInfo->Characteristic;

      CopyMem (
        Pciv2Data->MacAddress,
        PciInfo->MacAddress,
        sizeof (Pciv2Data->MacAddress)
        );

      if (PciInfo->IpmiToken != CM_NULL_TOKEN) {
        Type38Handle = FindSmbiosHandleEx (
                         CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType38),
                         PciInfo->IpmiToken
                         );
        if (Type38Handle == SMBIOS_HANDLE_INVALID) {
          DEBUG ((
            DEBUG_ERROR,
            "%a: Failed to find out Type38Handle. Token: 0x%lx\n",
            __func__,
            PciInfo->IpmiToken
            ));
          return EFI_NOT_FOUND;
        }
      } else {
        Type38Handle = SMBIOS_HANDLE_INVALID;
      }

      Pciv2Data->CredentialBootStrappingHandle = Type38Handle;
      break;
    default:
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid version. Version=0x%lx\n",
        __func__,
        PciInfo->Version
        ));
      Status = EFI_INVALID_PARAMETER;
  }

  return Status;
}

/** MCHI Network device descriptor operation to get/add device descriptor
    data for SMBIOS Type 42 record.

    See the DSP0270 version 1.3.1 (https://www.dmtf.org/dsp/DSP0270)
**/
STATIC MCHI_NETWORK_DEVICE_DESC_OPS  mMchiNetworkDeviceOps[] = {
  {
    MchiNetworkDeviceTypeUsb,
    GetMchiNetworkUsbDeviceDescCmObj,
    GetSizeofMchiNetworkUsbDeviceDescData,
    AddMchiNetworkUsbDeviceDescData,
  },
  {
    MchiNetworkDeviceTypePci,
    GetMchiNetworkPciDeviceDescCmObj,
    GetSizeofMchiNetworkPciDeviceDescData,
    AddMchiNetworkPciDeviceDescData,
  },
  {
    MchiNetworkDeviceTypeUsbv2,
    GetMchiNetworkUsbDeviceDescCmObj,
    GetSizeofMchiNetworkUsbDeviceDescData,
    AddMchiNetworkUsbDeviceDescData,
  },
  {
    MchiNetworkDeviceTypePciv2,
    GetMchiNetworkPciDeviceDescCmObj,
    GetSizeofMchiNetworkPciDeviceDescData,
    AddMchiNetworkPciDeviceDescData,
  },
};

/** Get MCHI Network Data operation.

  @param [in]       DeviceType           Device Type.

  @retval MCHI_NETWORK_DEVICE_DESC_OPS
  @retval NULL                           No operation associated with DeviceType.
**/
STATIC
MCHI_NETWORK_DEVICE_DESC_OPS *
EFIAPI
GetMchiNetDevOps (
  IN MCHI_NETWORK_DEVICE_TYPE  DeviceType
  )
{
  UINTN  Idx;

  for (Idx = 0; Idx < ARRAY_SIZE (mMchiNetworkDeviceOps); Idx++) {
    if (mMchiNetworkDeviceOps[Idx].DeviceType == DeviceType) {
      return &mMchiNetworkDeviceOps[Idx];
    }
  }

  return NULL;
}

/** Get MCHI MCTP Data CM objects.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager.
                                         Protocol Interface.
  @param [in]       Token                MCHI Data Token.
  @param [in]       InterfaceType        Interface Type.
  @param [out]      CmObject             MCHI Data CM object.
  @param [out]      Count                Number of MCHI Data CM object.

  @retval EFI_SUCCESS
  @retval Others                         Failed to initialise
**/
STATIC
EFI_STATUS
EFIAPI
GetMchiMctpDataCmObj (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CM_OBJECT_TOKEN                                     Token,
  IN      MC_HOST_INTERFACE_TYPE                              InterfaceType,
  OUT     VOID                                                **CmObject,
  OUT     UINT32                                              *Count
  )
{
  EFI_STATUS                          Status;
  CM_ARCH_COMMON_MCHI_MCTP_DATA_INFO  *DataInfo;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (CmObject != NULL);
  ASSERT (Count != NULL);

  if (InterfaceType != MCHostInterfaceTypeMMBI) {
    *CmObject = NULL;
    return EFI_SUCCESS;
  }

  ASSERT (Token != CM_NULL_TOKEN);

  Status = GetEArchCommonObjMchiMctpDataInfo (
             CfgMgrProtocol,
             Token,
             &DataInfo,
             Count
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get MCHI MCTP Datainfo. Status=%r Token=0x%lx\n",
      __func__,
      Status,
      Token
      ));
    return Status;
  }

  *CmObject = (VOID *)DataInfo;

  return EFI_SUCCESS;
}

/** Get MCHI Network Data CM objects.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager.
                                         Protocol Interface.
  @param [in]       Token                MCHI Data Token.
  @param [in]       InterfaceType        Interface Type.
  @param [out]      CmObject             MCHI Data CM object.
  @param [out]      Count                Number of MCHI Data CM object.

  @retval EFI_SUCCESS
  @retval Others                         Failed to initialise
**/
STATIC
EFI_STATUS
EFIAPI
GetMchiNetworkDataCmObj (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CM_OBJECT_TOKEN                                     Token,
  IN      MC_HOST_INTERFACE_TYPE                              InterfaceType,
  OUT     VOID                                                **CmObject,
  OUT     UINT32                                              *Count
  )
{
  EFI_STATUS                             Status;
  CM_ARCH_COMMON_MCHI_NETWORK_DATA_INFO  *DataInfo;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Token != CM_NULL_TOKEN);
  ASSERT (CmObject != NULL);
  ASSERT (Count != NULL);

  Status = GetEArchCommonObjMchiNetworkDataInfo (
             CfgMgrProtocol,
             Token,
             &DataInfo,
             Count
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get MCHI Network Datainfo. Status=%r Token=0x%lx\n",
      __func__,
      Status,
      Token
      ));
    return Status;
  }

  *CmObject = (VOID *)DataInfo;

  return EFI_SUCCESS;
}

/** Get size of MCHI MCTP Data.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager.
                                         Protocol Interface.
  @param [in]       InterfaceType        Interface Type.
  @param [in]       CmObject             MCHI MCTP Data CM object.
  @param [out]      Size                 Size of MCHI MCTP Data.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSizeofMchiMctpData (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      MC_HOST_INTERFACE_TYPE                              InterfaceType,
  IN      CONST VOID                                          *CmObject,
  OUT     UINTN                                               *Size
  )
{
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Size != NULL);

  switch (InterfaceType) {
    case MCHostInterfaceTypeMMBI:
      *Size = sizeof (UINT64);
      break;
    case MCHostInterfaceTypeI2C_SMBUS:
    case MCHostInterfaceTypeI3C:
    case MCHostInterfaceTypePCC:
    case MCHostInterfaceTypeUSB:
      *Size = sizeof (UINT32);
      break;
    default:
      /*
       * According to SMBIOS specification, minimum data size is 4 bytes.
       */
      *Size = sizeof (UINT32);
  }

  return EFI_SUCCESS;
}

/** Get size of MCHI Newtwork Data.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager.
                                         Protocol Interface.
  @param [in]       InterfaceType        Interface Type.
  @param [in]       CmObject             MCHI Network Data CM object.
  @param [out]      Size                 Size of MCHI Network Data.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSizeofMchiNetworkData (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      MC_HOST_INTERFACE_TYPE                              InterfaceType,
  IN      CONST VOID                                          *CmObject,
  OUT     UINTN                                               *Size
  )
{
  EFI_STATUS                                   Status;
  CONST CM_ARCH_COMMON_MCHI_NETWORK_DATA_INFO  *DataInfo;
  MCHI_NETWORK_DEVICE_DESC_OPS                 *NetDevOps;
  UINT32                                       Count;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (CmObject != NULL);
  ASSERT (Size != NULL);

  DataInfo = CmObject;

  NetDevOps = GetMchiNetDevOps (DataInfo->DeviceType);
  if (NetDevOps == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid Network Device Type: 0x%x\n",
      __func__,
      DataInfo->DeviceType
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = NetDevOps->GetMchiNetworkDeviceDescCmObj (
                        CfgMgrProtocol,
                        DataInfo->DeviceDataToken,
                        DataInfo->DeviceType,
                        &NetDevOps->CmObject,
                        &Count
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get MCHI Network device descriptor CM Object. Status=%r\n",
      __func__,
      Status
      ));
    return Status;
  }

  if (Count != 1) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Network Device Data Token should be unique. Token=0x%lx\n",
      __func__,
      DataInfo->DeviceDataToken
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = NetDevOps->GetSizeofMchiNetworkDeviceDescData (
                        CfgMgrProtocol,
                        NetDevOps->CmObject,
                        &NetDevOps->DataSize
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get size of MCHI Network device descriptor data. Status=%r\n",
      __func__,
      Status
      ));
    return Status;
  }

  *Size = sizeof (MCHI_NETWORK_SPECIFIC_DATA) + NetDevOps->DataSize;

  return EFI_SUCCESS;
}

/** Add MCHI MCTP Data.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       InterfaceType        Interface Type.
  @param [in]       CmObject             CM object of Processor Specific Data.
  @param [in]       StrTable             String Table.
  @param [out]      Buffer               Buffer.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddMchiMctpData (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      MC_HOST_INTERFACE_TYPE                                       InterfaceType,
  IN      CONST VOID                                                   *CmObject,
  IN      STRING_TABLE                                         *CONST  StrTable,
  OUT     VOID                                                         *Buffer
  )
{
  CONST CM_ARCH_COMMON_MCHI_MCTP_DATA_INFO  *MctpInfo;
  MCHI_MCTP_MMBI_SPECIFIC_DATA              *MmbiData;
  UINT32                                    *Reserved;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Buffer != NULL);

  switch (InterfaceType) {
    case MCHostInterfaceTypeMMBI:
      ASSERT (CmObject != NULL);

      MctpInfo                    = CmObject;
      MmbiData                    = Buffer;
      MmbiData->MmbiCapDesPointer =  MctpInfo->MmbiCapDesPointer;
      break;
    case MCHostInterfaceTypeI2C_SMBUS:
    case MCHostInterfaceTypeI3C:
    case MCHostInterfaceTypePCC:
    case MCHostInterfaceTypeUSB:
      Reserved  = Buffer;
      *Reserved = 0x00;
      break;
    default:
      return EFI_SUCCESS;
  }

  return EFI_SUCCESS;
}

/** Add MCHI Network Data.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       InterfaceType        Interface Type.
  @param [in]       CmObject             CM object of Processor Specific Data.
  @param [in]       StrTable             String Table.
  @param [out]      Buffer               Buffer.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddMchiNetworkData (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      MC_HOST_INTERFACE_TYPE                                       InterfaceType,
  IN      CONST VOID                                                   *CmObject,
  IN      STRING_TABLE                                         *CONST  StrTable,
  OUT     VOID                                                         *Buffer
  )
{
  EFI_STATUS                                   Status;
  CONST CM_ARCH_COMMON_MCHI_NETWORK_DATA_INFO  *DataInfo;
  MCHI_NETWORK_DEVICE_DESC_OPS                 *NetDevOps;
  MCHI_NETWORK_SPECIFIC_DATA                   *InterfaceData;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (CmObject != NULL);
  ASSERT (Buffer != NULL);

  DataInfo      = CmObject;
  InterfaceData = Buffer;

  NetDevOps = GetMchiNetDevOps (DataInfo->DeviceType);
  if (NetDevOps == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid Network Device Type: 0x%x\n",
      __func__,
      DataInfo->DeviceType
      ));
    return EFI_INVALID_PARAMETER;
  }

  InterfaceData->DeviceType = DataInfo->DeviceType;

  Status = NetDevOps->AddMchiNetworkDeviceDescData (
                        CfgMgrProtocol,
                        NetDevOps->CmObject,
                        StrTable,
                        (VOID *)(InterfaceData + 1)
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to add Mchi Network device descriptor data.  Status=%r\n",
      __func__,
      Status
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

/** MCHI data operation to get/add device descriptor data for
    SMBIOS Type 42 record.
**/
STATIC MCHI_DATA_OPS  mMchiDataOps[] = {
  {
    /// 0x00 - 0x3F, MCTP Host Interface, DSP0239.
    MchiMctpDataOpsIdx,
    GetMchiMctpDataCmObj,
    GetSizeofMchiMctpData,
    AddMchiMctpData,
  },
  {
    /// 0x40, Network Host Interface, DSP0270.
    MchiNetworkDataOpsIdx,
    GetMchiNetworkDataCmObj,
    GetSizeofMchiNetworkData,
    AddMchiNetworkData,
  },
};

/** Get MCHI Data operation.

  @param [in]       InterfaceType        Interface Type.

  @retval MCHI_DATA_OPS
  @retval NULL                           No operation associated with InterfaceType.
**/
STATIC
MCHI_DATA_OPS *
EFIAPI
GetMchiDataOps (
  IN MC_HOST_INTERFACE_TYPE  InterfaceType
  )
{
  UINT8  OpsIdx;
  UINT8  Idx;

  switch (InterfaceType) {
    case MCHostInterfaceTypeKCS:
    case MCHostInterfaceType8250_UARTRegisterCompatible:
    case MCHostInterfaceType16450_UARTRegisterCompatible:
    case MCHostInterfaceType16550_16550A_UARTRegisterCompatible:
    case MCHostInterfaceType16650_16650A_UARTRegisterCompatible:
    case MCHostInterfaceType16750_16750A_UARTRegisterCompatible:
    case MCHostInterfaceType16850_16850A_UARTRegisterCompatible:
    case MCHostInterfaceTypeI2C_SMBUS:
    case MCHostInterfaceTypeI3C:
    case MCHostInterfaceTypePCIeVDM:
    case MCHostInterfaceTypeMMBI:
    case MCHostInterfaceTypePCC:
    case MCHostInterfaceTypeUCIe:
    case MCHostInterfaceTypeUSB:
      OpsIdx = MchiMctpDataOpsIdx;
      break;
    case MCHostInterfaceTypeNetworkHostInterface:
      OpsIdx = MchiNetworkDataOpsIdx;
      break;
    case MCHostInterfaceTypeOemDefined:
      OpsIdx = MchiOemDataOpsIdx;
      break;
    default:
      DEBUG ((
        DEBUG_ERROR,
        "%a:Invalid Interface Type: 0x%lx\n",
        __func__,
        InterfaceType
        ));
      return NULL;
  }

  for (Idx = 0; Idx < ARRAY_SIZE (mMchiDataOps); Idx++) {
    if (mMchiDataOps[Idx].MchiDataOpsIdx == OpsIdx) {
      return &mMchiDataOps[Idx];
    }
  }

  return NULL;
}

/** Free any resources allocated when installing SMBIOS Type44 table.

 @param [in]  This                 Pointer to the SMBIOS table generator.
 @param [in]  TableFactoryProtocol Pointer to the SMBIOS Table Factory
                                   Protocol interface.
 @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table information.
 @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                   Protocol interface.
 @param [in]  Table                Pointer to the SMBIOS table.
 @param [in]  CmObjectToken        Pointer to the CM ObjectToken Array.
 @param [in]  TableCount           Number of SMBIOS tables.

 @retval EFI_SUCCESS            Table generated successfully.
 @retval EFI_BAD_BUFFER_SIZE    The size returned by the Configuration
                                Manager is less than the Object size for
                                the requested object.
 @retval EFI_INVALID_PARAMETER  A parameter is invalid.
 @retval EFI_NOT_FOUND          Could not find information.
 @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
 @retval EFI_UNSUPPORTED        Unsupported configuration.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSmbiosType42TableEx (
  IN      CONST SMBIOS_TABLE_GENERATOR                    *CONST   This,
  IN      CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL      *CONST   TableFactoryProtocol,
  IN      CONST CM_STD_OBJ_SMBIOS_TABLE_INFO              *CONST   SmbiosTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL      *CONST   CfgMgrProtocol,
  IN      SMBIOS_STRUCTURE                               ***CONST  Table,
  IN      CM_OBJECT_TOKEN                                          **CmObjectToken,
  IN      CONST UINTN                                              TableCount
  )
{
  UINTN             Index;
  SMBIOS_STRUCTURE  **TableList;

  TableList = *Table;
  for (Index = 0; Index < TableCount; Index++) {
    if (TableList[Index] != NULL) {
      FreePool (TableList[Index]);
    }
  }

  if (TableList != NULL) {
    FreePool (TableList);
  }

  CleanupMchiProtocolList ();

  return EFI_SUCCESS;
}

/** Construct SMBIOS Type 42 Table describing Management Controller
    Host Interface.

  If this function allocates any resources then they must be freed
  in the FreeXXXXTableResources function.

 @param [in]  This                 Pointer to the SMBIOS table generator.
 @param [in]  TableFactoryProtocol Pointer to the SMBIOS Table Factory
                                   Protocol interface.
 @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table information.
 @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                   Protocol interface.
 @param [out] Table                Pointer to the SMBIOS table.
 @param [out] CmObjectToken        Pointer to the CM Object Token Array.
 @param [out] TableCount           Number of tables installed.

 @retval EFI_SUCCESS            Table generated successfully.
 @retval EFI_BAD_BUFFER_SIZE    The size returned by the Configuration
                                Manager is less than the Object size for
                                the requested object.
 @retval EFI_INVALID_PARAMETER  A parameter is invalid.
 @retval EFI_NOT_FOUND          Could not find information.
 @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
 @retval EFI_UNSUPPORTED        Unsupported configuration.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSmbiosType42TableEx (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               ***Table,
  OUT       CM_OBJECT_TOKEN                                **CmObjectToken,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                         Status;
  SMBIOS_STRUCTURE                   **TableList;
  SMBIOS_TABLE_TYPE42                *SmbiosRecord;
  UINTN                              SmbiosRecordSize;
  UINT8                              *Buffer;
  STRING_TABLE                       StrTable;
  CM_ARCH_COMMON_MCHI_INFO           *MchiList;
  UINT32                             MchiCount;
  CM_ARCH_COMMON_OBJ_REF             *ProtocolRefList;
  UINT32                             ProtocolCount;
  CM_ARCH_COMMON_MCHI_PROTOCOL_INFO  *ProtocolInfo;
  MCHI_PROTOCOL_DATA_INFO            *ProtocolDataInfo;
  LIST_ENTRY                         *Entry;
  UINT32                             Count;
  UINTN                              InterfaceIdx;
  UINTN                              ProtocolIdx;
  MCHI_DATA_OPS                      *DataOps;
  MCHI_PROTOCOL_SPECIFIC_DATA_OPS    *ProtocolOps;
  UINTN                              MchiDataSize;
  UINTN                              ProtocolSize;

  ASSERT (This != NULL);
  ASSERT (SmbiosTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (TableCount != NULL);
  ASSERT (SmbiosTableInfo->TableGeneratorId == This->GeneratorID);

  if ((This == NULL) || (SmbiosTableInfo == NULL) || (CfgMgrProtocol == NULL) ||
      (Table == NULL) || (TableCount == NULL) || (CmObjectToken == NULL) ||
      (SmbiosTableInfo->TableGeneratorId != This->GeneratorID))
  {
    DEBUG ((DEBUG_ERROR, "%a:Invalid Paramater\n ", __func__));
    return EFI_INVALID_PARAMETER;
  }

  TableList   = NULL;
  *Table      = NULL;
  *TableCount = 0;

  Status = GetEArchCommonObjMchiInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &MchiList,
             &MchiCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get MCHI info. Status = %r\n",
      __func__,
      Status
      ));
    return EFI_INVALID_PARAMETER;
  }

  TableList = (SMBIOS_STRUCTURE **)AllocateZeroPool (sizeof (SMBIOS_STRUCTURE *) * MchiCount);
  if (TableList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to alloc memory for %u MCHI table\n",
      __func__,
      MchiCount
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  for (InterfaceIdx = 0; InterfaceIdx < MchiCount; InterfaceIdx++) {
    SmbiosRecordSize  = OFFSET_OF (SMBIOS_TABLE_TYPE42, InterfaceTypeSpecificData);
    SmbiosRecordSize += sizeof (UINT8); // Number of Protocol Record Field size.
    MchiDataSize      = 0;
    ProtocolSize      = 0;

    DataOps = GetMchiDataOps (MchiList[InterfaceIdx].InterfaceType);
    if (DataOps == NULL) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Unsupported Interface Type: 0x%x\n",
        __func__,
        MchiList[InterfaceIdx].InterfaceType
        ));
      Status = EFI_INVALID_PARAMETER;
      goto ErrorHandler;
    }

    Status = DataOps->GetMchiDataCmObj (
                        CfgMgrProtocol,
                        MchiList[InterfaceIdx].InterfaceDataToken,
                        MchiList[InterfaceIdx].InterfaceType,
                        &DataOps->CmObject,
                        &Count
                        );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to get MCHI inteface data CM object. Token: %lx\n",
        __func__,
        MchiList[InterfaceIdx].InterfaceDataToken
        ));
      goto ErrorHandler;
    }

    if (Count != 1) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: MCHI Data Token should be unique. Token=0x%lx\n",
        __func__,
        MchiList[InterfaceIdx].InterfaceDataToken
        ));
      Status = EFI_INVALID_PARAMETER;
      goto ErrorHandler;
    }

    Status = DataOps->GetSizeofMchiData (
                        CfgMgrProtocol,
                        MchiList[InterfaceIdx].InterfaceType,
                        DataOps->CmObject,
                        &MchiDataSize
                        );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to get MCHI interface data size.\n",
        __func__
        ));
      goto ErrorHandler;
    }

    Status = GetEArchCommonObjCmRef (
               CfgMgrProtocol,
               MchiList[InterfaceIdx].ProtocolTokenArray.ReferenceToken,
               &ProtocolRefList,
               &ProtocolCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to get MCHI Protocol Reference List.\n",
        __func__
        ));
      goto ErrorHandler;
    }

    if (ProtocolCount > MAX_UINT8) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Too many Protocol..\n",
        __func__
        ));
      goto ErrorHandler;
    }

    for (ProtocolIdx = 0; ProtocolIdx < ProtocolCount; ProtocolIdx++) {
      Status = GetEArchCommonObjMchiProtocolInfo (
                 CfgMgrProtocol,
                 ProtocolRefList[ProtocolIdx].ReferenceToken,
                 &ProtocolInfo,
                 &Count
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to get MCHI Protocol Info. Token=0x%lx, Status=%r\n",
          __func__,
          ProtocolRefList[ProtocolIdx].ReferenceToken,
          Status
          ));
        goto ErrorHandler;
      }

      if (Count != 1) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Protocol Info Token should be unique. Token=0x%lx\n",
          __func__,
          ProtocolRefList[ProtocolIdx].ReferenceToken
          ));
        Status = EFI_INVALID_PARAMETER;
        goto ErrorHandler;
      }

      ProtocolOps = GetMchiProtocolOps (ProtocolInfo->ProtocolType);
      if (ProtocolOps == NULL) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Unsupported Protocol Type: 0x%x\n",
          __func__,
          ProtocolInfo->ProtocolType
          ));
        Status = EFI_INVALID_PARAMETER;
        goto ErrorHandler;
      }

      Status = ProtocolOps->GetMchiProtocolDataInfo (
                              CfgMgrProtocol,
                              ProtocolInfo->ProtocolDataToken,
                              ProtocolInfo->ProtocolType,
                              &ProtocolDataInfo
                              );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to get Protocol Data Info. Token=0x%lx\n",
          __func__,
          ProtocolInfo->ProtocolDataToken
          ));
        goto ErrorHandler;
      }

      Status = ProtocolOps->GetSizeofMchiProtocolSpecificData (
                              CfgMgrProtocol,
                              ProtocolDataInfo
                              );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to get Protocol Data Info. Token=0x%lx\n",
          __func__,
          ProtocolInfo->ProtocolDataToken
          ));
        goto ErrorHandler;
      }

      ProtocolSize += ProtocolDataInfo->DataSize;
    }

    // Per protocol, Protocol Type (UINT8) + Protocol data length (UINT8).
    ProtocolSize += ProtocolCount * (sizeof (UINT8) + sizeof (UINT8));

    SmbiosRecordSize += MchiDataSize;
    SmbiosRecordSize += ProtocolSize;
    if (SmbiosRecordSize > MAX_UINT8) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Too big SmbiosRecordSize. Size=0x%lx\n",
        __func__,
        SmbiosRecordSize
        ));
      goto ErrorHandler;
    }

    SmbiosRecord = AllocateZeroPool (SmbiosRecordSize);
    if (SmbiosRecord == NULL) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to allocate SmbiosRecord.\n",
        __func__
        ));
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorHandler;
    }

    Buffer = (UINT8 *)SmbiosRecord;

    Status = StringTableInitialize (&StrTable, SMBIOS_TYPE42_MAX_STRINGS);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to initialize string table. Status = %r\n",
        __func__,
        Status
        ));
      FreePool (SmbiosRecord);
      goto ErrorHandler;
    }

    // Set up the header
    SmbiosRecord->Hdr.Type   = EFI_SMBIOS_TYPE_MANAGEMENT_CONTROLLER_HOST_INTERFACE;
    SmbiosRecord->Hdr.Length = (UINT8)SmbiosRecordSize;

    SmbiosRecord->InterfaceType                   = MchiList[InterfaceIdx].InterfaceType;
    SmbiosRecord->InterfaceTypeSpecificDataLength = (UINT8)MchiDataSize;

    Buffer += OFFSET_OF (SMBIOS_TABLE_TYPE42, InterfaceTypeSpecificData);

    Status = DataOps->AddMchiData (
                        CfgMgrProtocol,
                        MchiList[InterfaceIdx].InterfaceType,
                        DataOps->CmObject,
                        &StrTable,
                        Buffer
                        );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to add MCHI interface data into SmbiosRecord Status=%r.\n",
        __func__,
        Status
        ));
      StringTableFree (&StrTable);
      FreePool (SmbiosRecord);
      goto ErrorHandler;
    }

    Buffer += MchiDataSize;

    // Set Number of Protocol Records Field.
    *Buffer = (UINT8)ProtocolCount;
    Buffer += sizeof (UINT8);

    ProtocolIdx = 0;
    for (Entry = GetFirstNode (&mMchiProtocolList);
         Entry != &mMchiProtocolList;
         Entry = GetNextNode (&mMchiProtocolList, Entry))
    {
      ProtocolDataInfo = BASE_CR (Entry, MCHI_PROTOCOL_DATA_INFO, Entry);
      ProtocolOps      = GetMchiProtocolOps (ProtocolDataInfo->ProtocolType);
      ASSERT (ProtocolOps != NULL);

      // Set ProtocolType.
      *Buffer = ProtocolDataInfo->ProtocolType;
      Buffer += sizeof (UINT8);

      // Set Protocol Data Size.
      *Buffer = (UINT8)ProtocolDataInfo->DataSize;
      Buffer += sizeof (UINT8);

      Status = ProtocolOps->AddMchiProtocolSpecificData (
                              CfgMgrProtocol,
                              ProtocolDataInfo,
                              &StrTable,
                              Buffer
                              );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to add MCHI Protocol data into SmbiosRecord Status=%r.\n",
          __func__,
          Status
          ));
        StringTableFree (&StrTable);
        FreePool (SmbiosRecord);
        goto ErrorHandler;
      }

      Buffer += ProtocolDataInfo->DataSize;
      ProtocolIdx++;
    }

    ASSERT (ProtocolIdx == ProtocolCount);

    Buffer = AllocateSmbiosRecord (SmbiosRecordSize, &StrTable);
    if (Buffer == NULL) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to allocate SmbiosRecord with string table.\n",
        __func__
        ));
      Status = EFI_OUT_OF_RESOURCES;
      StringTableFree (&StrTable);
      FreePool (SmbiosRecord);
      goto ErrorHandler;
    }

    CopyMem (Buffer, SmbiosRecord, SmbiosRecordSize);
    FreePool (SmbiosRecord);
    SmbiosRecord = (SMBIOS_TABLE_TYPE42 *)Buffer;
    Buffer      += SmbiosRecordSize;

    Status = StringTablePublishStringSet (
               &StrTable,
               (CHAR8 *)Buffer,
               StringTableGetStringSetSize (&StrTable)
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to publish string table. Status=%r.\n",
        __func__,
        Status
        ));
      StringTableFree (&StrTable);
      FreePool (SmbiosRecord);
      goto ErrorHandler;
    }

    CleanupMchiProtocolList ();
    StringTableFree (&StrTable);

    TableList[InterfaceIdx] = (SMBIOS_STRUCTURE *)SmbiosRecord;
  }

  ASSERT (InterfaceIdx == MchiCount);

  *Table         = TableList;
  *CmObjectToken = NULL;
  *TableCount    = MchiCount;

  return EFI_SUCCESS;

ErrorHandler:
  CleanupMchiProtocolList ();

  if (TableList != NULL) {
    while (InterfaceIdx-- != 0) {
      if (TableList[InterfaceIdx] != NULL) {
        FreePool (TableList[InterfaceIdx]);
      }
    }

    FreePool (TableList);
  }

  return Status;
}

/** The interface for the SMBIOS Type4 Table Generator.
*/
STATIC
CONST
SMBIOS_TABLE_GENERATOR  SmbiosType42Generator = {
  // Generator ID
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType42),
  // Generator Description
  L"SMBIOS.TYPE42.GENERATOR",
  // SMBIOS Table Type
  EFI_SMBIOS_TYPE_MANAGEMENT_CONTROLLER_HOST_INTERFACE,
  NULL,
  NULL,
  // Build table function.
  BuildSmbiosType42TableEx,
  // Free function.
  FreeSmbiosType42TableEx,
};

/** Register the Generator with the SMBIOS Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is registered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID
                                is already registered.
**/
EFI_STATUS
EFIAPI
SmbiosType42LibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType42Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 42: Register Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Deregister the Generator from the SMBIOS Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is deregistered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The Generator is not registered.
**/
EFI_STATUS
EFIAPI
SmbiosType42LibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType42Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 42: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
