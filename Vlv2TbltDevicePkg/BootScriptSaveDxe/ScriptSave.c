/** @file
  Implementation for S3 Boot Script Saver driver.

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

**/

#include "InternalBootScriptSave.h"

EFI_HANDLE                    mHandle = NULL;
EFI_BOOT_SCRIPT_SAVE_PROTOCOL mS3ScriptSave = {
  BootScriptWrite,
  BootScriptCloseTable
  };

/**
  Internal function to add IO write opcode to the table.

  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enough resource to do operation.
  @retval EFI_SUCCESS           Opcode is added.

**/
EFI_STATUS
BootScriptIoWrite (
  IN VA_LIST                       Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH Width;
  UINT64                Address;
  UINTN                 Count;
  UINT8                 *Buffer;

  Width       = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Address     = VA_ARG (Marker, UINT64);
  Count       = VA_ARG (Marker, UINTN);
  Buffer      = VA_ARG (Marker, UINT8 *);

  return S3BootScriptSaveIoWrite (Width, Address, Count, Buffer);
}

/**
  Internal function to add IO read/write opcode to the table.

  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enough resource to do operation.
  @retval EFI_SUCCESS           Opcode is added.

**/
EFI_STATUS
BootScriptIoReadWrite (
  IN VA_LIST                       Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH Width;
  UINT64                Address;
  UINT8                 *Data;
  UINT8                 *DataMask;

  Width       = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Address     = VA_ARG (Marker, UINT64);
  Data        = VA_ARG (Marker, UINT8 *);
  DataMask    = VA_ARG (Marker, UINT8 *);

  return S3BootScriptSaveIoReadWrite (Width, Address, Data, DataMask);
}

/**
  Internal function to add memory write opcode to the table.

  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enough resource to do operation.
  @retval EFI_SUCCESS           Opcode is added.

**/
EFI_STATUS
BootScriptMemWrite (
  IN VA_LIST                       Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH Width;
  UINT64                Address;
  UINTN                 Count;
  UINT8                 *Buffer;

  Width       = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Address     = VA_ARG (Marker, UINT64);
  Count       = VA_ARG (Marker, UINTN);
  Buffer      = VA_ARG (Marker, UINT8 *);

  return S3BootScriptSaveMemWrite (Width, Address, Count, Buffer);
}

/**
  Internal function to add memory read/write opcode to the table.

  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enough resource to do operation.
  @retval EFI_SUCCESS           Opcode is added.

**/
EFI_STATUS
BootScriptMemReadWrite (
  IN VA_LIST                       Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH Width;
  UINT64                Address;
  UINT8                 *Data;
  UINT8                 *DataMask;

  Width       = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Address     = VA_ARG (Marker, UINT64);
  Data        = VA_ARG (Marker, UINT8 *);
  DataMask    = VA_ARG (Marker, UINT8 *);

  return S3BootScriptSaveMemReadWrite (Width, Address, Data, DataMask);
}

/**
  Internal function to add PciCfg write opcode to the table.

  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enough resource to do operation.
  @retval EFI_SUCCESS           Opcode is added.

**/
EFI_STATUS
BootScriptPciCfgWrite (
  IN VA_LIST                       Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH Width;
  UINT64                Address;
  UINTN                 Count;
  UINT8                 *Buffer;

  Width       = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Address     = VA_ARG (Marker, UINT64);
  Count       = VA_ARG (Marker, UINTN);
  Buffer      = VA_ARG (Marker, UINT8 *);

  return S3BootScriptSavePciCfgWrite (Width, Address, Count, Buffer);
}

/**
  Internal function to PciCfg read/write opcode to the table.

  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enough resource to do operation.
  @retval EFI_SUCCESS           Opcode is added.

**/
EFI_STATUS
BootScriptPciCfgReadWrite (
  IN VA_LIST                       Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH Width;
  UINT64                Address;
  UINT8                 *Data;
  UINT8                 *DataMask;

  Width       = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Address     = VA_ARG (Marker, UINT64);
  Data        = VA_ARG (Marker, UINT8 *);
  DataMask    = VA_ARG (Marker, UINT8 *);

  return S3BootScriptSavePciCfgReadWrite (Width, Address, Data, DataMask);
}

