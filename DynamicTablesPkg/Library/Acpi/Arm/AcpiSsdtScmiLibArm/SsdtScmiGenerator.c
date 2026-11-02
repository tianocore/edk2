/** @file
  SSDT SCMI AML Table Generator.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI for System Control and Management Interface
    (https://support.arm.com/documentation/111115/latest)
**/

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include <AcpiTableGenerator.h>
#include <ConfigurationManagerHelper.h>
#include <Library/AcpiHelperLib.h>
#include <Library/AmlLib/AmlLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include "SsdtScmiGenerator.h"

/** SCMI Device Properties UUID. */
STATIC CONST EFI_GUID  mScmiDevicePropertiesUuid = {
  0x84a2d1c6,
  0x86b6,
  0x4199,
  { 0x8d,    0xac,0x9c, 0x17, 0x44, 0x9d, 0x5e, 0x03 }
};

/** Describes how a PCC transport is used by an SCMI device. */
typedef struct ScmiPccTransportUsage {
  CM_OBJECT_TOKEN    TransportToken;
  CM_OBJECT_TOKEN    PccSubspaceToken;
  UINT32             DeviceIndex;
  UINT32             ProtocolIndex;
  UINT64             TransportUid;
  UINT8              PccSubspaceId;
  BOOLEAN            IsCommon;
  BOOLEAN            IsP2a;
} SCMI_PCC_TRANSPORT_USAGE;

/** Retrieve the SCMI device information objects. */
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjScmiDeviceInfo,
  CM_ARM_SCMI_DEVICE_INFO
  );

/** Retrieve the SCMI PCC transport information objects. */
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjScmiPccTransportInfo,
  CM_ARM_SCMI_PCC_TRANSPORT_INFO
  );

/** Retrieve the SCMI protocol information objects. */
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjScmiProtocolInfo,
  CM_ARM_SCMI_PROTOCOL_INFO
  );

/** Retrieve PCC Subspace Type 3 information. */
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjPccSubspaceType3Info,
  CM_ARCH_COMMON_PCC_SUBSPACE_TYPE3_INFO
  );

/** Retrieve PCC Subspace Type 4 information. */
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjPccSubspaceType4Info,
  CM_ARCH_COMMON_PCC_SUBSPACE_TYPE4_INFO
  );

/**
  Resolve a PCC subspace token to its PCCT subspace ID.

  A2P transports use PCC Type 3 and P2A transports use PCC Type 4.

  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager protocol.
  @param [in]  PccToken        Token identifying the PCC subspace.
  @param [in]  IsP2a           TRUE for a P2A transport.
  @param [out] PccSubspaceId   PCCT subspace ID associated with the token.

  @retval EFI_SUCCESS            The PCC subspace was resolved.
  @retval EFI_INVALID_PARAMETER  A parameter or CM object is invalid.
  @retval EFI_NOT_FOUND          The PCC subspace was not found.
**/
STATIC
EFI_STATUS
GetPccSubspaceId (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST   CfgMgrProtocol,
  IN  CONST CM_OBJECT_TOKEN                                PccToken,
  IN  CONST BOOLEAN                                        IsP2a,
  OUT       UINT8                                  *CONST  PccSubspaceId
  )
{
  EFI_STATUS                              Status;
  CM_ARCH_COMMON_PCC_SUBSPACE_TYPE3_INFO  *PccType3Info;
  CM_ARCH_COMMON_PCC_SUBSPACE_TYPE4_INFO  *PccType4Info;
  PCC_SUBSPACE_GENERIC_INFO               *PccInfo;
  UINT32                                  PccCount;
  UINT8                                   ExpectedPccType;

  if ((CfgMgrProtocol == NULL)    ||
      (PccToken == CM_NULL_TOKEN) ||
      (PccSubspaceId == NULL))
  {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  PccType3Info    = NULL;
  PccType4Info    = NULL;
  PccInfo         = NULL;
  PccCount        = 0;
  ExpectedPccType = EFI_ACPI_6_7_PCCT_SUBSPACE_TYPE_3_EXTENDED_PCC;

  if (IsP2a) {
    ExpectedPccType = EFI_ACPI_6_7_PCCT_SUBSPACE_TYPE_4_EXTENDED_PCC;
    Status          = GetEArchCommonObjPccSubspaceType4Info (
                        CfgMgrProtocol,
                        PccToken,
                        &PccType4Info,
                        &PccCount
                        );
    PccInfo = (PCC_SUBSPACE_GENERIC_INFO *)PccType4Info;
  } else {
    Status = GetEArchCommonObjPccSubspaceType3Info (
               CfgMgrProtocol,
               PccToken,
               &PccType3Info,
               &PccCount
               );
    PccInfo = (PCC_SUBSPACE_GENERIC_INFO *)PccType3Info;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SCMI: Failed to resolve a PCC subspace."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  if ((PccInfo == NULL) || (PccCount != 1)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SCMI: Invalid PCC subspace object.\n"
      ));
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  if (PccInfo->Type != ExpectedPccType) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SCMI: PCC subspace type does not match the"
      " transport direction.\n"
      ));
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  *PccSubspaceId = PccInfo->SubspaceId;
  return EFI_SUCCESS;
}

