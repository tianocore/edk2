/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:


    SmmScriptSave.c

Abstract:

    ScriptTableSave module at run time

--*/

#include "SmmScriptSave.h"

//
// internal functions
//

EFI_STATUS
BootScriptIoWrite  (
  IN EFI_SMM_SCRIPT_TABLE     *ScriptTable,
  IN VA_LIST                  Marker
  );

EFI_STATUS
BootScriptPciCfgWrite  (
  IN EFI_SMM_SCRIPT_TABLE     *ScriptTable,
  IN VA_LIST                  Marker
  );

VOID
SmmCopyMem (
  IN  UINT8    *Destination,
  IN  UINT8    *Source,
  IN  UINTN    ByteCount
  );

//
// Function implementations
//
EFI_STATUS
SmmBootScriptWrite (
  IN OUT EFI_SMM_SCRIPT_TABLE    *ScriptTable,
  IN UINTN                       Type,
  IN UINT16                      OpCode,
  ...
  )
{
  EFI_STATUS    Status;
  VA_LIST       Marker;

  if (ScriptTable == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Build script according to opcode
  //
  switch ( OpCode ) {

    case EFI_BOOT_SCRIPT_IO_WRITE_OPCODE:
      VA_START(Marker, OpCode);
      Status = BootScriptIoWrite (ScriptTable, Marker);
      VA_END(Marker);
      break;

    case EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE:
      VA_START(Marker, OpCode);
      Status = BootScriptPciCfgWrite(ScriptTable, Marker);
      VA_END(Marker);
      break;

    default:
      Status = EFI_SUCCESS;
      break;
  }

  return Status;
}


EFI_STATUS
SmmBootScriptCreateTable (
  IN OUT EFI_SMM_SCRIPT_TABLE    *ScriptTable,
  IN UINTN                       Type
  )
{
  BOOT_SCRIPT_POINTERS          Script;
  UINT8                         *Buffer;

  if (ScriptTable == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Buffer = (UINT8*) ((UINTN)(*ScriptTable));

  //
  // Fill Table Header
  //
  Script.Raw = Buffer;
  Script.TableInfo->OpCode      = EFI_BOOT_SCRIPT_TABLE_OPCODE;
  Script.TableInfo->Length      = sizeof(EFI_BOOT_SCRIPT_TABLE_HEADER);
  Script.TableInfo->TableLength = sizeof(EFI_BOOT_SCRIPT_TABLE_HEADER);

  //
  // Update current table pointer
  //
  *ScriptTable = *ScriptTable + sizeof(EFI_BOOT_SCRIPT_TABLE_HEADER);
  return EFI_SUCCESS;
}


EFI_STATUS
SmmBootScriptCloseTable (
  IN EFI_SMM_SCRIPT_TABLE        ScriptTableBase,
  IN EFI_SMM_SCRIPT_TABLE        ScriptTablePtr,
  IN UINTN                       Type
  )
{
  BOOT_SCRIPT_POINTERS    Script;

  //
  // Add final "termination" node to script table
  //
  Script.Raw               = (UINT8*) ((UINTN)ScriptTablePtr);
  Script.Terminate->OpCode = EFI_BOOT_SCRIPT_TERMINATE_OPCODE;
  Script.Terminate->Length = sizeof (EFI_BOOT_SCRIPT_TERMINATE);
  ScriptTablePtr          += sizeof (EFI_BOOT_SCRIPT_TERMINATE);


  //
  // Update Table Header
  //
  Script.Raw                    = (UINT8*) ((UINTN)ScriptTableBase);
  Script.TableInfo->OpCode      = EFI_BOOT_SCRIPT_TABLE_OPCODE;
  Script.TableInfo->Length      = sizeof (EFI_BOOT_SCRIPT_TABLE_HEADER);
  Script.TableInfo->TableLength = (UINT32)(ScriptTablePtr - ScriptTableBase);

  return EFI_SUCCESS;
}


EFI_STATUS
BootScriptIoWrite  (
  IN EFI_SMM_SCRIPT_TABLE     *ScriptTable,
  IN VA_LIST                  Marker
  )
{
  BOOT_SCRIPT_POINTERS    Script;
  EFI_BOOT_SCRIPT_WIDTH   Width;
  UINTN                   Address;
  UINTN                   Count;
  UINT8                   *Buffer;
  UINTN                   NodeLength;
  UINT8                   WidthInByte;

  Width     = VA_ARG(Marker, EFI_BOOT_SCRIPT_WIDTH);
  Address   = VA_ARG(Marker, UINTN);
  Count     = VA_ARG(Marker, UINTN);
  Buffer    = VA_ARG(Marker, UINT8*);

  WidthInByte = (UINT8)(0x01 << (Width & 0x03));
  Script.Raw  = (UINT8*) ((UINTN)(*ScriptTable));
  NodeLength  = sizeof (EFI_BOOT_SCRIPT_IO_WRITE) + (WidthInByte * Count);

  //
  // Build script data
  //
  Script.IoWrite->OpCode  = EFI_BOOT_SCRIPT_IO_WRITE_OPCODE;
  Script.IoWrite->Length  = (UINT8)(NodeLength);
  Script.IoWrite->Width   = Width;
  Script.IoWrite->Address = Address;
  Script.IoWrite->Count   = (UINT32)Count;
  SmmCopyMem (
    (UINT8*)(Script.Raw + sizeof (EFI_BOOT_SCRIPT_IO_WRITE)),
    Buffer,
    WidthInByte * Count
    );

  //
  // Update Script table pointer
  //
  *ScriptTable = *ScriptTable + NodeLength;
  return EFI_SUCCESS;
}


EFI_STATUS
BootScriptPciCfgWrite  (
  IN EFI_SMM_SCRIPT_TABLE        *ScriptTable,
  IN VA_LIST                     Marker
  )
{
  BOOT_SCRIPT_POINTERS    Script;
  EFI_BOOT_SCRIPT_WIDTH   Width;
  UINT64                  Address;
  UINTN                   Count;
  UINT8                   *Buffer;
  UINTN                   NodeLength;
  UINT8                   WidthInByte;

  Width     = VA_ARG(Marker, EFI_BOOT_SCRIPT_WIDTH);
  Address   = VA_ARG(Marker, UINT64);
  Count     = VA_ARG(Marker, UINTN);
  Buffer    = VA_ARG(Marker, UINT8*);

  WidthInByte = (UINT8)(0x01 << (Width & 0x03));
  Script.Raw  = (UINT8*) ((UINTN)(*ScriptTable));
  NodeLength  = sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE) + (WidthInByte * Count);

  //
  // Build script data
  //
  Script.PciWrite->OpCode  = EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE;
  Script.PciWrite->Length  = (UINT8)(NodeLength);
  Script.PciWrite->Width   = Width;
  Script.PciWrite->Address = Address;
  Script.PciWrite->Count   = (UINT32)Count;
  SmmCopyMem (
    (UINT8*)(Script.Raw + sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE)),
    Buffer,
    WidthInByte * Count
    );

  //
  // Update Script table pointer
  //
  *ScriptTable = *ScriptTable + NodeLength;
  return EFI_SUCCESS;
}

VOID
SmmCopyMem (
  IN  UINT8    *Destination,
  IN  UINT8    *Source,
  IN  UINTN    ByteCount
  )
{
  UINTN   Index;

  for (Index = 0; Index < ByteCount; Index++, Destination++, Source++) {
    *Destination = *Source;
  }
}