/**
  Internal function to add PciCfg2 write opcode to the table.

  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enough resource to do operation.
  @retval EFI_SUCCESS           Opcode is added.

**/
EFI_STATUS
BootScriptPciCfg2Write (
  IN VA_LIST                       Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH Width;
  UINT64                Address;
  UINTN                 Count;
  UINT8                 *Buffer;
  UINT16                Segment;

  Width       = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Address     = VA_ARG (Marker, UINT64);
  Count       = VA_ARG (Marker, UINTN);
  Buffer      = VA_ARG (Marker, UINT8 *);
  Segment     = VA_ARG (Marker, UINT16);

  return S3BootScriptSavePciCfg2Write (Width, Segment, Address, Count, Buffer);
}

/**
  Internal function to PciCfg2 read/write opcode to the table.

  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enough resource to do operation.
  @retval EFI_SUCCESS           Opcode is added.

**/
EFI_STATUS
BootScriptPciCfg2ReadWrite (
  IN VA_LIST                       Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH Width;
  UINT16                Segment;
  UINT64                Address;
  UINT8                 *Data;
  UINT8                 *DataMask;

  Width       = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Address     = VA_ARG (Marker, UINT64);
  Segment     = VA_ARG (Marker, UINT16);
  Data        = VA_ARG (Marker, UINT8 *);
  DataMask    = VA_ARG (Marker, UINT8 *);

  return S3BootScriptSavePciCfg2ReadWrite (Width, Segment, Address, Data, DataMask);
}

/**
  Internal function to add smbus excute opcode to the table.

  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enough resource to do operation.
  @retval EFI_SUCCESS           Opcode is added.

**/
EFI_STATUS
BootScriptSmbusExecute (
  IN VA_LIST                       Marker
  )
{
  EFI_SMBUS_DEVICE_ADDRESS  SlaveAddress;
  EFI_SMBUS_DEVICE_COMMAND  Command;
  EFI_SMBUS_OPERATION       Operation;
  BOOLEAN                   PecCheck;
  VOID                     *Buffer;
  UINTN                    *DataSize;
  UINTN                     SmBusAddress;

  SlaveAddress.SmbusDeviceAddress = VA_ARG (Marker, UINTN);
  Command                         = VA_ARG (Marker, EFI_SMBUS_DEVICE_COMMAND);
  Operation                       = VA_ARG (Marker, EFI_SMBUS_OPERATION);
  PecCheck                        = VA_ARG (Marker, BOOLEAN);
  SmBusAddress                    = SMBUS_LIB_ADDRESS (SlaveAddress.SmbusDeviceAddress,Command,0,PecCheck);
  DataSize                        = VA_ARG (Marker, UINTN *);
  Buffer                          = VA_ARG (Marker, VOID *);

  return S3BootScriptSaveSmbusExecute (SmBusAddress, Operation, DataSize, Buffer);
}

/**
  Internal function to add stall opcode to the table.

  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enough resource to do operation.
  @retval EFI_SUCCESS           Opcode is added.

**/
EFI_STATUS
BootScriptStall (
  IN VA_LIST                       Marker
  )
{
  UINT32                Duration;

  Duration    = VA_ARG (Marker, UINT32);

  return S3BootScriptSaveStall (Duration);
}

/**
  Internal function to add Save jmp address according to DISPATCH_OPCODE.
  We ignore "Context" parameter.

  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enough resource to do operation.
  @retval EFI_SUCCESS           Opcode is added.

**/
EFI_STATUS
BootScriptDispatch (
  IN VA_LIST                       Marker
  )
{
  VOID        *EntryPoint;

  EntryPoint = (VOID*)(UINTN)VA_ARG (Marker, EFI_PHYSICAL_ADDRESS);
  return S3BootScriptSaveDispatch (EntryPoint);
}

/**
  Internal function to add memory pool operation to the table.

  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enough resource to do operation.
  @retval EFI_SUCCESS           Opcode is added.

**/
EFI_STATUS
BootScriptMemPoll (
  IN VA_LIST                       Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH Width;
  UINT64                Address;
  UINT8                 *BitMask;
  UINT8                 *BitValue;
  UINTN                Duration;
  UINTN                LoopTimes;

  Width       = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Address     = VA_ARG (Marker, UINT64);
  BitMask     = VA_ARG (Marker, UINT8 *);
  BitValue    = VA_ARG (Marker, UINT8 *);
  Duration    = (UINTN)VA_ARG (Marker, UINT64);
  LoopTimes   = (UINTN)VA_ARG (Marker, UINT64);

  return S3BootScriptSaveMemPoll (Width, Address, BitMask, BitValue, Duration, LoopTimes);
}