/**
  Add and validate an SCMI PCC transport usage.

  @param [in]      CfgMgrProtocol  Configuration Manager protocol.
  @param [in]      TransportToken  SCMI transport token.
  @param [in]      DeviceIndex     Index of the owning SCMI device.
  @param [in]      ProtocolIndex   Index of the owning protocol, or
                                   SCMI_COMMON_PROTOCOL_INDEX.
  @param [in]      IsCommon        TRUE for a common transport.
  @param [in]      IsP2a           TRUE for a P2A transport.
  @param [in, out] TransportUsage  Transport usage array.
  @param [in, out] UsageCount      Number of entries in TransportUsage.

  @retval EFI_SUCCESS            The transport usage was added.
  @retval EFI_INVALID_PARAMETER  The transport configuration is invalid.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
  @retval other                  Failed to retrieve a referenced CM object.
**/
STATIC
EFI_STATUS
AddScmiTransportUsage (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST    CfgMgrProtocol,
  IN      CONST CM_OBJECT_TOKEN                                 TransportToken,
  IN      CONST UINT32                                          DeviceIndex,
  IN      CONST UINT32                                          ProtocolIndex,
  IN      CONST BOOLEAN                                         IsCommon,
  IN      CONST BOOLEAN                                         IsP2a,
  IN OUT        SCMI_PCC_TRANSPORT_USAGE               **CONST  TransportUsage,
  IN OUT        UINTN                                  *CONST   UsageCount
  )
{
  EFI_STATUS                      Status;
  CM_ARM_SCMI_PCC_TRANSPORT_INFO  *TransportInfo;
  SCMI_PCC_TRANSPORT_USAGE        *NewTransportUsage;
  UINT32                          TransportCount;
  UINT64                          TransportUid;
  UINT8                           PccSubspaceId;
  UINTN                           Index;
  UINTN                           OldSize;
  UINTN                           NewSize;

  if ((CfgMgrProtocol == NULL)        ||
      (TransportToken == CM_NULL_TOKEN) ||
      (TransportUsage == NULL)        ||
      (UsageCount == NULL))
  {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Status = GetEArmObjScmiPccTransportInfo (
             CfgMgrProtocol,
             TransportToken,
             &TransportInfo,
             &TransportCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SCMI: Failed to get an SCMI PCC transport."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  if ((TransportInfo == NULL) || (TransportCount != 1)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SCMI: Transport token must resolve to one object.\n"
      ));
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  if (TransportInfo->PccSubspaceToken == CM_NULL_TOKEN) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SCMI: Transport has no PCC subspace token.\n"
      ));
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Status = GetPccSubspaceId (
             CfgMgrProtocol,
             TransportInfo->PccSubspaceToken,
             IsP2a,
             &PccSubspaceId
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TransportUid = 0;

  for (Index = 0; Index < *UsageCount; Index++) {
    if ((*TransportUsage)[Index].DeviceIndex == DeviceIndex) {
      TransportUid++;
    }

    if ((*TransportUsage)[Index].PccSubspaceId != PccSubspaceId) {
      continue;
    }

    if ((*TransportUsage)[Index].DeviceIndex != DeviceIndex) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SCMI: PCC subspace %u is used by multiple"
        " SCMI devices.\n",
        PccSubspaceId
        ));
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
    }

    if ((*TransportUsage)[Index].IsP2a != IsP2a) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SCMI: PCC subspace %u is used as both an A2P"
        " and P2A transport.\n",
        PccSubspaceId
        ));
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
    }

    if ((*TransportUsage)[Index].IsCommon != IsCommon) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SCMI: PCC subspace %u is both common and"
        " protocol-exclusive.\n",
        PccSubspaceId
        ));
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
    }

    if (!IsCommon &&
        ((*TransportUsage)[Index].ProtocolIndex != ProtocolIndex))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SCMI: Exclusive PCC subspace %u is shared by"
        " multiple protocols.\n",
        PccSubspaceId
        ));
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
    }
  }

  if (*UsageCount >= (MAX_UINTN / sizeof (**TransportUsage))) {
    return EFI_OUT_OF_RESOURCES;
  }

  OldSize = sizeof (**TransportUsage) * (*UsageCount);
  NewSize = sizeof (**TransportUsage) * (*UsageCount + 1);

  NewTransportUsage = ReallocatePool (
                        OldSize,
                        NewSize,
                        *TransportUsage
                        );
  if (NewTransportUsage == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewTransportUsage[*UsageCount].TransportToken   = TransportToken;
  NewTransportUsage[*UsageCount].PccSubspaceToken =
    TransportInfo->PccSubspaceToken;
  NewTransportUsage[*UsageCount].DeviceIndex   = DeviceIndex;
  NewTransportUsage[*UsageCount].ProtocolIndex = ProtocolIndex;
  NewTransportUsage[*UsageCount].TransportUid  = TransportUid;
  NewTransportUsage[*UsageCount].PccSubspaceId = PccSubspaceId;
  NewTransportUsage[*UsageCount].IsCommon      = IsCommon;
  NewTransportUsage[*UsageCount].IsP2a         = IsP2a;

  *TransportUsage = NewTransportUsage;
  (*UsageCount)++;

  return EFI_SUCCESS;
}

