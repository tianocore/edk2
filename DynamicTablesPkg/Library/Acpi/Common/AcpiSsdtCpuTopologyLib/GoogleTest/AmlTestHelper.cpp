/** @file
  AML Test Helper implementation for validating generated SSDT CPU Topology tables.

  Provides utilities to parse and validate AML output against ACPI 6.6
  specification requirements.

  Copyright (c) 2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.6 Specification - s8.4 Declaring Processors
    - ACPI 6.6 Specification - s8.4.7.1 _CPC (Continuous Performance Control)
    - ACPI 6.6 Specification - s8.4.4 _LPI (Low Power Idle States)
    - ACPI 6.6 Specification - s8.4.5.5 _PSD (P-State Dependency)
    - ACPI 6.6 Specification - s8.4.1.1 _CST (C-State)
    - ACPI 6.6 Specification - s8.4.1.2 _CSD (C-State Dependency)
    - ACPI 6.6 Specification - s8.4.5.1 _PCT (Performance Control)
    - ACPI 6.6 Specification - s8.4.5.2 _PSS (Performance Supported States)
    - ACPI 6.6 Specification - s8.4.5.3 _PPC (Performance Present Capabilities)
**/

#include "AmlTestHelper.h"

// =============================================================================
// Basic AML Tree Navigation
// =============================================================================