/**
  Internal function to add Save jmp address according to DISPATCH_OPCODE2.
  The "Context" parameter is not ignored.

  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enough resource to do operation.
  @retval EFI_SUCCESS           Opcode is added.

**/
EFI_STATUS
BootScriptDispatch2 (
  IN VA_LIST                       Marker
  )
{
  VOID                  *EntryPoint;
  VOID                  *Context;

  EntryPoint = (VOID*)(UINTN)VA_ARG (Marker, EFI_PHYSICAL_ADDRESS);
  Context    = (VOID*)(UINTN)VA_ARG (Marker, EFI_PHYSICAL_ADDRESS);

  return S3BootScriptSaveDispatch2 (EntryPoint, Context);
}

/**
  Internal function to add the opcode link node to the link list.

  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enought resource to complete the operations.
  @retval EFI_SUCCESS           The opcode entry is added to the link list
                                successfully.
**/
EFI_STATUS
BootScriptInformation (
  IN VA_LIST                       Marker
  )
{
  UINT32                InformationLength;
  EFI_PHYSICAL_ADDRESS  Information;

  InformationLength = VA_ARG (Marker, UINT32);
  Information = VA_ARG (Marker, EFI_PHYSICAL_ADDRESS);
  return S3BootScriptSaveInformation (InformationLength, (VOID*)(UINTN)Information);
}