/**
  Validate the SCMI device, protocol and transport relationships.

  @param [in]  CfgMgrProtocol  Configuration Manager protocol.
  @param [in]  ScmiDeviceInfo  SCMI device information array.
  @param [in]  DeviceCount     Number of SCMI devices.
  @param [out] TransportUsage  Validated transport usage array.
  @param [out] UsageCount      Number of entries in TransportUsage.

  @retval EFI_SUCCESS            The SCMI configuration is valid.
  @retval EFI_INVALID_PARAMETER  The SCMI configuration is invalid.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
  @retval other                  Failed to retrieve a referenced CM object.
**/
STATIC
EFI_STATUS
ValidateScmiConfiguration (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST    CfgMgrProtocol,
  IN  CONST CM_ARM_SCMI_DEVICE_INFO                *CONST   ScmiDeviceInfo,
  IN  CONST UINT32                                          DeviceCount,
  OUT       SCMI_PCC_TRANSPORT_USAGE               **CONST  TransportUsage,
  OUT       UINTN                                  *CONST   UsageCount
  )
{
  EFI_STATUS                 Status;
  CM_ARM_SCMI_PROTOCOL_INFO  *ProtocolInfo;
  SCMI_PCC_TRANSPORT_USAGE   *UsageList;
  UINT32                     ProtocolCount;
  UINT32                     DeviceIndex;
  UINT32                     ProtocolIndex;
  UINT32                     PreviousProtocolIndex;
  UINT32                     TransportDirection;
  UINTN                      LocalUsageCount;

  if ((CfgMgrProtocol == NULL) ||
      (ScmiDeviceInfo == NULL) ||
      (DeviceCount == 0)       ||
      (TransportUsage == NULL) ||
      (UsageCount == NULL))
  {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  UsageList       = NULL;
  LocalUsageCount = 0;
  *TransportUsage = NULL;
  *UsageCount     = 0;

  for (DeviceIndex = 0; DeviceIndex < DeviceCount; DeviceIndex++) {
    if (ScmiDeviceInfo[DeviceIndex]
          .CommonTransportToken[ArmScmiTransportDirectionA2p] ==
        CM_NULL_TOKEN)
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SCMI: Device %u has no common A2P transport.\n",
        DeviceIndex
        ));
      Status = EFI_INVALID_PARAMETER;
      ASSERT (FALSE);
      goto error_handler;
    }

    for (TransportDirection = ArmScmiTransportDirectionA2p;
         TransportDirection < ArmScmiTransportDirectionMax;
         TransportDirection++)
    {
      if (ScmiDeviceInfo[DeviceIndex]
            .CommonTransportToken[TransportDirection] == CM_NULL_TOKEN)
      {
        continue;
      }

      Status = AddScmiTransportUsage (
                 CfgMgrProtocol,
                 ScmiDeviceInfo[DeviceIndex]
                   .CommonTransportToken[TransportDirection],
                 DeviceIndex,
                 SCMI_COMMON_PROTOCOL_INDEX,
                 TRUE,
                 TransportDirection == ArmScmiTransportDirectionP2a,
                 &UsageList,
                 &LocalUsageCount
                 );
      if (EFI_ERROR (Status)) {
        goto error_handler;
      }
    }

    if (ScmiDeviceInfo[DeviceIndex].ProtocolInfoToken == CM_NULL_TOKEN) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SCMI: Device %u has no protocol list token.\n",
        DeviceIndex
        ));
      Status = EFI_INVALID_PARAMETER;
      ASSERT (FALSE);
      goto error_handler;
    }

    Status = GetEArmObjScmiProtocolInfo (
               CfgMgrProtocol,
               ScmiDeviceInfo[DeviceIndex].ProtocolInfoToken,
               &ProtocolInfo,
               &ProtocolCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SCMI: Failed to get protocols for device %u."
        " Status = %r\n",
        DeviceIndex,
        Status
        ));
      goto error_handler;
    }

    if ((ProtocolInfo == NULL) || (ProtocolCount == 0)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SCMI: Device %u must expose at least one"
        " protocol.\n",
        DeviceIndex
        ));
      Status = EFI_INVALID_PARAMETER;
      ASSERT (FALSE);
      goto error_handler;
    }

    for (ProtocolIndex = 0;
         ProtocolIndex < ProtocolCount;
         ProtocolIndex++)
    {
      if ((UINT32)ProtocolInfo[ProtocolIndex].ProtocolType >=
          (UINT32)ArmScmiProtocolTypeMax)
      {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SSDT-SCMI: Device %u has an unsupported protocol.\n",
          DeviceIndex
          ));
        Status = EFI_INVALID_PARAMETER;
        ASSERT (FALSE);
        goto error_handler;
      }

      for (PreviousProtocolIndex = 0;
           PreviousProtocolIndex < ProtocolIndex;
           PreviousProtocolIndex++)
      {
        if (ProtocolInfo[PreviousProtocolIndex].ProtocolType ==
            ProtocolInfo[ProtocolIndex].ProtocolType)
        {
          DEBUG ((
            DEBUG_ERROR,
            "ERROR: SSDT-SCMI: Device %u contains a duplicate"
            " protocol property.\n",
            DeviceIndex
            ));
          Status = EFI_INVALID_PARAMETER;
          ASSERT (FALSE);
          goto error_handler;
        }
      }

      for (TransportDirection = ArmScmiTransportDirectionA2p;
           TransportDirection < ArmScmiTransportDirectionMax;
           TransportDirection++)
      {
        if (ProtocolInfo[ProtocolIndex]
              .ExclusiveTransportToken[TransportDirection] == CM_NULL_TOKEN)
        {
          continue;
        }

        Status = AddScmiTransportUsage (
                   CfgMgrProtocol,
                   ProtocolInfo[ProtocolIndex]
                     .ExclusiveTransportToken[TransportDirection],
                   DeviceIndex,
                   ProtocolIndex,
                   FALSE,
                   TransportDirection == ArmScmiTransportDirectionP2a,
                   &UsageList,
                   &LocalUsageCount
                   );
        if (EFI_ERROR (Status)) {
          goto error_handler;
        }
      }

      if (ProtocolInfo[ProtocolIndex].NotificationsSupported &&
          (ScmiDeviceInfo[DeviceIndex]
             .CommonTransportToken[ArmScmiTransportDirectionP2a] ==
           CM_NULL_TOKEN) &&
          (ProtocolInfo[ProtocolIndex]
             .ExclusiveTransportToken[ArmScmiTransportDirectionP2a] ==
           CM_NULL_TOKEN))
      {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SSDT-SCMI: Protocol %u on device %u supports"
          " notifications but has no P2A transport.\n",
          ProtocolIndex,
          DeviceIndex
          ));
        Status = EFI_INVALID_PARAMETER;
        ASSERT (FALSE);
        goto error_handler;
      }
    }
  }

  *TransportUsage = UsageList;
  *UsageCount     = LocalUsageCount;

  return EFI_SUCCESS;

