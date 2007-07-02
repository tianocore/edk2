/*++

Copyright (c) 2007, Intel Corporation. All rights reserved. <BR> 
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.



Module Name:
  
    BootScriptLib.c

Abstract:

  Support for EFI script. 

--*/

//
// The package level header files this module uses
//
#include <PiDxe.h>
//
// The protocols, PPI and GUID defintions for this module
//
//
// The Library classes this module consumes
//
#include <Library/BootScriptLib.h>

EFI_STATUS
EFIAPI
BootScriptSaveIoWrite (
  IN  UINT16                            TableName,
  IN  EFI_BOOT_SCRIPT_WIDTH             Width,
  IN  UINT64                            Address,
  IN  UINTN                             Count,
  IN  VOID                              *Buffer
  )
/*++

Routine Description:

  Save I/O write to boot script 

Arguments:

  TableName - Desired boot script table

  (Standard EFI IO write script parameter) 

Returns: 
  
  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
// GC_TODO:    Width - add argument and description to function comment
// GC_TODO:    Address - add argument and description to function comment
// GC_TODO:    Count - add argument and description to function comment
// GC_TODO:    Buffer - add argument and description to function comment
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BootScriptSaveIoReadWrite (
  IN  UINT16                            TableName,
  IN  EFI_BOOT_SCRIPT_WIDTH             Width,
  IN  UINT64                            Address,
  IN  VOID                              *Data,
  IN  VOID                              *DataMask
  )
/*++

Routine Description:

  Save I/O write to boot script 

Arguments:

  TableName - Desired boot script table

  (Standard EFI IO read write script parameter) 

Returns: 
  
  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
// GC_TODO:    Width - add argument and description to function comment
// GC_TODO:    Address - add argument and description to function comment
// GC_TODO:    Data - add argument and description to function comment
// GC_TODO:    DataMask - add argument and description to function comment
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BootScriptSaveMemWrite (
  IN  UINT16                            TableName,
  IN  EFI_BOOT_SCRIPT_WIDTH             Width,
  IN  UINT64                            Address,
  IN  UINTN                             Count,
  IN  VOID                              *Buffer
  )
/*++

Routine Description:

  Save I/O write to boot script 

Arguments:

  TableName - Desired boot script table

  (Standard EFI MEM write script parameter) 

Returns: 
  
  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
// GC_TODO:    Width - add argument and description to function comment
// GC_TODO:    Address - add argument and description to function comment
// GC_TODO:    Count - add argument and description to function comment
// GC_TODO:    Buffer - add argument and description to function comment
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BootScriptSaveMemReadWrite (
  IN  UINT16                            TableName,
  IN  EFI_BOOT_SCRIPT_WIDTH             Width,
  IN  UINT64                            Address,
  IN  VOID                              *Data,
  IN  VOID                              *DataMask
  )
/*++

Routine Description:

  Save I/O write to boot script 

Arguments:

  TableName - Desired boot script table

  (Standard EFI MEM read write script parameter) 

Returns: 
  
  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
// GC_TODO:    Width - add argument and description to function comment
// GC_TODO:    Address - add argument and description to function comment
// GC_TODO:    Data - add argument and description to function comment
// GC_TODO:    DataMask - add argument and description to function comment
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BootScriptSavePciCfgWrite (
  IN  UINT16                            TableName,
  IN  EFI_BOOT_SCRIPT_WIDTH             Width,
  IN  UINT64                            Address,
  IN  UINTN                             Count,
  IN  VOID                              *Buffer
  )
/*++

Routine Description:

  Save I/O write to boot script 

Arguments:

  TableName - Desired boot script table

  (Standard EFI PCI write script parameter) 

Returns: 
  
  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
// GC_TODO:    Width - add argument and description to function comment
// GC_TODO:    Address - add argument and description to function comment
// GC_TODO:    Count - add argument and description to function comment
// GC_TODO:    Buffer - add argument and description to function comment
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BootScriptSavePciCfgReadWrite (
  IN  UINT16                            TableName,
  IN  EFI_BOOT_SCRIPT_WIDTH             Width,
  IN  UINT64                            Address,
  IN  VOID                              *Data,
  IN  VOID                              *DataMask
  )
/*++

Routine Description:

  Save I/O write to boot script 

Arguments:

  TableName - Desired boot script table

  (Standard EFI PCI read write script parameter) 

Returns: 
  
  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
// GC_TODO:    Width - add argument and description to function comment
// GC_TODO:    Address - add argument and description to function comment
// GC_TODO:    Data - add argument and description to function comment
// GC_TODO:    DataMask - add argument and description to function comment
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BootScriptSaveSmbusExecute (
  IN  UINT16                            TableName,
  IN  EFI_SMBUS_DEVICE_ADDRESS          SlaveAddress,
  IN  EFI_SMBUS_DEVICE_COMMAND          Command,
  IN  EFI_SMBUS_OPERATION               Operation,
  IN  BOOLEAN                           PecCheck,
  IN  UINTN                             *Length,
  IN  VOID                              *Buffer
  )
/*++

Routine Description:

  Save I/O write to boot script 

Arguments:

  TableName - Desired boot script table

  (Standard EFI Smbus execute script parameter) 

Returns: 
  
  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
// GC_TODO:    SlaveAddress - add argument and description to function comment
// GC_TODO:    Command - add argument and description to function comment
// GC_TODO:    Operation - add argument and description to function comment
// GC_TODO:    PecCheck - add argument and description to function comment
// GC_TODO:    Length - add argument and description to function comment
// GC_TODO:    Buffer - add argument and description to function comment
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BootScriptSaveStall (
  IN  UINT16                            TableName,
  IN  UINTN                             Duration
  )
/*++

Routine Description:

  Save I/O write to boot script 

Arguments:

  TableName - Desired boot script table

  (Standard EFI stall script parameter) 

Returns: 
  
  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
// GC_TODO:    Duration - add argument and description to function comment
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BootScriptSaveDispatch2 (
  IN  UINT16                            TableName,
  IN  EFI_PHYSICAL_ADDRESS              EntryPoint,
  IN  EFI_PHYSICAL_ADDRESS              Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  TableName   - GC_TODO: add argument description
  EntryPoint  - GC_TODO: add argument description

Returns:

  EFI_NOT_FOUND - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BootScriptSaveInformation (
  IN  UINT16                                 TableName,
  IN  UINT32                                 Length, 
  IN  EFI_PHYSICAL_ADDRESS                   Buffer
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  TableName   - GC_TODO: add argument description
  EntryPoint  - GC_TODO: add argument description

Returns:

  EFI_NOT_FOUND - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BootScriptSaveInformationUnicodeString (
  IN        UINT16              TableName,
  IN  CONST CHAR16              *String
  )
  /*++

Routine Description:

  Save unicode string information specified by Buffer to 
  boot script with opcode EFI_BOOT_SCRIPT_INFORMATION_OPCODE

Arguments:

  TableName     - Desired boot script table

  FfsName       - The file name of the code to be dispatched.

  Context       - The data that will be passed into code.

  ParentHandle  - The caller's image handle.

Returns:

  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BootScriptSaveInformationAsciiString (
  IN        UINT16              TableName,
  IN  CONST CHAR8               *String
  )
  /*++

Routine Description:

  Save ASCII string information specified by Buffer to 
  boot script with opcode EFI_BOOT_SCRIPT_INFORMATION_OPCODE

Arguments:

  TableName     - Desired boot script table

  FfsName       - The file name of the code to be dispatched.

  Context       - The data that will be passed into code.

  ParentHandle  - The caller's image handle.

Returns:

  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
{
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
BootScriptSaveDispatch (
  IN  UINT16                            TableName,
  IN  EFI_PHYSICAL_ADDRESS              EntryPoint
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  TableName   - GC_TODO: add argument description
  EntryPoint  - GC_TODO: add argument description

Returns:

  EFI_NOT_FOUND - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BootScriptSaveDispatch2Image (
  IN  UINT16                            TableName,
  IN  EFI_GUID                          *FfsName,
  IN  EFI_PHYSICAL_ADDRESS              Context,
  IN  EFI_HANDLE                        ParentHandle
  )
/*++

Routine Description:

  Save dispatching specified arbitrary code to boot script with opcode 
  EFI_BOOT_SCRIPT_DISPATCH_OPCODE

Arguments:

  TableName     - Desired boot script table

  FfsName       - The file name of the code to be dispatched.

  Context       - The data that will be passed into code.

  ParentHandle  - The caller's image handle.

Returns:

  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
{
  return EFI_SUCCESS;
}
