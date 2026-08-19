/** @file
  SSDT Generic Event Device Generator.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI Specification, section 5.6.9,
    "Interrupt-signaled ACPI events".
**/

#include <IndustryStandard/Acpi66.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>

#include <AcpiTableGenerator.h>
#include <ConfigurationManagerHelper.h>
#include <ConfigurationManagerObject.h>
#include <Library/AmlLib/AmlLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include "SsdtGedGenerator.h"

/** SSDT Generic Event Device generator requirements.

  The following Configuration Manager objects are required:
  - EArchCommonObjGedDeviceInfo
  - EArchCommonObjGedEventInfo
  - EArchCommonObjGedActionInfo
*/

/** This macro expands to a function that retrieves GED device information
    from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjGedDeviceInfo,
  CM_ARCH_COMMON_GED_DEVICE_INFO
  );

/** This macro expands to a function that retrieves GED event information
    from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjGedEventInfo,
  CM_ARCH_COMMON_GED_EVENT_INFO
  );

/** This macro expands to a function that retrieves GED action information
    from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjGedActionInfo,
  CM_ARCH_COMMON_GED_ACTION_INFO
  );

/**
  Generate the ACPI NameSeg for a GED device.

  GED identifiers from 0 through 15 are represented as GED0 through GEDF.

  @param [in]  Id              GED device identifier.
  @param [out] DeviceName      Buffer receiving the generated NameSeg.
  @param [in]  DeviceNameSize  Size of DeviceName in bytes.

  @retval EFI_SUCCESS            The NameSeg was generated successfully.
  @retval EFI_INVALID_PARAMETER  An input parameter is invalid.
**/
STATIC
EFI_STATUS
GetGedDeviceName (
  IN  UINT32  Id,
  OUT CHAR8   *DeviceName,
  IN  UINTN   DeviceNameSize
  )
{
  if ((DeviceName == NULL) ||
      (DeviceNameSize < (AML_NAME_SEG_SIZE + 1)) ||
      (Id > GED_MAXIMUM_ID))
  {
    return EFI_INVALID_PARAMETER;
  }

  AsciiSPrint (
    DeviceName,
    DeviceNameSize,
    "GED%X",
    Id
    );

  return EFI_SUCCESS;
}