/**
  Adds a record into a specified Framework boot script table.

  This function is used to store a boot script record into a given boot
  script table. If the table specified by TableName is nonexistent in the
  system, a new table will automatically be created and then the script record
  will be added into the new table. A boot script table can add new script records
  until EFI_BOOT_SCRIPT_SAVE_PROTOCOL.CloseTable() is called. Currently, the only
  meaningful table name is EFI_ACPI_S3_RESUME_SCRIPT_TABLE. This function is
  responsible for allocating necessary memory for the script.

  This function has a variable parameter list. The exact parameter list depends on
  the OpCode that is passed into the function. If an unsupported OpCode or illegal
  parameter list is passed in, this function returns EFI_INVALID_PARAMETER.
  If there are not enough resources available for storing more scripts, this function returns
  EFI_OUT_OF_RESOURCES.

  @param  This                  A pointer to the EFI_BOOT_SCRIPT_SAVE_PROTOCOL instance.
  @param  TableName             Name of the script table. Currently, the only meaningful value is
                                EFI_ACPI_S3_RESUME_SCRIPT_TABLE.
  @param  OpCode                The operation code (opcode) number.
  @param  ...                   Argument list that is specific to each opcode.

  @retval EFI_SUCCESS           The operation succeeded. A record was added into the
                                specified script table.
  @retval EFI_INVALID_PARAMETER The parameter is illegal or the given boot script is not supported.
                                If the opcode is unknow or not supported because of the PCD
                                Feature Flags.
  @retval EFI_OUT_OF_RESOURCES  There is insufficient memory to store the boot script.

**/
EFI_STATUS
EFIAPI
BootScriptWrite (
  IN EFI_BOOT_SCRIPT_SAVE_PROTOCOL    *This,
  IN UINT16                           TableName,
  IN UINT16                           OpCode,
  ...
  )
{
  EFI_STATUS                Status;
  VA_LIST                   Marker;

  if (TableName != FRAMEWORK_EFI_ACPI_S3_RESUME_SCRIPT_TABLE) {
    //
    // Only S3 boot script is supported for now.
    //
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Build script according to opcode.
  //
  switch (OpCode) {

  case EFI_BOOT_SCRIPT_IO_WRITE_OPCODE:
    VA_START (Marker, OpCode);
    Status = BootScriptIoWrite (Marker);
    VA_END (Marker);
    break;

  case EFI_BOOT_SCRIPT_IO_READ_WRITE_OPCODE:
    VA_START (Marker, OpCode);
    Status = BootScriptIoReadWrite (Marker);
    VA_END (Marker);
    break;

  case EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE:
    VA_START (Marker, OpCode);
    Status = BootScriptMemWrite (Marker);
    VA_END (Marker);
    break;

  case EFI_BOOT_SCRIPT_MEM_READ_WRITE_OPCODE:
    VA_START (Marker, OpCode);
    Status = BootScriptMemReadWrite (Marker);
    VA_END (Marker);
    break;

  case EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE:
    VA_START (Marker, OpCode);
    Status = BootScriptPciCfgWrite (Marker);
    VA_END (Marker);
    break;

  case EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE_OPCODE:
    VA_START (Marker, OpCode);
    Status = BootScriptPciCfgReadWrite (Marker);
    VA_END (Marker);
    break;

  case EFI_BOOT_SCRIPT_SMBUS_EXECUTE_OPCODE:
    VA_START (Marker, OpCode);
    Status = BootScriptSmbusExecute (Marker);
    VA_END (Marker);
    break;

  case EFI_BOOT_SCRIPT_STALL_OPCODE:
    VA_START (Marker, OpCode);
    Status = BootScriptStall (Marker);
    VA_END (Marker);

    break;

  case EFI_BOOT_SCRIPT_DISPATCH_OPCODE:
    VA_START (Marker, OpCode);
    Status = BootScriptDispatch (Marker);
    VA_END (Marker);
    break;

  case EFI_BOOT_SCRIPT_DISPATCH_2_OPCODE:
    VA_START (Marker, OpCode);
    Status = BootScriptDispatch2 (Marker);
    VA_END (Marker);
    break;

  case EFI_BOOT_SCRIPT_INFORMATION_OPCODE:
    VA_START (Marker, OpCode);
    Status = BootScriptInformation (Marker);
    VA_END (Marker);
    break;

  case EFI_BOOT_SCRIPT_MEM_POLL_OPCODE:
    VA_START (Marker, OpCode);
    Status = BootScriptMemPoll (Marker);
    VA_END (Marker);
    break;

  case EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE_OPCODE:
    VA_START (Marker, OpCode);
    Status = BootScriptPciCfg2Write (Marker);
    VA_END (Marker);
    break;

  case EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE_OPCODE:
    VA_START (Marker, OpCode);
    Status = BootScriptPciCfg2ReadWrite (Marker);
    VA_END (Marker);
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
    break;
  }

  return Status;
}

/**
  Closes the specified script table.

  This function closes the specified boot script table and returns the base address
  of the table. It allocates a new pool to duplicate all the boot scripts in the specified
  table. Once this function is called, the specified table will be destroyed after it is
  copied into the allocated pool. As a result, any attempts to add a script record into a
  closed table will cause a new table to be created. The base address of the allocated pool
  will be returned in Address. After using the boot script table, the caller is responsible
  for freeing the pool that is allocated by this function. If the boot script table,
  such as EFI_ACPI_S3_RESUME_SCRIPT_TABLE, is required to be stored in a nonperturbed
  memory region, the caller should copy the table into the nonperturbed memory region by itself.

  @param  This                  A pointer to the EFI_BOOT_SCRIPT_SAVE_PROTOCOL instance.
  @param  TableName             Name of the script table. Currently, the only meaningful value is
                                 EFI_ACPI_S3_RESUME_SCRIPT_TABLE.
  @param  Address               A pointer to the physical address where the table begins.

  @retval EFI_SUCCESS           The table was successfully returned.
  @retval EFI_NOT_FOUND         The specified table was not created previously.
  @retval EFI_OUT_OF_RESOURCE   Memory is insufficient to hold the reorganized boot script table.
  @retval EFI_UNSUPPORTED       The table type is not EFI_ACPI_S3_RESUME_SCRIPT_TABLE.

**/
EFI_STATUS
EFIAPI
BootScriptCloseTable (
  IN EFI_BOOT_SCRIPT_SAVE_PROTOCOL    *This,
  IN UINT16                           TableName,
  OUT EFI_PHYSICAL_ADDRESS            *Address
  )
{
  if (TableName != FRAMEWORK_EFI_ACPI_S3_RESUME_SCRIPT_TABLE) {
    //
    // Only S3 boot script is supported for now.
    //
    return EFI_NOT_FOUND;
  }
  *Address = (EFI_PHYSICAL_ADDRESS)(UINTN)S3BootScriptCloseTable ();

  if (*Address == 0) {
    return  EFI_NOT_FOUND;
  }
  return EFI_SUCCESS;
}

/**
  This routine is entry point of ScriptSave driver.

  @param  ImageHandle           Handle for this drivers loaded image protocol.
  @param  SystemTable           EFI system table.

  @retval EFI_OUT_OF_RESOURCES  No enough resource.
  @retval EFI_SUCCESS           Succesfully installed the ScriptSave driver.
  @retval other                 Errors occured.

**/
EFI_STATUS
EFIAPI
InitializeScriptSave (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  return  gBS->InstallProtocolInterface (
                  &mHandle,
                  &gEfiBootScriptSaveProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mS3ScriptSave
                  );

}

