/*++

Copyright (c) 2006, Intel Corporation. All rights reserved. 
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:
  
    EfiScriptLib.h

Abstract:

 
--*/

#ifndef _BOOT_SCRIPT_LIB_H_
#define _BOOT_SCRIPT_LIB_H_

#include <PiPei.h>
#include <Ppi/BootScriptExecuter.h>

#include <IndustryStandard/SmBus.h>

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

  Save I/O write to boot script with opcode EFI_BOOT_SCRIPT_IO_WRITE_OPCODE

Arguments:

  TableName - Desired boot script table

  Width   - The width of the I/O operations.
  
  Address - The base address of the I/O operations.
  
  Count   - The number of I/O operations to perform.
  
  Buffer  - The source buffer from which to write data. 

Returns: 
  
  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
;

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

  Save I/O modify to boot script with opcode EFI_BOOT_SCRIPT_IO_READ_WRITE_OPCODE

Arguments:

  TableName - Desired boot script table

  Width   - The width of the I/O operations.
  
  Address - The base address of the I/O operations.
  
  Data    - A pointer to the data to be OR-ed.
  
  DataMask  - A pointer to the data mask to be AND-ed with the data read from the register.

Returns: 
  
  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
;

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

  Save memory write to boot script with opcode EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE

Arguments:

  TableName - Desired boot script table

  Width   - The width of the memory operations.
  
  Address - The base address of the memory operations.
  
  Count   - The number of memory operations to perform.
  
  Buffer  - The source buffer from which to write the data. 

Returns: 
  
  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
;

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

  Save memory modify to boot script with opcode EFI_BOOT_SCRIPT_MEM_READ_WRITE_OPCODE

Arguments:

  TableName - Desired boot script table

  Width   - The width of the memory operations.
  
  Address - The base address of the memory operations.
  
  Data    - A pointer to the data to be OR-ed.
  
  DataMask  - A pointer to the data mask to be AND-ed with the data read from the register.

Returns: 
  
  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
;

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

  Save PCI configuration space write operation to boot script with opcode 
  EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE

Arguments:

  TableName - Desired boot script table

  Width   - The width of the PCI operations
  
  Address - The address within the PCI configuration space.
  
  Count   - The number of PCI operations to perform.
  
  Buffer  - The source buffer from which to write the data.

Returns: 
  
  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
;

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

  Save PCI configuration space modify operation to boot script with opcode 
  EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE_OPCODE

Arguments:

  TableName - Desired boot script table

  Width   - The width of the PCI operations
  
  Address - The address within the PCI configuration space.
  
  Data    - A pointer to the data to be OR-ed.
  
  DataMask  - A pointer to the data mask to be AND-ed with the data read from the register.

Returns: 
  
  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
;

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

  Save SMBus command execution to boot script with opcode 
  EFI_BOOT_SCRIPT_SMBUS_EXECUTE_OPCODE

Arguments:

  TableName     - Desired boot script table
  SlaveAddress  - The SMBus address for the slave device that the operation is targeting.
  Command       - The command that is transmitted by the SMBus host controller to the 
                  SMBus slave device.
  Operation     - Indicates which particular SMBus protocol it will use to execute the 
                  SMBus transactions.
  PecCheck      - Defines if Packet Error Code (PEC) checking is required for this operation.
  Length        - A pointer to signify the number of bytes that this operation will do.
  Buffer        - Contains the value of data to execute to the SMBUS slave device.

Returns:

  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
;

EFI_STATUS
EFIAPI
BootScriptSaveStall (
  IN  UINT16                            TableName,
  IN  UINTN                             Duration
  )
/*++

Routine Description:

  Save execution stall on the processor to boot script with opcode 
  EFI_BOOT_SCRIPT_STALL_OPCODE

Arguments:

  TableName     - Desired boot script table
  
  Duration      - Duration in microseconds of the stall.

Returns:

  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
;

EFI_STATUS
EFIAPI
BootScriptSaveDispatch (
  IN  UINT16                            TableName,
  IN  EFI_PHYSICAL_ADDRESS              EntryPoint
  )
/*++

Routine Description:

  Save dispatching specified arbitrary code to boot script with opcode 
  EFI_BOOT_SCRIPT_DISPATCH_OPCODE

Arguments:

  TableName     - Desired boot script table
  
  EntryPoint    - Entry point of the code to be dispatched.

Returns:

  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
;

EFI_STATUS
EFIAPI
BootScriptSaveDispatch2 (
  IN  UINT16                            TableName,
  IN  EFI_PHYSICAL_ADDRESS              EntryPoint,
  IN  EFI_PHYSICAL_ADDRESS              Context
  )
/*++

Routine Description:

  Save dispatching specified arbitrary code to boot script with opcode 
  EFI_BOOT_SCRIPT_DISPATCH_OPCODE

Arguments:

  TableName     - Desired boot script table
  
  EntryPoint    - Entry point of the code to be dispatched.

  Context       - The data that will be passed into code.

Returns:

  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
;

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
;

EFI_STATUS
EFIAPI
BootScriptSaveInformation (
  IN  UINT16                                 TableName,
  IN  UINT32                                 Length, 
  IN  EFI_PHYSICAL_ADDRESS                   Buffer
  )
  /*++

Routine Description:

  Save information specified by Buffer, length is specified by Length, to 
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
;

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
;

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
;

EFI_STATUS
EFIAPI
BootScriptSaveInitialize (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:

  Intialize Boot Script Lib if it has not yet been initialized. 

Arguments:

  ImageHandle     - The firmware allocated handle for the EFI image.
  
  SystemTable     - A pointer to the EFI System Table.

Returns: 

  EFI_STATUS always returns EFI_SUCCESS

--*/
;  
#endif