/**
  Convert a register width to the corresponding AML Field access type.

  @param [in]  RegisterBitWidth  Register width in bits.
  @param [out] AccessType        AML Field access type.

  @retval EFI_SUCCESS            The access type was returned successfully.
  @retval EFI_INVALID_PARAMETER  The register width or output pointer is
                                 invalid.
**/
STATIC
EFI_STATUS
GetFieldAccessType (
  IN  UINT32                 RegisterBitWidth,
  OUT AML_FIELD_ACCESS_TYPE  *AccessType
  )
{
  if (AccessType == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  switch (RegisterBitWidth) {
    case 8:
      *AccessType = AmlFieldAccessByte;
      break;

    case 16:
      *AccessType = AmlFieldAccessWord;
      break;

    case 32:
      *AccessType = AmlFieldAccessDWord;
      break;

    case 64:
      *AccessType = AmlFieldAccessQWord;
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Validate a GED register-write action.

  @param [in] DeviceInfo  GED device information.
  @param [in] ActionInfo  GED action information.

  @retval EFI_SUCCESS            The action is valid.
  @retval EFI_INVALID_PARAMETER  The action is invalid.
**/
STATIC
EFI_STATUS
ValidateRegisterWriteAction (
  IN CONST CM_ARCH_COMMON_GED_DEVICE_INFO  *DeviceInfo,
  IN CONST CM_ARCH_COMMON_GED_ACTION_INFO  *ActionInfo
  )
{
  UINT64  RegisterSize;

  if ((DeviceInfo == NULL) || (ActionInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (DeviceInfo->RegisterRegion.Length == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: A register-write action requires a register region.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  switch (ActionInfo->RegisterBitWidth) {
    case 8:
    case 16:
    case 32:
    case 64:
      break;

    default:
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-GED: Invalid register width %u.\n",
        ActionInfo->RegisterBitWidth
        ));
      return EFI_INVALID_PARAMETER;
  }

  RegisterSize = ActionInfo->RegisterBitWidth / 8;

  if ((ActionInfo->RegisterOffset >
       DeviceInfo->RegisterRegion.Length) ||
      (RegisterSize >
       (DeviceInfo->RegisterRegion.Length -
        ActionInfo->RegisterOffset)))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Register is outside the GED register region.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  //
  // FieldPkgLen can encode values below 2^28 bits. RegisterOffset is
  // expressed in bytes, so it must be below 2^25.
  //
  if (ActionInfo->RegisterOffset >= (1ULL << 25)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Register offset is too large.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Avoid shifting a 64-bit value by 64.
  //
  if ((ActionInfo->RegisterBitWidth < 64) &&
      ((ActionInfo->Value >> ActionInfo->RegisterBitWidth) != 0))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Register value exceeds the register width.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Generate an AML Field containing one register.

  A separate Field declaration is generated for each register-write action.
  This permits actions to reference registers in any order and avoids
  requiring the Configuration Manager data to be sorted by register offset.

  @param [in]  ActionInfo     GED register-write action.
  @param [in]  FieldIndex     Index used to generate the Field NameSeg.
  @param [in]  DeviceNode     Parent GED Device node.
  @param [out] FieldName      Buffer receiving the generated field name.
  @param [in]  FieldNameSize  Size of FieldName in bytes.

  @retval EFI_SUCCESS            The Field was generated successfully.
  @retval EFI_INVALID_PARAMETER  An input parameter is invalid.
  @retval Others                 Error returned by an AML code generator.
**/
STATIC
EFI_STATUS
BuildRegisterField (
  IN  CONST CM_ARCH_COMMON_GED_ACTION_INFO  *ActionInfo,
  IN        UINT32                          FieldIndex,
  IN        AML_OBJECT_NODE_HANDLE          DeviceNode,
  OUT       CHAR8                           *FieldName,
  IN        UINTN                           FieldNameSize
  )
{
  EFI_STATUS              Status;
  AML_FIELD_ACCESS_TYPE   AccessType;
  AML_OBJECT_NODE_HANDLE  FieldNode;

  FieldNode = NULL;

  if ((ActionInfo == NULL) ||
      (DeviceNode == NULL) ||
      (FieldName == NULL) ||
      (FieldNameSize < (AML_NAME_SEG_SIZE + 1)) ||
      (FieldIndex >= GED_MAXIMUM_FIELD_COUNT))
  {
    return EFI_INVALID_PARAMETER;
  }

  AsciiSPrint (
    FieldName,
    FieldNameSize,
    "F%03X",
    FieldIndex
    );

  Status = GetFieldAccessType (
             ActionInfo->RegisterBitWidth,
             &AccessType
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AmlCodeGenField (
             GED_OPERATION_REGION_NAME,
             AccessType,
             AmlFieldNoLock,
             AmlFieldUpdatePreserve,
             DeviceNode,
             &FieldNode
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Failed to generate Field. Status = %r\n",
      Status
      ));
    return Status;
  }

  if (ActionInfo->RegisterOffset != 0) {
    Status = AmlCodeGenFieldElemReserved (
               (UINT32)(ActionInfo->RegisterOffset * 8),
               FieldNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-GED: Failed to generate ReservedField. Status = %r\n",
        Status
        ));
      return Status;
    }
  }

  Status = AmlCodeGenFieldElemNamed (
             FieldName,
             ActionInfo->RegisterBitWidth,
             FieldNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Failed to generate NamedField. Status = %r\n",
      Status
      ));
  }

  return Status;
}

/**
  Generate one AML statement for a GED action.

  @param [in]      DeviceInfo  GED device information.
  @param [in]      ActionInfo  GED action information.
  @param [in]      DeviceNode  GED Device node.
  @param [in]      IfNode      If node receiving the generated statement.
  @param [in, out] FieldIndex  Index used to generate unique Field names.

  @retval EFI_SUCCESS            The action was generated successfully.
  @retval EFI_INVALID_PARAMETER  An input parameter is invalid.
  @retval Others                 Error returned by an AML code generator.
**/
STATIC
EFI_STATUS
BuildGedAction (
  IN     CONST CM_ARCH_COMMON_GED_DEVICE_INFO  *DeviceInfo,
  IN     CONST CM_ARCH_COMMON_GED_ACTION_INFO  *ActionInfo,
  IN           AML_OBJECT_NODE_HANDLE          DeviceNode,
  IN           AML_OBJECT_NODE_HANDLE          IfNode,
  IN OUT       UINT32                          *FieldIndex
  )
{
  EFI_STATUS        Status;
  CHAR8             FieldName[AML_NAME_SEG_SIZE + 1];
  AML_METHOD_PARAM  NotifyObject;
  AML_METHOD_PARAM  StoreSource;

  if ((DeviceInfo == NULL) ||
      (ActionInfo == NULL) ||
      (DeviceNode == NULL) ||
      (IfNode == NULL) ||
      (FieldIndex == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  switch (ActionInfo->ActionType) {
    case GedActionWriteRegister:
      if (*FieldIndex >= GED_MAXIMUM_FIELD_COUNT) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SSDT-GED: Too many register-write actions.\n"
          ));
        return EFI_INVALID_PARAMETER;
      }

      Status = ValidateRegisterWriteAction (
                 DeviceInfo,
                 ActionInfo
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Status = BuildRegisterField (
                 ActionInfo,
                 *FieldIndex,
                 DeviceNode,
                 FieldName,
                 sizeof (FieldName)
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      (*FieldIndex)++;

      StoreSource.Type         = AmlMethodParamTypeInteger;
      StoreSource.Data.Integer = ActionInfo->Value;
      StoreSource.DataSize     = 0;

      Status = AmlCodeGenStoreToName (
                 StoreSource,
                 FieldName,
                 IfNode,
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SSDT-GED: Failed to generate Store. Status = %r\n",
          Status
          ));
      }

      return Status;

    case GedActionNotify:
      if ((ActionInfo->NotifyObject == NULL) ||
          (ActionInfo->NotifyObject[0] == '\0'))
      {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SSDT-GED: Invalid Notify object.\n"
          ));
        return EFI_INVALID_PARAMETER;
      }

      NotifyObject.Type        = AmlMethodParamTypeString;
      NotifyObject.Data.Buffer = (VOID *)ActionInfo->NotifyObject;
      NotifyObject.DataSize    = AsciiStrSize (ActionInfo->NotifyObject);

      Status = AmlCodeGenNotify (
                 NotifyObject,
                 ActionInfo->NotifyValue,
                 IfNode,
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SSDT-GED: Failed to generate Notify. Status = %r\n",
          Status
          ));
      }

      return Status;

    default:
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-GED: Unsupported action type %u.\n",
        ActionInfo->ActionType
        ));
      return EFI_INVALID_PARAMETER;
  }
}

