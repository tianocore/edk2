/*++

Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    EfiScriptLib.h

Abstract:

 
--*/

#ifndef _EFI_SCRIPT_LIB_H_
#define _EFI_SCRIPT_LIB_H_

#include "Tiano.h"
#include "EfiCommonLib.h"
#include "EfiBootScript.h"
#include EFI_PROTOCOL_DEFINITION (BootScriptSave)


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
BootScriptMemPoll (
  IN  UINT16                            TableName,
  IN  EFI_BOOT_SCRIPT_WIDTH             Width,
  IN  UINT64                            Address,
  IN  VOID                              *BitMask,
  IN  VOID                              *BitValue,
  IN  UINTN                             Duration,
  IN  UINTN                             LoopTimes
  )
/*++

Routine Description:
  Polling one memory mapping register

Arguments:
  TableName - Desired boot script table

  Width     - The width of the memory operations.
  
  Address   - The base address of the memory operations.
  
  BitMask   - A pointer to the bit mask to be AND-ed with the data read from the register.

  BitValue  - A pointer to the data value after to be Masked.

  Duration  - Duration in microseconds of the stall.
  
  LoopTimes - The times of the register polling.

Returns:

  EFI_SUCCESS           - The operation was executed successfully

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

  Save a Information Opcode record in table specified with TableName

Arguments:

  TableName   - Desired boot script table
  Length         - Length of information in bytes
  Buffer          - Content of information that will be saved in script table

Returns:

  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
;

EFI_STATUS
EFIAPI
BootScriptSaveInformationUnicodeString (
  IN        UINT16              TableName,
  IN        CHAR16              *String
  )
/*++

Routine Description:

  Save a Information Opcode record in table specified with TableName, the information
  is a unicode string.

Arguments:

  TableName   - Desired boot script table
  String          - The string that will be saved in script table

Returns:

  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
;

EFI_STATUS
EFIAPI
BootScriptSaveInformationAsciiString (
  IN        UINT16              TableName,
  IN        CHAR8               *String
  )
/*++

Routine Description:

  Save a Information Opcode record in table specified with TableName, the information
  is a ascii string.

Arguments:

  TableName   - Desired boot script table
  String          - The string that will be saved in script table

Returns:

  EFI_NOT_FOUND - BootScriptSave Protocol not exist.
  
  EFI_STATUS - BootScriptSave Protocol exist, always returns EFI_SUCCESS

--*/
;
  
#ifdef EFI_S3_RESUME
  
#define INITIALIZE_SCRIPT(ImageHandle, SystemTable) \
          BootScriptSaveInitialize(ImageHandle, SystemTable)

#define SCRIPT_IO_WRITE(TableName, Width, Address, Count, Buffer) \
          BootScriptSaveIoWrite(TableName, Width, Address, Count, Buffer)

#define SCRIPT_IO_READ_WRITE(TableName, Width, Address, Data, DataMask) \
          BootScriptSaveIoReadWrite(TableName, Width, Address, Data, DataMask)

#define SCRIPT_MEM_WRITE(TableName, Width, Address, Count, Buffer) \
          BootScriptSaveMemWrite(TableName, Width, Address, Count, Buffer)

#define SCRIPT_MEM_WRITE_THIS(TableName, Width, Address, Count) \
          BootScriptSaveMemWrite(TableName, Width, Address, Count, (VOID*)(UINTN)Address)

#define SCRIPT_MEM_READ_WRITE(TableName, Width, Address, Data, DataMask) \
          BootScriptSaveMemReadWrite(TableName, Width, Address, Data, DataMask)

#define SCRIPT_PCI_CFG_WRITE(TableName, Width, Address, Count, Buffer) \
          BootScriptSavePciCfgWrite(TableName, Width, Address, Count, Buffer)

#define SCRIPT_PCI_CFG_READ_WRITE(TableName, Width, Address, Data, DataMask) \
          BootScriptSavePciCfgReadWrite(TableName, Width, Address, Data, DataMask)

#define SCRIPT_SMBUS_EXECUTE(TableName, SlaveAddress, Command, Operation, PecCheck, Length, Buffer) \
          BootScriptSaveSmbusExecute(TableName, SlaveAddress, Command, Operation, PecCheck, Length, Buffer)

#define SCRIPT_STALL(TableName, Duration) \
          BootScriptSaveStall(TableName, Duration)

#define SCRIPT_DISPATCH(TableName, EntryPoint) \
          BootScriptSaveDispatch(TableName, EntryPoint)

#define SCRIPT_MEM_POLL(TableName, Width, Address, BitMask, BitValue, Duration, LoopTimes) \
          BootScriptMemPoll(TableName, Width, Address, BitMask, BitValue, Duration, LoopTimes)

#define SCRIPT_INFORMATION(TableName, Length, Buffer) \
          BootScriptSaveInformation(TableName, Length, Buffer)

#define SCRIPT_INFORMATION_UNICODE_STRING(TableName, String) \
          BootScriptSaveInformationUnicodeString(TableName, String)

#define SCRIPT_INFORMATION_ASCII_STRING(TableName, String) \
          BootScriptSaveInformationAsciiString(TableName, String)

//
// For backward compatibility
//
#define SCRIPT_INOFRMATION(TableName, Length, Buffer) \
          BootScriptSaveInformation(TableName, Length, Buffer)

#define SCRIPT_INOFRMATION_UNICODE_STRING(TableName, String) \
          BootScriptSaveInformationUnicodeString(TableName, String)

#define SCRIPT_INOFRMATION_ASCII_STRING(TableName, String) \
          BootScriptSaveInformationAsciiString(TableName, String)
          
#else

#define INITIALIZE_SCRIPT(ImageHandle, SystemTable)          

#define SCRIPT_IO_WRITE(TableName, Width, Address, Count, Buffer) 

#define SCRIPT_IO_READ_WRITE(TableName, Width, Address, Data, DataMask) 
          
#define SCRIPT_MEM_WRITE(TableName, Width, Address, Count, Buffer) 

#define SCRIPT_MEM_WRITE_THIS(TableName, Width, Address, Count)

#define SCRIPT_MEM_READ_WRITE(TableName, Width, Address, Data, DataMask) 

#define SCRIPT_PCI_CFG_WRITE(TableName, Width, Address, Count, Buffer) 

#define SCRIPT_PCI_CFG_READ_WRITE(TableName, Width, Address, Data, DataMask) 

#define SCRIPT_SMBUS_EXECUTE(TableName, SlaveAddress, Command, Operation, PecCheck, Length, Buffer) 

#define SCRIPT_STALL(TableName, Duration)           

#define SCRIPT_DISPATCH(TableName, EntryPoint) 

#define SCRIPT_MEM_POLL(TableName, Width, Address, BitMask, BitValue, Duration, LoopTimes)

#define SCRIPT_INFORMATION(TableName, Length, Buffer)

#define SCRIPT_INFORMATION_UNICODE_STRING(TableName, String)

#define SCRIPT_INFORMATION_ASCII_STRING(TableName, String)

//
// For backward compatibility
//
#define SCRIPT_INOFRMATION(TableName, Length, Buffer)

#define SCRIPT_INOFRMATION_UNICODE_STRING(TableName, String)

#define SCRIPT_INOFRMATION_ASCII_STRING(TableName, String)

#endif

#endif
