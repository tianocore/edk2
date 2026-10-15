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

/** SCMI SSDT generator revision. */
#define SSDT_SCMI_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** SCMI Device Properties package revision. */
#define SCMI_DEVICE_PROPERTIES_REVISION  1

/** SCMI ACPI device hardware identifier. */
#define SCMI_DEVICE_HID  "ARML0001"

/** SCMI PCC transport property key. */
#define SCMI_PCC_TRANSPORT_PROPERTY  "arm-arml0001-transport-pcc"

/** SCMI power capping and monitoring protocol property key. */
#define SCMI_PCAP_PROTOCOL_PROPERTY  "arm-arml0001-protocol-pcap"

/** SCMI system telemetry protocol property key. */
#define SCMI_TELEMETRY_PROTOCOL_PROPERTY  \
  "arm-arml0001-protocol-telemetry"

/** SCMI PCC transport descriptor flags. */
#define SCMI_PCC_TRANSPORT_FLAG_COMMON  BIT0
#define SCMI_PCC_TRANSPORT_FLAG_P2A     BIT1

/** Indicates that a transport is common to all protocols. */
#define SCMI_COMMON_PROTOCOL_INDEX  MAX_UINT32

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

/**
  Resolve a PCC subspace token to its PCCT subspace ID.

  The ACPI for SCMI specification recommends PCC Type 3 for A2P and
  PCC Type 4 for P2A, but does not require those types. Therefore, all
  PCC subspace types are accepted here.

  @param [in]  CfgMgrProtocol     Pointer to the Configuration Manager
                                  protocol.
  @param [in]  PccSubspaceObjectId
                                  Object ID identifying the PCC subspace type.
  @param [in]  PccToken           Token identifying the PCC subspace.
  @param [out] PccSubspaceId      PCCT subspace ID associated with the token.

  @retval EFI_SUCCESS            The PCC subspace was resolved.
  @retval EFI_INVALID_PARAMETER  A parameter or CM object is invalid.
  @retval EFI_NOT_FOUND          The PCC subspace was not found.
**/
STATIC
EFI_STATUS
GetPccSubspaceId (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST   CfgMgrProtocol,
  IN  CONST CM_OBJECT_ID                                   PccSubspaceObjectId,
  IN  CONST CM_OBJECT_TOKEN                                PccToken,
  OUT       UINT8                                  *CONST  PccSubspaceId
  )
{
  EFI_STATUS                 Status;
  CM_OBJ_DESCRIPTOR          CmObjectDesc;
  PCC_SUBSPACE_GENERIC_INFO  *PccInfo;
  UINT32                     PccObjectId;

  PccObjectId = GET_CM_OBJECT_ID (PccSubspaceObjectId);

  if ((CfgMgrProtocol == NULL)                                    ||
      (PccToken == CM_NULL_TOKEN)                                 ||
      (PccSubspaceId == NULL)                                     ||
      (GET_CM_NAMESPACE_ID (PccSubspaceObjectId) !=
       EObjNameSpaceArchCommon)                                   ||
      (PccObjectId < EArchCommonObjPccSubspaceType0Info)          ||
      (PccObjectId > EArchCommonObjPccSubspaceType5Info))
  {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Status = CfgMgrProtocol->GetObject (
                             CfgMgrProtocol,
                             PccSubspaceObjectId,
                             PccToken,
                             &CmObjectDesc
                             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SCMI: Failed to resolve a PCC subspace."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  if ((CmObjectDesc.ObjectId != PccSubspaceObjectId)          ||
      (CmObjectDesc.Data == NULL)                             ||
      (CmObjectDesc.Count != 1)                               ||
      (CmObjectDesc.Size < sizeof (PCC_SUBSPACE_GENERIC_INFO)))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SCMI: Invalid PCC subspace object.\n"
      ));
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  PccInfo = (PCC_SUBSPACE_GENERIC_INFO *)CmObjectDesc.Data;
  if (PccObjectId !=
      (EArchCommonObjPccSubspaceType0Info + PccInfo->Type))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SCMI: PCC subspace type does not match its object ID.\n"
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
             TransportInfo->PccSubspaceObjectId,
             TransportInfo->PccSubspaceToken,
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
    if (ScmiDeviceInfo[DeviceIndex].CommonA2pTransportToken ==
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

    Status = AddScmiTransportUsage (
               CfgMgrProtocol,
               ScmiDeviceInfo[DeviceIndex].CommonA2pTransportToken,
               DeviceIndex,
               SCMI_COMMON_PROTOCOL_INDEX,
               TRUE,
               FALSE,
               &UsageList,
               &LocalUsageCount
               );
    if (EFI_ERROR (Status)) {
      goto error_handler;
    }

    if (ScmiDeviceInfo[DeviceIndex].CommonP2aTransportToken !=
        CM_NULL_TOKEN)
    {
      Status = AddScmiTransportUsage (
                 CfgMgrProtocol,
                 ScmiDeviceInfo[DeviceIndex].CommonP2aTransportToken,
                 DeviceIndex,
                 SCMI_COMMON_PROTOCOL_INDEX,
                 TRUE,
                 TRUE,
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

      if (ProtocolInfo[ProtocolIndex].ExclusiveA2pTransportToken !=
          CM_NULL_TOKEN)
      {
        Status = AddScmiTransportUsage (
                   CfgMgrProtocol,
                   ProtocolInfo[ProtocolIndex].ExclusiveA2pTransportToken,
                   DeviceIndex,
                   ProtocolIndex,
                   FALSE,
                   FALSE,
                   &UsageList,
                   &LocalUsageCount
                   );
        if (EFI_ERROR (Status)) {
          goto error_handler;
        }
      }

      if (ProtocolInfo[ProtocolIndex].ExclusiveP2aTransportToken !=
          CM_NULL_TOKEN)
      {
        Status = AddScmiTransportUsage (
                   CfgMgrProtocol,
                   ProtocolInfo[ProtocolIndex].ExclusiveP2aTransportToken,
                   DeviceIndex,
                   ProtocolIndex,
                   FALSE,
                   TRUE,
                   &UsageList,
                   &LocalUsageCount
                   );
        if (EFI_ERROR (Status)) {
          goto error_handler;
        }
      }

      if (ProtocolInfo[ProtocolIndex].NotificationsSupported &&
          (ScmiDeviceInfo[DeviceIndex].CommonP2aTransportToken ==
           CM_NULL_TOKEN) &&
          (ProtocolInfo[ProtocolIndex].ExclusiveP2aTransportToken ==
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
  Attach an Integer object to a Package.

  @param [in] Package  Package receiving the Integer object.
  @param [in] Value    Integer value.

  @retval EFI_SUCCESS            The Integer object was attached.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
**/
STATIC
EFI_STATUS
AttachIntegerToPackage (
  IN AML_OBJECT_NODE_HANDLE  Package,
  IN UINT64                  Value
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  IntegerNode;

  if (Package == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  IntegerNode = NULL;
  Status      = AmlCodeGenInteger (Value, &IntegerNode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AmlAttachNode (
             (AML_NODE_HANDLE)Package,
             (AML_NODE_HANDLE)IntegerNode
             );
  if (EFI_ERROR (Status)) {
    AmlDeleteTree ((AML_NODE_HANDLE)IntegerNode);
  }

  return Status;
}

/**
  Attach a String object to a Package.

  @param [in] Package  Package receiving the String object.
  @param [in] String   NULL-terminated string.

  @retval EFI_SUCCESS            The String object was attached.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
**/
STATIC
EFI_STATUS
AttachStringToPackage (
  IN AML_OBJECT_NODE_HANDLE  Package,
  IN CONST CHAR8             *String
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  StringNode;

  if ((Package == NULL) || (String == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  StringNode = NULL;
  Status     = AmlCodeGenString (String, &StringNode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AmlAttachNode (
             (AML_NODE_HANDLE)Package,
             (AML_NODE_HANDLE)StringNode
             );
  if (EFI_ERROR (Status)) {
    AmlDeleteTree ((AML_NODE_HANDLE)StringNode);
  }

  return Status;
}

/**
  Create and attach an empty Package to another Package.

  @param [in]  ParentPackage  Package receiving the new Package.
  @param [out] NewPackage     Newly created Package.

  @retval EFI_SUCCESS            The Package was created and attached.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
**/
STATIC
EFI_STATUS
AttachPackageToPackage (
  IN  AML_OBJECT_NODE_HANDLE          ParentPackage,
  OUT AML_OBJECT_NODE_HANDLE  *CONST  NewPackage
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  PackageNode;

  if ((ParentPackage == NULL) || (NewPackage == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  *NewPackage = NULL;
  PackageNode = NULL;

  Status = AmlCodeGenPackage (&PackageNode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AmlAttachNode (
             (AML_NODE_HANDLE)ParentPackage,
             (AML_NODE_HANDLE)PackageNode
             );
  if (EFI_ERROR (Status)) {
    AmlDeleteTree ((AML_NODE_HANDLE)PackageNode);
    return Status;
  }

  *NewPackage = PackageNode;
  return EFI_SUCCESS;
}

/**
  Generate the SCMI PCC transport property for an SCMI device.

  @param [in] DeviceIndex      Index of the SCMI device.
  @param [in] TransportUsage   Validated transport usage array.
  @param [in] UsageCount       Number of transport usages.
  @param [in] Properties       SCMI Device Properties Package.

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
  IN AML_OBJECT_NODE_HANDLE          Properties
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  Property;
  AML_OBJECT_NODE_HANDLE  Value;
  AML_OBJECT_NODE_HANDLE  Descriptor;
  UINT64                  Flags;
  UINT64                  TransportCount;
  UINTN                   Index;

  if ((TransportUsage == NULL) ||
      (UsageCount == 0)         ||
      (Properties == NULL))
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

  Status = AttachPackageToPackage (Properties, &Property);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AttachStringToPackage (Property, SCMI_PCC_TRANSPORT_PROPERTY);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AttachPackageToPackage (Property, &Value);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AttachIntegerToPackage (
             Value,
             SCMI_DEVICE_PROPERTIES_REVISION
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AttachIntegerToPackage (Value, TransportCount);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < UsageCount; Index++) {
    if (TransportUsage[Index].DeviceIndex != DeviceIndex) {
      continue;
    }

    Status = AttachPackageToPackage (Value, &Descriptor);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = AttachIntegerToPackage (
               Descriptor,
               TransportUsage[Index].PccSubspaceId
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = AttachIntegerToPackage (
               Descriptor,
               TransportUsage[Index].TransportUid
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Flags = 0;
    if (TransportUsage[Index].IsCommon) {
      Flags |= SCMI_PCC_TRANSPORT_FLAG_COMMON;
    }

    if (TransportUsage[Index].IsP2a) {
      Flags |= SCMI_PCC_TRANSPORT_FLAG_P2A;
    }

    Status = AttachIntegerToPackage (Descriptor, Flags);
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
  @param [in] Properties       SCMI Device Properties Package.

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
  IN AML_OBJECT_NODE_HANDLE          Properties
  )
{
  EFI_STATUS              Status;
  CONST CHAR8             *PropertyName;
  AML_OBJECT_NODE_HANDLE  Property;
  AML_OBJECT_NODE_HANDLE  Value;
  AML_OBJECT_NODE_HANDLE  ExclusiveTransports;
  AML_OBJECT_NODE_HANDLE  Association;
  AML_OBJECT_NODE_HANDLE  Descriptor;
  UINTN                   Index;

  if ((TransportUsage == NULL) ||
      (UsageCount == 0)         ||
      (Properties == NULL))
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

  Status = AttachPackageToPackage (Properties, &Property);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AttachStringToPackage (Property, PropertyName);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AttachPackageToPackage (Property, &Value);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AttachIntegerToPackage (
             Value,
             SCMI_DEVICE_PROPERTIES_REVISION
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AttachPackageToPackage (Value, &ExclusiveTransports);
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

    Status = AttachPackageToPackage (
               ExclusiveTransports,
               &Descriptor
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = AttachIntegerToPackage (
               Descriptor,
               TransportUsage[Index].TransportUid
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Protocol-exclusive transport flags are reserved and must be zero.
    Status = AttachIntegerToPackage (Descriptor, 0);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  // The protocol association package must be empty in revision 1.
  Status = AttachPackageToPackage (Value, &Association);
  if (EFI_ERROR (Status)) {
    return Status;
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
  AML_OBJECT_NODE_HANDLE     Properties;
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
             &Properties
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = BuildScmiTransportProperty (
             DeviceIndex,
             TransportUsage,
             UsageCount,
             Properties
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
               Properties
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