EFI_STATUS
AmlTestHelper::ParseSsdtTable (
  IN  EFI_ACPI_DESCRIPTION_HEADER  *Table,
  OUT AML_ROOT_NODE_HANDLE         *RootNode
  )
{
  if ((Table == NULL) || (RootNode == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  return AmlParseDefinitionBlock (Table, RootNode);
}

EFI_STATUS
AmlTestHelper::FindDeviceInScope (
  IN  AML_NODE_HANDLE  ScopeNode,
  IN  CONST CHAR8      *DeviceName,
  OUT AML_NODE_HANDLE  *OutNode
  )
{
  AML_NODE_HEADER  *Child;

  if (!IS_AML_OBJECT_NODE (ScopeNode)) {
    return EFI_INVALID_PARAMETER;
  }

  // Iterate through children of the scope
  Child = AmlGetNextVariableArgument ((AML_NODE_HEADER *)ScopeNode, NULL);
  while (Child != NULL) {
    if (IS_AML_OBJECT_NODE (Child)) {
      // Check if this is a Device node - we can look at the fixed args
      AML_OBJECT_NODE  *ObjNode = (AML_OBJECT_NODE *)Child;

      // Fixed argument 0 of a Device node should be the name (4-char NameSeg)
      AML_NODE_HEADER  *NameArg = AmlGetFixedArgument (ObjNode, EAmlParseIndexTerm0);
      if (IS_AML_DATA_NODE (NameArg)) {
        AML_DATA_NODE  *DataNode = (AML_DATA_NODE *)NameArg;
        // WARNING: AML names are NOT null-terminated! They are exactly 4 bytes.
        // Use CompareMem instead of AsciiStrnCmp to avoid buffer overrun.
        if ((DataNode->Buffer != NULL) && (DataNode->Size == 4) &&
            (CompareMem (DataNode->Buffer, DeviceName, 4) == 0))
        {
          *OutNode = (AML_NODE_HANDLE)Child;
          return EFI_SUCCESS;
        }
      }
    }

    Child = AmlGetNextVariableArgument ((AML_NODE_HEADER *)ScopeNode, Child);
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
AmlTestHelper::FindDeviceByPath (
  IN  AML_ROOT_NODE_HANDLE  RootNode,
  IN  CONST CHAR8           *AslPath,
  OUT AML_NODE_HANDLE       *OutNode
  )
{
  EFI_STATUS       Status;
  AML_NODE_HEADER  *Child;
  AML_NODE_HANDLE  ScopeNode = NULL;

  if ((RootNode == NULL) || (AslPath == NULL) || (OutNode == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // For path like "\_SB.C000", we need to:
  // 1. Find the _SB_ scope as a child of root
  // 2. Find C000 device as a child of _SB_

  // First, find _SB_ scope in root's children
  Child = AmlGetNextVariableArgument ((AML_NODE_HEADER *)RootNode, NULL);

  while (Child != NULL) {
    if (IS_AML_OBJECT_NODE (Child)) {
      AML_OBJECT_NODE  *ObjNode = (AML_OBJECT_NODE *)Child;
      AML_NODE_HEADER  *NameArg = AmlGetFixedArgument (ObjNode, EAmlParseIndexTerm0);

      if ((NameArg != NULL) && IS_AML_DATA_NODE (NameArg)) {
        AML_DATA_NODE  *DataNode = (AML_DATA_NODE *)NameArg;
        // Check if this contains "_SB" (the name might be \_SB or _SB_)
        // Look for the sequence '_' 'S' 'B' in the buffer (safely)
        if ((DataNode->Buffer != NULL) && (DataNode->Size >= 3)) {
          for (UINT32 i = 0; i <= DataNode->Size - 3; i++) {
            if ((DataNode->Buffer[i] == '_') &&
                (DataNode->Buffer[i+1] == 'S') &&
                (DataNode->Buffer[i+2] == 'B'))
            {
              ScopeNode = (AML_NODE_HANDLE)Child;
              break;
            }
          }

          if (ScopeNode != NULL) {
            break;
          }
        }
      }
    }

    Child = AmlGetNextVariableArgument ((AML_NODE_HEADER *)RootNode, Child);
  }

  if (ScopeNode == NULL) {
    return EFI_NOT_FOUND;
  }

  // Now find the device in the scope (extract device name from path)
  // Path is like "\_SB.C000", device name is "C000"
  CONST CHAR8  *DeviceName = AsciiStrStr (AslPath, ".");

  if (DeviceName == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DeviceName++;  // Skip the dot

  Status = FindDeviceInScope (ScopeNode, DeviceName, OutNode);
  return Status;
}

BOOLEAN
AmlTestHelper::IsDeviceNode (
  IN  AML_NODE_HANDLE  Node
  )
{
  if (!IS_AML_OBJECT_NODE (Node)) {
    return FALSE;
  }

  return AmlNodeCompareOpCode (
           (AML_OBJECT_NODE *)Node,
           AML_EXT_OP,
           AML_EXT_DEVICE_OP
           );
}

BOOLEAN
AmlTestHelper::IsNameNode (
  IN  AML_NODE_HANDLE  Node
  )
{
  if (!IS_AML_OBJECT_NODE (Node)) {
    return FALSE;
  }

  return AmlNodeCompareOpCode (
           (AML_OBJECT_NODE *)Node,
           AML_NAME_OP,
           0
           );
}

EFI_STATUS
AmlTestHelper::FindNamedObjectInDevice (
  IN  AML_NODE_HANDLE  DeviceNode,
  IN  CONST CHAR8      *Name,
  OUT AML_NODE_HANDLE  *OutNode
  )
{
  AML_NODE_HEADER  *Child;

  if ((DeviceNode == NULL) || (Name == NULL) || (OutNode == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IS_AML_OBJECT_NODE (DeviceNode)) {
    return EFI_INVALID_PARAMETER;
  }

  // Iterate through variable arguments (children) of the device
  Child = AmlGetNextVariableArgument ((AML_NODE_HEADER *)DeviceNode, NULL);
  while (Child != NULL) {
    if (IsNameNode ((AML_NODE_HANDLE)Child)) {
      // Get the name from fixed argument 0
      AML_NODE_HEADER  *NameArg = AmlGetFixedArgument ((AML_OBJECT_NODE *)Child, EAmlParseIndexTerm0);
      if (IS_AML_DATA_NODE (NameArg)) {
        AML_DATA_NODE  *DataNode = (AML_DATA_NODE *)NameArg;
        // AML names are exactly 4 bytes, NOT null-terminated
        // Use CompareMem to safely compare without buffer overrun
        if ((DataNode->Buffer != NULL) && (DataNode->Size == 4) &&
            (CompareMem (DataNode->Buffer, Name, 4) == 0))
        {
          *OutNode = (AML_NODE_HANDLE)Child;
          return EFI_SUCCESS;
        }
      }
    }

    Child = AmlGetNextVariableArgument ((AML_NODE_HEADER *)DeviceNode, Child);
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
AmlTestHelper::GetDeviceHid (
  IN  AML_NODE_HANDLE  DeviceNode,
  OUT CHAR8            *HidString,
  IN  UINTN            BufferSize
  )
{
  EFI_STATUS       Status;
  AML_NODE_HANDLE  HidNode;
  AML_NODE_HEADER  *ValueArg;

  if ((DeviceNode == NULL) || (HidString == NULL) || (BufferSize < 9)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FindNamedObjectInDevice (DeviceNode, "_HID", &HidNode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Get the value from fixed argument 1
  ValueArg = AmlGetFixedArgument ((AML_OBJECT_NODE *)HidNode, EAmlParseIndexTerm1);
  if (ValueArg == NULL) {
    return EFI_NOT_FOUND;
  }

  // Check if it's a string (data node with string)
  if (IS_AML_DATA_NODE (ValueArg)) {
    AML_DATA_NODE  *DataNode = (AML_DATA_NODE *)ValueArg;
    if ((DataNode->Buffer != NULL) && (DataNode->Size > 0) && (DataNode->Size < BufferSize)) {
      CopyMem (HidString, DataNode->Buffer, DataNode->Size);
      HidString[DataNode->Size] = '\0';
      return EFI_SUCCESS;
    }
  }

  // Check if it's an object node (String or EISAID)
  if (IS_AML_OBJECT_NODE (ValueArg)) {
    // For a string literal (0x0D opcode), the actual string data is in fixed arg 0
    AML_NODE_HEADER  *StringData = AmlGetFixedArgument ((AML_OBJECT_NODE *)ValueArg, EAmlParseIndexTerm0);

    if ((StringData != NULL) && IS_AML_DATA_NODE (StringData)) {
      AML_DATA_NODE  *DataNode = (AML_DATA_NODE *)StringData;
      if ((DataNode->Buffer != NULL) && (DataNode->Size > 0) && (DataNode->Size < BufferSize)) {
        CopyMem (HidString, DataNode->Buffer, DataNode->Size);
        HidString[DataNode->Size] = '\0';
        return EFI_SUCCESS;
      }
    }

    // If not a string, might be EISAID - not supported for now
    return EFI_NOT_FOUND;
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
AmlTestHelper::GetDeviceUid (
  IN  AML_NODE_HANDLE  DeviceNode,
  OUT UINT64           *Uid
  )
{
  EFI_STATUS       Status;
  AML_NODE_HANDLE  UidNode;
  AML_NODE_HEADER  *ValueArg;

  if ((DeviceNode == NULL) || (Uid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FindNamedObjectInDevice (DeviceNode, "_UID", &UidNode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Get the value from fixed argument 1
  ValueArg = AmlGetFixedArgument ((AML_OBJECT_NODE *)UidNode, EAmlParseIndexTerm1);
  if (ValueArg == NULL) {
    return EFI_NOT_FOUND;
  }

  // Handle integer values (data node or special integer opcodes)
  if (IS_AML_DATA_NODE (ValueArg)) {
    AML_DATA_NODE  *DataNode = (AML_DATA_NODE *)ValueArg;
    *Uid = 0;
    switch (DataNode->Size) {
      case 1:
        *Uid = *(UINT8 *)DataNode->Buffer;
        return EFI_SUCCESS;
      case 2:
        *Uid = *(UINT16 *)DataNode->Buffer;
        return EFI_SUCCESS;
      case 4:
        *Uid = *(UINT32 *)DataNode->Buffer;
        return EFI_SUCCESS;
      case 8:
        *Uid = *(UINT64 *)DataNode->Buffer;
        return EFI_SUCCESS;
      default:
        return EFI_NOT_FOUND;
    }
  }

  // Handle integer opcodes (ZeroOp, OneOp, BytePrefix, etc.)
  if (IS_AML_OBJECT_NODE (ValueArg)) {
    AML_OBJECT_NODE  *ObjNode = (AML_OBJECT_NODE *)ValueArg;

    // Check for special integer opcodes
    if (AmlNodeCompareOpCode (ObjNode, AML_ZERO_OP, 0)) {
      *Uid = 0;
      return EFI_SUCCESS;
    }

    if (AmlNodeCompareOpCode (ObjNode, AML_ONE_OP, 0)) {
      *Uid = 1;
      return EFI_SUCCESS;
    }

    // For BytePrefix, WordPrefix, DWordPrefix, QWordPrefix
    // the value is in the first fixed argument
    if (AmlNodeCompareOpCode (ObjNode, AML_BYTE_PREFIX, 0) ||
        AmlNodeCompareOpCode (ObjNode, AML_WORD_PREFIX, 0) ||
        AmlNodeCompareOpCode (ObjNode, AML_DWORD_PREFIX, 0) ||
        AmlNodeCompareOpCode (ObjNode, AML_QWORD_PREFIX, 0))
    {
      AML_NODE_HEADER  *IntArg = AmlGetFixedArgument (ObjNode, EAmlParseIndexTerm0);
      if (IS_AML_DATA_NODE (IntArg)) {
        AML_DATA_NODE  *IntData = (AML_DATA_NODE *)IntArg;
        *Uid = 0;
        CopyMem (Uid, IntData->Buffer, IntData->Size);
        return EFI_SUCCESS;
      }
    }
  }

  return EFI_NOT_FOUND;
}

BOOLEAN
AmlTestHelper::DeviceHasNamedObject (
  IN  AML_NODE_HANDLE  DeviceNode,
  IN  CONST CHAR8      *Name
  )
{
  AML_NODE_HANDLE  Node;
  EFI_STATUS       Status;

  Status = FindNamedObjectInDevice (DeviceNode, Name, &Node);
  return !EFI_ERROR (Status);
}

BOOLEAN
AmlTestHelper::DeviceHasHid (
  IN  AML_NODE_HANDLE  DeviceNode,
  IN  CONST CHAR8      *HidValue
  )
{
  CHAR8       HidBuffer[16];
  EFI_STATUS  Status;

  Status = GetDeviceHid (DeviceNode, HidBuffer, sizeof (HidBuffer));
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return (AsciiStrCmp (HidBuffer, HidValue) == 0);
}

EFI_STATUS
AmlTestHelper::CountCpuDevices (
  IN  AML_ROOT_NODE_HANDLE  RootNode,
  IN  CONST CHAR8           *ScopePath,
  OUT UINT32                *Count
  )
{
  AML_NODE_HANDLE  ScopeNode;
  AML_NODE_HEADER  *Child;
  EFI_STATUS       Status;
  CHAR8            HidBuffer[16];

  if ((RootNode == NULL) || (ScopePath == NULL) || (Count == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Count = 0;

  Status = AmlFindNode ((AML_NODE_HANDLE)RootNode, ScopePath, &ScopeNode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Iterate through children looking for Device nodes with ACPI0007 HID
  Child = AmlGetNextVariableArgument ((AML_NODE_HEADER *)ScopeNode, NULL);
  while (Child != NULL) {
    if (IsDeviceNode ((AML_NODE_HANDLE)Child)) {
      Status = GetDeviceHid ((AML_NODE_HANDLE)Child, HidBuffer, sizeof (HidBuffer));
      if (!EFI_ERROR (Status)) {
        if (AsciiStrCmp (HidBuffer, ACPI_HID_PROCESSOR_DEVICE_STR) == 0) {
          (*Count)++;
        }
      }
    }

    Child = AmlGetNextVariableArgument ((AML_NODE_HEADER *)ScopeNode, Child);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
AmlTestHelper::GetDeviceName (
  IN  AML_NODE_HANDLE  DeviceNode,
  OUT CHAR8            *NameBuffer,
  IN  UINTN            BufferSize
  )
{
  AML_NODE_HEADER  *NameArg;

  if ((DeviceNode == NULL) || (NameBuffer == NULL) || (BufferSize < 5)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IS_AML_OBJECT_NODE (DeviceNode)) {
    return EFI_INVALID_PARAMETER;
  }

  // Device name is in fixed argument 0
  NameArg = AmlGetFixedArgument ((AML_OBJECT_NODE *)DeviceNode, EAmlParseIndexTerm0);
  if (!IS_AML_DATA_NODE (NameArg)) {
    return EFI_NOT_FOUND;
  }

  AML_DATA_NODE  *DataNode = (AML_DATA_NODE *)NameArg;

  if (DataNode->Size < 4) {
    return EFI_NOT_FOUND;
  }

  CopyMem (NameBuffer, DataNode->Buffer, 4);
  NameBuffer[4] = '\0';

  return EFI_SUCCESS;
}

VOID
AmlTestHelper::DeleteAmlTree (
  IN  AML_ROOT_NODE_HANDLE  RootNode
  )
{
  if (RootNode != NULL) {
    AmlDeleteTree ((AML_NODE_HANDLE)RootNode);
  }
}

// =============================================================================
// Package Node Operations
// =============================================================================

BOOLEAN
AmlTestHelper::IsPackageNode (
  IN  AML_NODE_HANDLE  Node
  )
{
  if (!IS_AML_OBJECT_NODE (Node)) {
    return FALSE;
  }

  // Package opcode is 0x12
  return AmlNodeCompareOpCode (
           (AML_OBJECT_NODE *)Node,
           AML_PACKAGE_OP,
           0
           );
}

BOOLEAN
AmlTestHelper::IsMethodNode (
  IN  AML_NODE_HANDLE  Node
  )
{
  if (!IS_AML_OBJECT_NODE (Node)) {
    return FALSE;
  }

  return AmlNodeCompareOpCode (
           (AML_OBJECT_NODE *)Node,
           AML_METHOD_OP,
           0
           );
}

EFI_STATUS
AmlTestHelper::GetPackageElementCount (
  IN  AML_NODE_HANDLE  PackageNode,
  OUT UINT32           *Count
  )
{
  AML_NODE_HEADER  *Child;

  if ((PackageNode == NULL) || (Count == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsPackageNode (PackageNode)) {
    return EFI_INVALID_PARAMETER;
  }

  *Count = 0;

  // Elements are variable arguments of the Package node
  Child = AmlGetNextVariableArgument ((AML_NODE_HEADER *)PackageNode, NULL);
  while (Child != NULL) {
    (*Count)++;
    Child = AmlGetNextVariableArgument ((AML_NODE_HEADER *)PackageNode, Child);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
AmlTestHelper::GetPackageElement (
  IN  AML_NODE_HANDLE  PackageNode,
  IN  UINT32           Index,
  OUT AML_NODE_HANDLE  *Element
  )
{
  AML_NODE_HEADER  *Child;
  UINT32           CurrentIndex;

  if ((PackageNode == NULL) || (Element == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsPackageNode (PackageNode)) {
    return EFI_INVALID_PARAMETER;
  }

  CurrentIndex = 0;
  Child        = AmlGetNextVariableArgument ((AML_NODE_HEADER *)PackageNode, NULL);
  while (Child != NULL) {
    if (CurrentIndex == Index) {
      *Element = (AML_NODE_HANDLE)Child;
      return EFI_SUCCESS;
    }

    CurrentIndex++;
    Child = AmlGetNextVariableArgument ((AML_NODE_HEADER *)PackageNode, Child);
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
AmlTestHelper::GetIntegerValue (
  IN  AML_NODE_HANDLE  Node,
  OUT UINT64           *Value
  )
{
  if ((Node == NULL) || (Value == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Handle data node containing raw integer bytes
  if (IS_AML_DATA_NODE (Node)) {
    AML_DATA_NODE  *DataNode = (AML_DATA_NODE *)Node;
    *Value = 0;
    switch (DataNode->Size) {
      case 1:
        *Value = *(UINT8 *)DataNode->Buffer;
        return EFI_SUCCESS;
      case 2:
        *Value = *(UINT16 *)DataNode->Buffer;
        return EFI_SUCCESS;
      case 4:
        *Value = *(UINT32 *)DataNode->Buffer;
        return EFI_SUCCESS;
      case 8:
        *Value = *(UINT64 *)DataNode->Buffer;
        return EFI_SUCCESS;
      default:
        return EFI_UNSUPPORTED;
    }
  }

  // Handle object node with integer opcodes
  if (IS_AML_OBJECT_NODE (Node)) {
    AML_OBJECT_NODE  *ObjNode = (AML_OBJECT_NODE *)Node;

    if (AmlNodeCompareOpCode (ObjNode, AML_ZERO_OP, 0)) {
      *Value = 0;
      return EFI_SUCCESS;
    }

    if (AmlNodeCompareOpCode (ObjNode, AML_ONE_OP, 0)) {
      *Value = 1;
      return EFI_SUCCESS;
    }

    if (AmlNodeCompareOpCode (ObjNode, AML_ONES_OP, 0)) {
      *Value = (UINT64)-1;
      return EFI_SUCCESS;
    }

    // BytePrefix, WordPrefix, DWordPrefix, QWordPrefix
    if (AmlNodeCompareOpCode (ObjNode, AML_BYTE_PREFIX, 0) ||
        AmlNodeCompareOpCode (ObjNode, AML_WORD_PREFIX, 0) ||
        AmlNodeCompareOpCode (ObjNode, AML_DWORD_PREFIX, 0) ||
        AmlNodeCompareOpCode (ObjNode, AML_QWORD_PREFIX, 0))
    {
      AML_NODE_HEADER  *IntArg = AmlGetFixedArgument (ObjNode, EAmlParseIndexTerm0);
      if (IS_AML_DATA_NODE (IntArg)) {
        AML_DATA_NODE  *IntData = (AML_DATA_NODE *)IntArg;
        *Value = 0;
        CopyMem (Value, IntData->Buffer, IntData->Size);
        return EFI_SUCCESS;
      }
    }
  }

  return EFI_UNSUPPORTED;
}

EFI_STATUS
AmlTestHelper::FindNamedObjectValue (
  IN  AML_NODE_HANDLE  DeviceNode,
  IN  CONST CHAR8      *Name,
  OUT AML_NODE_HANDLE  *ValueNode
  )
{
  EFI_STATUS       Status;
  AML_NODE_HANDLE  NameNode;
  AML_NODE_HEADER  *ValueArg;

  Status = FindNamedObjectInDevice (DeviceNode, Name, &NameNode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Get the value from fixed argument 1
  ValueArg = AmlGetFixedArgument ((AML_OBJECT_NODE *)NameNode, EAmlParseIndexTerm1);
  if (ValueArg == NULL) {
    return EFI_NOT_FOUND;
  }

  *ValueNode = (AML_NODE_HANDLE)ValueArg;
  return EFI_SUCCESS;
}

// =============================================================================
// _CPC Validation per ACPI 6.6 s8.4.7.1
// =============================================================================

EFI_STATUS
AmlTestHelper::ValidateCpcPackageStructure (
  IN  AML_NODE_HANDLE  DeviceNode,
  OUT UINT32           *EntryCount
  )
{
  EFI_STATUS       Status;
  AML_NODE_HANDLE  CpcValue;
  UINT32           Count;

  if ((DeviceNode == NULL) || (EntryCount == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Find _CPC named object
  Status = FindNamedObjectValue (DeviceNode, "_CPC", &CpcValue);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // _CPC value must be a Package
  if (!IsPackageNode (CpcValue)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetPackageElementCount (CpcValue, &Count);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *EntryCount = Count;
  return EFI_SUCCESS;
}

EFI_STATUS
AmlTestHelper::GetCpcRevision (
  IN  AML_NODE_HANDLE  DeviceNode,
  OUT UINT32           *Revision
  )
{
  EFI_STATUS       Status;
  AML_NODE_HANDLE  CpcValue;
  AML_NODE_HANDLE  RevElement;
  UINT64           RevValue;

  Status = FindNamedObjectValue (DeviceNode, "_CPC", &CpcValue);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Revision is at index 1 (index 0 is NumEntries)
  Status = GetPackageElement (CpcValue, 1, &RevElement);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetIntegerValue (RevElement, &RevValue);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Revision = (UINT32)RevValue;
  return EFI_SUCCESS;
}

// =============================================================================
// _PSD Validation per ACPI 6.6 s8.4.5.5
// =============================================================================

EFI_STATUS
AmlTestHelper::ValidatePsdPackageStructure (
  IN  AML_NODE_HANDLE  DeviceNode,
  OUT UINT32           *OuterCount
  )
{
  EFI_STATUS       Status;
  AML_NODE_HANDLE  PsdValue;
  UINT32           Count;

  if ((DeviceNode == NULL) || (OuterCount == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FindNamedObjectValue (DeviceNode, "_PSD", &PsdValue);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!IsPackageNode (PsdValue)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetPackageElementCount (PsdValue, &Count);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *OuterCount = Count;
  return EFI_SUCCESS;
}

EFI_STATUS
AmlTestHelper::GetPsdFields (
  IN  AML_NODE_HANDLE  DeviceNode,
  OUT UINT32           *NumEntries,
  OUT UINT32           *Revision,
  OUT UINT32           *Domain,
  OUT UINT32           *CoordType,
  OUT UINT32           *NumProcessors
  )
{
  EFI_STATUS       Status;
  AML_NODE_HANDLE  PsdValue;
  AML_NODE_HANDLE  InnerPackage;
  AML_NODE_HANDLE  Element;
  UINT64           Value;

  Status = FindNamedObjectValue (DeviceNode, "_PSD", &PsdValue);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Get the first (and only) inner package
  Status = GetPackageElement (PsdValue, 0, &InnerPackage);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!IsPackageNode (InnerPackage)) {
    return EFI_INVALID_PARAMETER;
  }

  // Extract each field
  Status = GetPackageElement (InnerPackage, 0, &Element);
  if (!EFI_ERROR (Status)) {
    GetIntegerValue (Element, &Value);
    *NumEntries = (UINT32)Value;
  }

  Status = GetPackageElement (InnerPackage, 1, &Element);
  if (!EFI_ERROR (Status)) {
    GetIntegerValue (Element, &Value);
    *Revision = (UINT32)Value;
  }

  Status = GetPackageElement (InnerPackage, 2, &Element);
  if (!EFI_ERROR (Status)) {
    GetIntegerValue (Element, &Value);
    *Domain = (UINT32)Value;
  }

  Status = GetPackageElement (InnerPackage, 3, &Element);
  if (!EFI_ERROR (Status)) {
    GetIntegerValue (Element, &Value);
    *CoordType = (UINT32)Value;
  }

  Status = GetPackageElement (InnerPackage, 4, &Element);
  if (!EFI_ERROR (Status)) {
    GetIntegerValue (Element, &Value);
    *NumProcessors = (UINT32)Value;
  }

  return EFI_SUCCESS;
}

// =============================================================================
// _LPI Validation per ACPI 6.6 s8.4.4
// =============================================================================

EFI_STATUS
AmlTestHelper::GetLpiHeader (
  IN  AML_NODE_HANDLE  DeviceNode,
  OUT UINT32           *Revision,
  OUT UINT32           *LevelId,
  OUT UINT32           *StateCount
  )
{
  EFI_STATUS       Status;
  AML_NODE_HANDLE  LpiValue;
  AML_NODE_HANDLE  Element;
  UINT64           Value;

  Status = FindNamedObjectValue (DeviceNode, "_LPI", &LpiValue);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!IsPackageNode (LpiValue)) {
    return EFI_INVALID_PARAMETER;
  }

  // Get Revision (index 0)
  Status = GetPackageElement (LpiValue, 0, &Element);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetIntegerValue (Element, &Value);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Revision = (UINT32)Value;

  // Get LevelId (index 1)
  Status = GetPackageElement (LpiValue, 1, &Element);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetIntegerValue (Element, &Value);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *LevelId = (UINT32)Value;

  // Get Count (index 2)
  Status = GetPackageElement (LpiValue, 2, &Element);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetIntegerValue (Element, &Value);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *StateCount = (UINT32)Value;

  return EFI_SUCCESS;
}

// =============================================================================
// _CST Validation per ACPI 6.6 s8.4.1.1
// =============================================================================

EFI_STATUS
AmlTestHelper::GetCstStateCount (
  IN  AML_NODE_HANDLE  DeviceNode,
  OUT UINT32           *StateCount
  )
{
  EFI_STATUS       Status;
  AML_NODE_HANDLE  CstValue;
  AML_NODE_HANDLE  CountElement;
  UINT64           Count;

  Status = FindNamedObjectValue (DeviceNode, "_CST", &CstValue);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!IsPackageNode (CstValue)) {
    return EFI_INVALID_PARAMETER;
  }

  // Count is at index 0
  Status = GetPackageElement (CstValue, 0, &CountElement);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetIntegerValue (CountElement, &Count);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *StateCount = (UINT32)Count;
  return EFI_SUCCESS;
}

EFI_STATUS
AmlTestHelper::GetCstStateEntry (
  IN  AML_NODE_HANDLE  DeviceNode,
  IN  UINT32           StateIndex,
  OUT UINT8            *Type,
  OUT UINT16           *Latency,
  OUT UINT32           *Power
  )
{
  EFI_STATUS       Status;
  AML_NODE_HANDLE  CstValue;
  AML_NODE_HANDLE  StatePackage;
  AML_NODE_HANDLE  Element;
  UINT64           Value;

  Status = FindNamedObjectValue (DeviceNode, "_CST", &CstValue);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // C-state entries start at index 1 (index 0 is Count)
  Status = GetPackageElement (CstValue, StateIndex + 1, &StatePackage);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!IsPackageNode (StatePackage)) {
    return EFI_INVALID_PARAMETER;
  }

  // Index 0 is Register (skip for now - it's a buffer)
  // Index 1 is Type
  Status = GetPackageElement (StatePackage, 1, &Element);
  if (!EFI_ERROR (Status)) {
    GetIntegerValue (Element, &Value);
    *Type = (UINT8)Value;
  }

  // Index 2 is Latency
  Status = GetPackageElement (StatePackage, 2, &Element);
  if (!EFI_ERROR (Status)) {
    GetIntegerValue (Element, &Value);
    *Latency = (UINT16)Value;
  }

  // Index 3 is Power
  Status = GetPackageElement (StatePackage, 3, &Element);
  if (!EFI_ERROR (Status)) {
    GetIntegerValue (Element, &Value);
    *Power = (UINT32)Value;
  }

  return EFI_SUCCESS;
}

// =============================================================================
// _CSD Validation per ACPI 6.6 s8.4.1.2
// =============================================================================

EFI_STATUS
AmlTestHelper::GetCsdFields (
  IN  AML_NODE_HANDLE  DeviceNode,
  OUT UINT32           *NumEntries,
  OUT UINT32           *Revision,
  OUT UINT32           *Domain,
  OUT UINT32           *CoordType,
  OUT UINT32           *NumProcessors,
  OUT UINT32           *Index
  )
{
  EFI_STATUS       Status;
  AML_NODE_HANDLE  CsdValue;
  AML_NODE_HANDLE  InnerPackage;
  AML_NODE_HANDLE  Element;
  UINT64           Value;

  Status = FindNamedObjectValue (DeviceNode, "_CSD", &CsdValue);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Get the first inner package
  Status = GetPackageElement (CsdValue, 0, &InnerPackage);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!IsPackageNode (InnerPackage)) {
    return EFI_INVALID_PARAMETER;
  }

  // Extract each field
  Status = GetPackageElement (InnerPackage, 0, &Element);
  if (!EFI_ERROR (Status)) {
    GetIntegerValue (Element, &Value);
    *NumEntries = (UINT32)Value;
  }

  Status = GetPackageElement (InnerPackage, 1, &Element);
  if (!EFI_ERROR (Status)) {
    GetIntegerValue (Element, &Value);
    *Revision = (UINT32)Value;
  }

  Status = GetPackageElement (InnerPackage, 2, &Element);
  if (!EFI_ERROR (Status)) {
    GetIntegerValue (Element, &Value);
    *Domain = (UINT32)Value;
  }

  Status = GetPackageElement (InnerPackage, 3, &Element);
  if (!EFI_ERROR (Status)) {
    GetIntegerValue (Element, &Value);
    *CoordType = (UINT32)Value;
  }

  Status = GetPackageElement (InnerPackage, 4, &Element);
  if (!EFI_ERROR (Status)) {
    GetIntegerValue (Element, &Value);
    *NumProcessors = (UINT32)Value;
  }

  Status = GetPackageElement (InnerPackage, 5, &Element);
  if (!EFI_ERROR (Status)) {
    GetIntegerValue (Element, &Value);
    *Index = (UINT32)Value;
  }

  return EFI_SUCCESS;
}

// =============================================================================
// _PSS Validation per ACPI 6.6 s8.4.5.2
// =============================================================================

EFI_STATUS
AmlTestHelper::GetPssStateCount (
  IN  AML_NODE_HANDLE  DeviceNode,
  OUT UINT32           *StateCount
  )
{
  EFI_STATUS       Status;
  AML_NODE_HANDLE  PssValue;
  UINT32           Count;

  Status = FindNamedObjectValue (DeviceNode, "_PSS", &PssValue);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!IsPackageNode (PssValue)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetPackageElementCount (PssValue, &Count);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *StateCount = Count;
  return EFI_SUCCESS;
}

EFI_STATUS
AmlTestHelper::GetPssStateEntry (
  IN  AML_NODE_HANDLE  DeviceNode,
  IN  UINT32           StateIndex,
  OUT UINT32           *CoreFreq,
  OUT UINT32           *Power,
  OUT UINT32           *Latency,
  OUT UINT32           *BusMasterLatency,
  OUT UINT32           *Control,
  OUT UINT32           *StatusVal
  )
{
  EFI_STATUS       Status;
  AML_NODE_HANDLE  PssValue;
  AML_NODE_HANDLE  StatePackage;
  AML_NODE_HANDLE  Element;
  UINT64           Value;

  Status = FindNamedObjectValue (DeviceNode, "_PSS", &PssValue);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetPackageElement (PssValue, StateIndex, &StatePackage);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!IsPackageNode (StatePackage)) {
    return EFI_INVALID_PARAMETER;
  }

  // Index 0: CoreFrequency
  Status = GetPackageElement (StatePackage, 0, &Element);
  if (!EFI_ERROR (Status)) {
    GetIntegerValue (Element, &Value);
    *CoreFreq = (UINT32)Value;
  }

  // Index 1: Power
  Status = GetPackageElement (StatePackage, 1, &Element);
  if (!EFI_ERROR (Status)) {
    GetIntegerValue (Element, &Value);
    *Power = (UINT32)Value;
  }

  // Index 2: Latency
  Status = GetPackageElement (StatePackage, 2, &Element);
  if (!EFI_ERROR (Status)) {
    GetIntegerValue (Element, &Value);
    *Latency = (UINT32)Value;
  }

  // Index 3: BusMasterLatency
  Status = GetPackageElement (StatePackage, 3, &Element);
  if (!EFI_ERROR (Status)) {
    GetIntegerValue (Element, &Value);
    *BusMasterLatency = (UINT32)Value;
  }

  // Index 4: Control
  Status = GetPackageElement (StatePackage, 4, &Element);
  if (!EFI_ERROR (Status)) {
    GetIntegerValue (Element, &Value);
    *Control = (UINT32)Value;
  }

  // Index 5: Status
  Status = GetPackageElement (StatePackage, 5, &Element);
  if (!EFI_ERROR (Status)) {
    GetIntegerValue (Element, &Value);
    *StatusVal = (UINT32)Value;
  }

  return EFI_SUCCESS;
}

// =============================================================================
// _PPC Validation per ACPI 6.6 s8.4.5.3
// =============================================================================

EFI_STATUS
AmlTestHelper::GetPpcValue (
  IN  AML_NODE_HANDLE  DeviceNode,
  OUT UINT32           *PpcValue
  )
{
  EFI_STATUS       Status;
  AML_NODE_HANDLE  PpcNode;
  UINT64           Value;

  Status = FindNamedObjectValue (DeviceNode, "_PPC", &PpcNode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetIntegerValue (PpcNode, &Value);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *PpcValue = (UINT32)Value;
  return EFI_SUCCESS;
}

// =============================================================================
// _STA Validation per ACPI 6.6 s6.3.7
// =============================================================================

BOOLEAN
AmlTestHelper::DeviceHasSta (
  IN  AML_NODE_HANDLE  DeviceNode
  )
{
  return DeviceHasNamedObject (DeviceNode, "_STA");
}

// =============================================================================
// Hierarchy Traversal Helpers
// =============================================================================

EFI_STATUS
AmlTestHelper::CountContainerDevices (
  IN  AML_ROOT_NODE_HANDLE  RootNode,
  IN  CONST CHAR8           *ScopePath,
  OUT UINT32                *Count
  )
{
  AML_NODE_HANDLE  ScopeNode;
  AML_NODE_HEADER  *Child;
  EFI_STATUS       Status;
  CHAR8            HidBuffer[16];

  if ((RootNode == NULL) || (ScopePath == NULL) || (Count == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Count = 0;

  Status = AmlFindNode ((AML_NODE_HANDLE)RootNode, ScopePath, &ScopeNode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Child = AmlGetNextVariableArgument ((AML_NODE_HEADER *)ScopeNode, NULL);
  while (Child != NULL) {
    if (IsDeviceNode ((AML_NODE_HANDLE)Child)) {
      Status = GetDeviceHid ((AML_NODE_HANDLE)Child, HidBuffer, sizeof (HidBuffer));
      if (!EFI_ERROR (Status)) {
        if (AsciiStrCmp (HidBuffer, ACPI_HID_PROCESSOR_CONTAINER_STR) == 0) {
          (*Count)++;
        }
      }
    }

    Child = AmlGetNextVariableArgument ((AML_NODE_HEADER *)ScopeNode, Child);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
AmlTestHelper::ForEachCpuDevice (
  IN  AML_ROOT_NODE_HANDLE  RootNode,
  IN  CONST CHAR8           *ScopePath,
  IN  CpuDeviceCallback     Callback,
  IN  VOID                  *Context
  )
{
  AML_NODE_HANDLE  ScopeNode;
  AML_NODE_HEADER  *Child;
  EFI_STATUS       Status;
  CHAR8            HidBuffer[16];
  UINT32           Index = 0;

  if ((RootNode == NULL) || (ScopePath == NULL) || (Callback == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlFindNode ((AML_NODE_HANDLE)RootNode, ScopePath, &ScopeNode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Child = AmlGetNextVariableArgument ((AML_NODE_HEADER *)ScopeNode, NULL);
  while (Child != NULL) {
    if (IsDeviceNode ((AML_NODE_HANDLE)Child)) {
      Status = GetDeviceHid ((AML_NODE_HANDLE)Child, HidBuffer, sizeof (HidBuffer));
      if (!EFI_ERROR (Status)) {
        if (AsciiStrCmp (HidBuffer, ACPI_HID_PROCESSOR_DEVICE_STR) == 0) {
          Callback ((AML_NODE_HANDLE)Child, Index, Context);
          Index++;
        }
      }
    }

    Child = AmlGetNextVariableArgument ((AML_NODE_HEADER *)ScopeNode, Child);
  }

  return EFI_SUCCESS;
}