error_handler:
  if (UsageList != NULL) {
    FreePool (UsageList);
  }

  return Status;
}

/**
  Generate the SCMI PCC transport property for an SCMI device.

  @param [in] DeviceIndex      Index of the SCMI device.
  @param [in] TransportUsage   Validated transport usage array.
  @param [in] UsageCount       Number of transport usages.
  @param [in] DeviceProperties SCMI Device Properties Package.

  @retval EFI_SUCCESS            The transport property was generated.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
**/
STATIC
EFI_STATUS
BuildScmiTransportProperty (
  IN UINT32                          DeviceIndex,
  IN CONST SCMI_PCC_TRANSPORT_USAGE  *TransportUsage,
  IN UINTN                           UsageCount,
  IN AML_OBJECT_NODE_HANDLE          DeviceProperties
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  TransportList;
  UINT64                  Flags;
  UINT64                  TransportCount;
  UINTN                   Index;

  if ((TransportUsage == NULL) ||
      (UsageCount == 0)         ||
      (DeviceProperties == NULL))
  {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  TransportCount = 0;
  for (Index = 0; Index < UsageCount; Index++) {
    if (TransportUsage[Index].DeviceIndex == DeviceIndex) {
      TransportCount++;
    }
  }

  if (TransportCount == 0) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlCodeGenScmiPccTransportProperty (
             SCMI_DEVICE_PROPERTIES_REVISION,
             TransportCount,
             DeviceProperties,
             &TransportList
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < UsageCount; Index++) {
    if (TransportUsage[Index].DeviceIndex != DeviceIndex) {
      continue;
    }

    Flags = 0;
    if (TransportUsage[Index].IsCommon) {
      Flags |= SCMI_PCC_TRANSPORT_FLAG_COMMON;
    }

    if (TransportUsage[Index].IsP2a) {
      Flags |= SCMI_PCC_TRANSPORT_FLAG_P2A;
    }

    Status = AmlAddScmiPccTransportDescriptor (
               TransportUsage[Index].PccSubspaceId,
               TransportUsage[Index].TransportUid,
               Flags,
               TransportList
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Generate an SCMI protocol property.

  @param [in] DeviceIndex      Index of the SCMI device.
  @param [in] ProtocolIndex    Index of the protocol within the device.
  @param [in] ProtocolType     SCMI protocol type.
  @param [in] TransportUsage   Validated transport usage array.
  @param [in] UsageCount       Number of transport usages.
  @param [in] DeviceProperties SCMI Device Properties Package.

  @retval EFI_SUCCESS            The protocol property was generated.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
**/
STATIC
EFI_STATUS
BuildScmiProtocolProperty (
  IN UINT32                          DeviceIndex,
  IN UINT32                          ProtocolIndex,
  IN CM_ARM_SCMI_PROTOCOL_TYPE       ProtocolType,
  IN CONST SCMI_PCC_TRANSPORT_USAGE  *TransportUsage,
  IN UINTN                           UsageCount,
  IN AML_OBJECT_NODE_HANDLE          DeviceProperties
  )
{
  EFI_STATUS              Status;
  CONST CHAR8             *PropertyName;
  AML_OBJECT_NODE_HANDLE  ExclusiveTransports;
  UINTN                   Index;

  if ((TransportUsage == NULL) ||
      (UsageCount == 0)         ||
      (DeviceProperties == NULL))
  {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  switch (ProtocolType) {
    case ArmScmiProtocolTypePcap:
      PropertyName = SCMI_PCAP_PROTOCOL_PROPERTY;
      break;

    case ArmScmiProtocolTypeSystemTelemetry:
      PropertyName = SCMI_TELEMETRY_PROTOCOL_PROPERTY;
      break;

    default:
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
  }

  Status = AmlCodeGenScmiProtocolProperty (
             PropertyName,
             SCMI_DEVICE_PROPERTIES_REVISION,
             DeviceProperties,
             &ExclusiveTransports
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < UsageCount; Index++) {
    if ((TransportUsage[Index].DeviceIndex != DeviceIndex) ||
        TransportUsage[Index].IsCommon                     ||
        (TransportUsage[Index].ProtocolIndex != ProtocolIndex))
    {
      continue;
    }

    Status = AmlAddScmiProtocolTransportDescriptor (
               TransportUsage[Index].TransportUid,
               0,
               ExclusiveTransports
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Generate one SCMI ACPI device.

  @param [in] CfgMgrProtocol  Configuration Manager protocol.
  @param [in] ScmiDeviceInfo  SCMI device information.
  @param [in] DeviceIndex     Index of the SCMI device.
  @param [in] TransportUsage  Validated transport usage array.
  @param [in] UsageCount      Number of transport usages.
  @param [in] ScopeNode       AML \_SB scope node.

  @retval EFI_SUCCESS            The SCMI device was generated.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
  @retval other                  Failed to retrieve protocol information.
**/
STATIC
EFI_STATUS
BuildScmiDevice (
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST   CfgMgrProtocol,
  IN CONST CM_ARM_SCMI_DEVICE_INFO                *CONST  ScmiDeviceInfo,
  IN UINT32                                               DeviceIndex,
  IN CONST SCMI_PCC_TRANSPORT_USAGE               *CONST  TransportUsage,
  IN UINTN                                                UsageCount,
  IN AML_OBJECT_NODE_HANDLE                               ScopeNode
  )
{
  EFI_STATUS                 Status;
  CM_ARM_SCMI_PROTOCOL_INFO  *ProtocolInfo;
  AML_OBJECT_NODE_HANDLE     DeviceNode;
  AML_OBJECT_NODE_HANDLE     DsdNode;
  AML_OBJECT_NODE_HANDLE     DeviceProperties;
  CHAR8                      DeviceName[AML_NAME_SEG_SIZE + 1];
  UINT32                     ProtocolCount;
  UINT32                     ProtocolIndex;

  if ((CfgMgrProtocol == NULL) ||
      (ScmiDeviceInfo == NULL) ||
      (TransportUsage == NULL) ||
      (UsageCount == 0)        ||
      (ScopeNode == NULL)      ||
      (DeviceIndex > MAX_UINT8))
  {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  DeviceName[0] = 'S';
  DeviceName[1] = 'C';
  DeviceName[2] = AsciiFromHex ((DeviceIndex >> 4) & 0xF);
  DeviceName[3] = AsciiFromHex (DeviceIndex & 0xF);
  DeviceName[4] = '\0';

  Status = AmlCodeGenDevice (DeviceName, ScopeNode, &DeviceNode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AmlCodeGenNameString (
             "_HID",
             SCMI_DEVICE_HID,
             DeviceNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AmlCodeGenNameInteger (
             "_UID",
             DeviceIndex,
             DeviceNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AmlCodeGenNamePackage (
             "_DSD",
             DeviceNode,
             &DsdNode
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AmlAddDeviceDataDescriptorPackage (
             &mScmiDevicePropertiesUuid,
             DsdNode,
             &DeviceProperties
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = BuildScmiTransportProperty (
             DeviceIndex,
             TransportUsage,
             UsageCount,
             DeviceProperties
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetEArmObjScmiProtocolInfo (
             CfgMgrProtocol,
             ScmiDeviceInfo->ProtocolInfoToken,
             &ProtocolInfo,
             &ProtocolCount
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (ProtocolIndex = 0;
       ProtocolIndex < ProtocolCount;
       ProtocolIndex++)
  {
    Status = BuildScmiProtocolProperty (
               DeviceIndex,
               ProtocolIndex,
               ProtocolInfo[ProtocolIndex].ProtocolType,
               TransportUsage,
               UsageCount,
               DeviceProperties
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Build the SSDT containing the SCMI devices.

  @param [in]  This             Pointer to the table generator.
  @param [in]  AcpiTableInfo    Pointer to the ACPI table information.
  @param [in]  CfgMgrProtocol   Pointer to the Configuration Manager protocol.
  @param [out] Table            Pointer to the generated ACPI table.

  @retval EFI_SUCCESS            The table was generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
  @retval other                  Failed to retrieve CM objects or generate AML.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSsdtScmiTable (
  IN  CONST ACPI_TABLE_GENERATOR                           *This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                    **Table
  )
{
  EFI_STATUS                Status;
  EFI_STATUS                CleanupStatus;
  CM_ARM_SCMI_DEVICE_INFO   *ScmiDeviceInfo;
  SCMI_PCC_TRANSPORT_USAGE  *TransportUsage;
  AML_ROOT_NODE_HANDLE      RootNode;
  AML_OBJECT_NODE_HANDLE    ScopeNode;
  UINT32                    DeviceCount;
  UINT32                    DeviceIndex;
  UINTN                     UsageCount;

  if ((This == NULL)                                             ||
      (AcpiTableInfo == NULL)                                    ||
      (CfgMgrProtocol == NULL)                                   ||
      (Table == NULL)                                            ||
      (AcpiTableInfo->TableGeneratorId != This->GeneratorID)     ||
      (AcpiTableInfo->AcpiTableSignature != This->AcpiTableSignature))
  {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  *Table         = NULL;
  RootNode       = NULL;
  TransportUsage = NULL;
  UsageCount     = 0;

  Status = GetEArmObjScmiDeviceInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &ScmiDeviceInfo,
             &DeviceCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SCMI: Failed to get SCMI device information."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  if ((ScmiDeviceInfo == NULL) || (DeviceCount == 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SCMI: No SCMI devices were provided.\n"
      ));
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Status = ValidateScmiConfiguration (
             CfgMgrProtocol,
             ScmiDeviceInfo,
             DeviceCount,
             &TransportUsage,
             &UsageCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SCMI: Invalid SCMI configuration."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = AmlCodeGenDefinitionBlock (
             "SSDT",
             "ARMLTD",
             "SCMI",
             1,
             &RootNode
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SCMI: Failed to create the Definition Block."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  Status = AmlCodeGenScope ("\\_SB_", RootNode, &ScopeNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SCMI: Failed to create the _SB scope."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  for (DeviceIndex = 0; DeviceIndex < DeviceCount; DeviceIndex++) {
    Status = BuildScmiDevice (
               CfgMgrProtocol,
               &ScmiDeviceInfo[DeviceIndex],
               DeviceIndex,
               TransportUsage,
               UsageCount,
               ScopeNode
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SCMI: Failed to build SCMI device %u."
        " Status = %r\n",
        DeviceIndex,
        Status
        ));
      goto exit_handler;
    }
  }

  Status = AmlSerializeDefinitionBlock (RootNode, Table);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SCMI: Failed to serialize the SSDT."
      " Status = %r\n",
      Status
      ));
  }

exit_handler:
  if (RootNode != NULL) {
    CleanupStatus = AmlDeleteTree (RootNode);
    if (EFI_ERROR (CleanupStatus)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SCMI: Failed to delete the AML tree."
        " Status = %r\n",
        CleanupStatus
        ));

      if (!EFI_ERROR (Status)) {
        if (*Table != NULL) {
          FreePool (*Table);
          *Table = NULL;
        }

        Status = CleanupStatus;
      }
    }
  }

  if (TransportUsage != NULL) {
    FreePool (TransportUsage);
  }

  return Status;
}

/**
  Free resources allocated by the SCMI SSDT generator.

  @param [in]      This            Pointer to the table generator.
  @param [in]      AcpiTableInfo   Pointer to the ACPI table information.
  @param [in]      CfgMgrProtocol  Pointer to the Configuration Manager protocol.
  @param [in, out] Table           Pointer to the generated ACPI table.

  @retval EFI_SUCCESS            The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSsdtScmiTableResources (
  IN      CONST ACPI_TABLE_GENERATOR                           *CONST   This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST           AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST           CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER                    **CONST  Table
  )
{
  if ((This == NULL)                                             ||
      (AcpiTableInfo == NULL)                                    ||
      (CfgMgrProtocol == NULL)                                   ||
      (Table == NULL)                                            ||
      (*Table == NULL)                                           ||
      (AcpiTableInfo->TableGeneratorId != This->GeneratorID)     ||
      (AcpiTableInfo->AcpiTableSignature != This->AcpiTableSignature))
  {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;

  return EFI_SUCCESS;
}

/** SCMI SSDT table generator. */
STATIC
CONST
ACPI_TABLE_GENERATOR  SsdtScmiGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtScmi),
  // Generator Description
  L"ACPI.STD.SSDT.SCMI.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_6_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
  // ACPI Table Revision
  0,
  // Minimum supported ACPI Table Revision
  0,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_ARM,
  // Creator Revision
  SSDT_SCMI_GENERATOR_REVISION,
  // Build Table function
  BuildSsdtScmiTable,
  // Free Resource function
  FreeSsdtScmiTableResources,
  // Extended Build Table function
  NULL,
  // Extended Free Resource function
  NULL
};

/**
  Register the SCMI SSDT generator.

  @param [in] ImageHandle  The image handle.
  @param [in] SystemTable  Pointer to the EFI system table.

  @retval EFI_SUCCESS  The generator was registered successfully.
  @retval other        The generator registration failed.
**/
EFI_STATUS
EFIAPI
AcpiSsdtScmiConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&SsdtScmiGenerator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-SCMI: Register generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Deregister the SCMI SSDT generator.

  @param [in] ImageHandle  The image handle.
  @param [in] SystemTable  Pointer to the EFI system table.

  @retval EFI_SUCCESS  The generator was deregistered successfully.
  @retval other        The generator deregistration failed.
**/
EFI_STATUS
EFIAPI
AcpiSsdtScmiDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&SsdtScmiGenerator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-SCMI: Deregister generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);

  return Status;
}