/**
  Generate AML for one GED event.

  The generated AML is equivalent to:

    If (LEqual (Arg0, EventInterrupt))
    {
      Action0
      Action1
      ...
    }

  @param [in]      CfgMgrProtocol  Configuration Manager protocol.
  @param [in]      DeviceInfo     GED device information.
  @param [in]      EventInfo      GED event information.
  @param [in]      CrsNode        _CRS ResourceTemplate node.
  @param [in]      DeviceNode     GED Device node.
  @param [in]      MethodNode     _EVT Method node.
  @param [in, out] FieldIndex     Index used to generate unique Field names.

  @retval EFI_SUCCESS            The event was generated successfully.
  @retval EFI_INVALID_PARAMETER  An input parameter is invalid.
  @retval Others                 Error returned by a helper function.
**/
STATIC
EFI_STATUS
BuildGedEvent (
  IN     CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST   CfgMgrProtocol,
  IN     CONST CM_ARCH_COMMON_GED_DEVICE_INFO         *CONST  DeviceInfo,
  IN     CONST CM_ARCH_COMMON_GED_EVENT_INFO          *CONST  EventInfo,
  IN           AML_OBJECT_NODE_HANDLE                         CrsNode,
  IN           AML_OBJECT_NODE_HANDLE                         DeviceNode,
  IN           AML_OBJECT_NODE_HANDLE                         MethodNode,
  IN OUT       UINT32                                         *FieldIndex
  )
{
  EFI_STATUS                      Status;
  UINT32                          Interrupt;
  UINT32                          ActionCount;
  UINT32                          ActionIndex;
  AML_METHOD_PARAM                LeftOperand;
  AML_METHOD_PARAM                RightOperand;
  AML_OBJECT_NODE_HANDLE          PredicateNode;
  AML_OBJECT_NODE_HANDLE          IfNode;
  CM_ARCH_COMMON_GED_ACTION_INFO  *ActionInfo;

  PredicateNode = NULL;
  IfNode        = NULL;
  ActionInfo    = NULL;

  if ((CfgMgrProtocol == NULL) ||
      (DeviceInfo == NULL) ||
      (EventInfo == NULL) ||
      (CrsNode == NULL) ||
      (DeviceNode == NULL) ||
      (MethodNode == NULL) ||
      (FieldIndex == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((EventInfo->EventInterrupt.Flags & ~(BIT0 | BIT1)) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Invalid interrupt flags 0x%x.\n",
      EventInfo->EventInterrupt.Flags
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (EventInfo->ActionInfoToken == CM_NULL_TOKEN) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Event action token is NULL.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  Interrupt = EventInfo->EventInterrupt.Interrupt;

  Status = AmlCodeGenRdInterrupt (
             TRUE,
             ((EventInfo->EventInterrupt.Flags & BIT0) != 0),
             ((EventInfo->EventInterrupt.Flags & BIT1) != 0),
             EventInfo->Shared,
             &Interrupt,
             1,
             CrsNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Failed to generate interrupt resource."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = GetEArchCommonObjGedActionInfo (
             CfgMgrProtocol,
             EventInfo->ActionInfoToken,
             &ActionInfo,
             &ActionCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Failed to get action information."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  if ((ActionInfo == NULL) || (ActionCount == 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Event has no actions.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  LeftOperand.Type     = AmlMethodParamTypeArg;
  LeftOperand.Data.Arg = 0;
  LeftOperand.DataSize = 0;

  RightOperand.Type         = AmlMethodParamTypeInteger;
  RightOperand.Data.Integer = Interrupt;
  RightOperand.DataSize     = 0;

  Status = AmlCodeGenEqual (
             LeftOperand,
             RightOperand,
             NULL,
             &PredicateNode
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Failed to generate event predicate."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = AmlCodeGenIf (
             PredicateNode,
             MethodNode,
             &IfNode
             );

  //
  // AmlCodeGenIf() takes ownership of PredicateNode.
  //
  PredicateNode = NULL;

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Failed to generate If statement."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  for (ActionIndex = 0; ActionIndex < ActionCount; ActionIndex++) {
    Status = BuildGedAction (
               DeviceInfo,
               &ActionInfo[ActionIndex],
               DeviceNode,
               IfNode,
               FieldIndex
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-GED: Failed to generate action %u."
        " Status = %r\n",
        ActionIndex,
        Status
        ));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Check whether a GED event list contains duplicate interrupts.

  @param [in] EventInfo   GED event information.
  @param [in] EventCount  Number of GED event entries.

  @retval TRUE   Duplicate interrupt numbers were found.
  @retval FALSE  All interrupt numbers are unique.
**/
STATIC
BOOLEAN
HasDuplicateInterrupts (
  IN CONST CM_ARCH_COMMON_GED_EVENT_INFO  *EventInfo,
  IN       UINT32                         EventCount
  )
{
  UINT32  Index;
  UINT32  CompareIndex;

  if (EventInfo == NULL) {
    return TRUE;
  }

  for (Index = 0; Index < EventCount; Index++) {
    for (CompareIndex = Index + 1;
         CompareIndex < EventCount;
         CompareIndex++)
    {
      if (EventInfo[Index].EventInterrupt.Interrupt ==
          EventInfo[CompareIndex].EventInterrupt.Interrupt)
      {
        return TRUE;
      }
    }
  }

  return FALSE;
}

/**
  Generate one GED Device object.

  @param [in] CfgMgrProtocol  Configuration Manager protocol.
  @param [in] DeviceInfo     GED device information.
  @param [in] ScopeNode      \_SB scope node.

  @retval EFI_SUCCESS            The GED device was generated successfully.
  @retval EFI_INVALID_PARAMETER  An input parameter is invalid.
  @retval Others                 Error returned by a helper function.
**/
STATIC
EFI_STATUS
BuildGedDevice (
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST   CfgMgrProtocol,
  IN CONST CM_ARCH_COMMON_GED_DEVICE_INFO         *CONST  DeviceInfo,
  IN       AML_OBJECT_NODE_HANDLE                         ScopeNode
  )
{
  EFI_STATUS                     Status;
  CHAR8                          DeviceName[AML_NAME_SEG_SIZE + 1];
  UINT32                         EventCount;
  UINT32                         EventIndex;
  UINT32                         FieldIndex;
  AML_OBJECT_NODE_HANDLE         DeviceNode;
  AML_OBJECT_NODE_HANDLE         CrsNode;
  AML_OBJECT_NODE_HANDLE         MethodNode;
  CM_ARCH_COMMON_GED_EVENT_INFO  *EventInfo;

  DeviceNode = NULL;
  CrsNode    = NULL;
  MethodNode = NULL;
  EventInfo  = NULL;
  FieldIndex = 0;

  if ((CfgMgrProtocol == NULL) ||
      (DeviceInfo == NULL) ||
      (ScopeNode == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (DeviceInfo->EventInfoToken == CM_NULL_TOKEN) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: GED event token is NULL.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = GetGedDeviceName (
             DeviceInfo->Id,
             DeviceName,
             sizeof (DeviceName)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Invalid GED identifier %u.\n",
      DeviceInfo->Id
      ));
    return Status;
  }

  Status = GetEArchCommonObjGedEventInfo (
             CfgMgrProtocol,
             DeviceInfo->EventInfoToken,
             &EventInfo,
             &EventCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Failed to get event information."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  if ((EventInfo == NULL) || (EventCount == 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: GED device has no events.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (HasDuplicateInterrupts (EventInfo, EventCount)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: GED device has duplicate interrupts.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlCodeGenDevice (
             DeviceName,
             ScopeNode,
             &DeviceNode
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Failed to generate Device."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = AmlCodeGenNameString (
             "_HID",
             GED_HARDWARE_ID,
             DeviceNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AmlCodeGenNameInteger (
             "_UID",
             DeviceInfo->Id,
             DeviceNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AmlCodeGenNameResourceTemplate (
             "_CRS",
             DeviceNode,
             &CrsNode
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Failed to generate _CRS."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  //
  // The OperationRegion is optional for GEDs containing only Notify actions.
  //
  if (DeviceInfo->RegisterRegion.Length != 0) {
    Status = AmlCodeGenOperationRegion (
               GED_OPERATION_REGION_NAME,
               EFI_ACPI_6_6_SYSTEM_MEMORY,
               DeviceInfo->RegisterRegion.BaseAddress,
               DeviceInfo->RegisterRegion.Length,
               DeviceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-GED: Failed to generate OperationRegion."
        " Status = %r\n",
        Status
        ));
      return Status;
    }
  }

  Status = AmlCodeGenMethod (
             "_EVT",
             1,
             TRUE,
             0,
             DeviceNode,
             &MethodNode
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Failed to generate _EVT."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  for (EventIndex = 0; EventIndex < EventCount; EventIndex++) {
    Status = BuildGedEvent (
               CfgMgrProtocol,
               DeviceInfo,
               &EventInfo[EventIndex],
               CrsNode,
               DeviceNode,
               MethodNode,
               &FieldIndex
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-GED: Failed to generate event %u."
        " Status = %r\n",
        EventIndex,
        Status
        ));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Check whether a GED device list contains duplicate identifiers.

  @param [in] DeviceInfo   GED device information.
  @param [in] DeviceCount  Number of GED device entries.

  @retval TRUE   Duplicate identifiers were found.
  @retval FALSE  All identifiers are unique.
**/
STATIC
BOOLEAN
HasDuplicateDeviceIds (
  IN CONST CM_ARCH_COMMON_GED_DEVICE_INFO  *DeviceInfo,
  IN       UINT32                          DeviceCount
  )
{
  UINT32  Index;
  UINT32  CompareIndex;

  if (DeviceInfo == NULL) {
    return TRUE;
  }

  for (Index = 0; Index < DeviceCount; Index++) {
    for (CompareIndex = Index + 1;
         CompareIndex < DeviceCount;
         CompareIndex++)
    {
      if (DeviceInfo[Index].Id == DeviceInfo[CompareIndex].Id) {
        return TRUE;
      }
    }
  }

  return FALSE;
}

/**
  Build the SSDT containing the Generic Event Devices.

  @param [in]  This            Pointer to the ACPI table generator.
  @param [in]  AcpiTableInfo   Pointer to the ACPI table information.
  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager protocol.
  @param [out] Table           Pointer to the generated SSDT.

  @retval EFI_SUCCESS            The SSDT was generated successfully.
  @retval EFI_INVALID_PARAMETER  An input parameter is invalid.
  @retval Others                 Error returned by a helper function.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSsdtGedTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO    *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL
  *CONST                                          CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER           **Table
  )
{
  EFI_STATUS                      Status;
  EFI_STATUS                      CleanupStatus;
  UINT32                          DeviceCount;
  UINT32                          DeviceIndex;
  AML_ROOT_NODE_HANDLE            RootNode;
  AML_OBJECT_NODE_HANDLE          ScopeNode;
  CM_ARCH_COMMON_GED_DEVICE_INFO  *DeviceInfo;

  RootNode   = NULL;
  ScopeNode  = NULL;
  DeviceInfo = NULL;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((This == NULL) ||
      (AcpiTableInfo == NULL) ||
      (CfgMgrProtocol == NULL) ||
      (Table == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  Status = GetEArchCommonObjGedDeviceInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &DeviceInfo,
             &DeviceCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Failed to get GED device information."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  if ((DeviceInfo == NULL) || (DeviceCount == 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: No GED devices were provided.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (HasDuplicateDeviceIds (DeviceInfo, DeviceCount)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Duplicate GED device identifiers.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = AddSsdtAcpiHeader (
             CfgMgrProtocol,
             This,
             AcpiTableInfo,
             &RootNode
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Failed to generate the SSDT header."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = AmlCodeGenScope (
             "\\_SB_",
             RootNode,
             &ScopeNode
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Failed to generate the _SB scope."
      " Status = %r\n",
      Status
      ));
    goto cleanup_handler;
  }

  for (DeviceIndex = 0; DeviceIndex < DeviceCount; DeviceIndex++) {
    Status = BuildGedDevice (
               CfgMgrProtocol,
               &DeviceInfo[DeviceIndex],
               ScopeNode
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-GED: Failed to generate GED device %u."
        " Status = %r\n",
        DeviceIndex,
        Status
        ));
      goto cleanup_handler;
    }
  }

  Status = AmlSerializeDefinitionBlock (
             RootNode,
             Table
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Failed to serialize the SSDT."
      " Status = %r\n",
      Status
      ));
  }

cleanup_handler:
  if (RootNode != NULL) {
    CleanupStatus = AmlDeleteTree (RootNode);
    if (EFI_ERROR (CleanupStatus)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-GED: Failed to delete the AML tree."
        " Status = %r\n",
        CleanupStatus
        ));

      if (!EFI_ERROR (Status)) {
        FreePool (*Table);
        *Table = NULL;
        Status = CleanupStatus;
      }
    }
  }

  return Status;
}

/**
  Free resources allocated for the generated GED SSDT.

  @param [in]      This            Pointer to the ACPI table generator.
  @param [in]      AcpiTableInfo   Pointer to the ACPI table information.
  @param [in]      CfgMgrProtocol  Pointer to the Configuration Manager
                                   protocol.
  @param [in, out] Table           Pointer to the generated table.

  @retval EFI_SUCCESS            The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER  The table pointer is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSsdtGedTableResources (
  IN      CONST ACPI_TABLE_GENERATOR                   *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER           **CONST  Table
  )
{
  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((Table == NULL) || (*Table == NULL)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GED: Invalid table pointer.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;

  return EFI_SUCCESS;
}

/** The interface for the SSDT GED generator. */
STATIC CONST ACPI_TABLE_GENERATOR  SsdtGedGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtGed),
  // Generator Description
  L"ACPI.STD.SSDT.GED.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_6_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
  // ACPI Table Revision - Unused
  0,
  // Minimum ACPI Table Revision - Unused
  0,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID,
  // Creator Revision
  SSDT_GED_GENERATOR_REVISION,
  // Build table function
  BuildSsdtGedTable,
  // Free table function
  FreeSsdtGedTableResources,
  // Extended build table function - Not used
  NULL,
  // Extended free table function - Not used
  NULL
};

/**
  Register the GED generator with the ACPI Table Factory.

  @param [in] ImageHandle  Handle for the image.
  @param [in] SystemTable  Pointer to the EFI system table.

  @retval EFI_SUCCESS            The generator was registered successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_ALREADY_STARTED    The generator is already registered.
**/
EFI_STATUS
EFIAPI
AcpiSsdtGedLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&SsdtGedGenerator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-GED: Register generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Deregister the GED generator from the ACPI Table Factory.

  @param [in] ImageHandle  Handle for the image.
  @param [in] SystemTable  Pointer to the EFI system table.

  @retval EFI_SUCCESS            The generator was deregistered successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          The generator is not registered.
**/
EFI_STATUS
EFIAPI
AcpiSsdtGedLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&SsdtGedGenerator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-GED: Deregister generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);

  return Status;
}
