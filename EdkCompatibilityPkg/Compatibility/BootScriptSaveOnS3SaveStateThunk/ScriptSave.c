/** @file
  Implementation for S3 Boot Script Save thunk driver.
  This thunk driver consumes PI S3SaveState protocol to produce framework S3BootScriptSave Protocol 
  
  Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include "ScriptSave.h"

EFI_HANDLE                    mHandle;
EFI_BOOT_SCRIPT_SAVE_PROTOCOL mS3ScriptSave = {
                                  BootScriptWrite,
                                  BootScriptCloseTable
                                 };
EFI_S3_SAVE_STATE_PROTOCOL    *mS3SaveState;

/**
  Wrapper for a thunk  to transition from long mode to compatibility mode to execute 32-bit code and then transit back to
  long mode.
  
  @param  Function     The 32bit code entry to be executed.
  @param  Param1       The first parameter to pass to 32bit code
  @param  Param2       The second parameter to pass to 32bit code
  @retval EFI_SUCCESS  Execute 32bit code successfully.
  @retval other        Something wrong when execute the 32bit code 
              
**/  
EFI_STATUS
Execute32BitCode (
  IN UINT64      Function,
  IN UINT64      Param1,
  IN UINT64      Param2
  );

/**
  A stub to convert framework boot script dispatch to PI boot script dispatch.
  
  @param  ImageHandle  It should be is NULL.
  @param  Context      The first parameter to pass to 32bit code

  @return dispatch value.
              
**/  
EFI_STATUS
EFIAPI
FrameworkBootScriptDispatchStub (
  IN EFI_HANDLE ImageHandle,
  IN VOID       *Context
  )
{
  EFI_STATUS                Status;
  DISPATCH_ENTRYPOINT_FUNC  EntryFunc;
  VOID                      *PeiServices;
  IA32_DESCRIPTOR           Idtr;

  DEBUG ((EFI_D_ERROR, "FrameworkBootScriptDispatchStub - 0x%08x\n", (UINTN)Context));

  EntryFunc = (DISPATCH_ENTRYPOINT_FUNC) (UINTN) (Context);
  AsmReadIdtr (&Idtr);
  PeiServices = (VOID *)(UINTN)(*(UINT32 *)(Idtr.Base - sizeof (UINT32)));

  //
  // ECP assumes first parameter is NULL, and second parameter is PeiServices.
  //
  Status = Execute32BitCode ((UINT64)(UINTN)EntryFunc, 0, (UINT64)(UINTN)PeiServices);

  return Status;
}

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
  EFI_BOOT_SCRIPT_WIDTH Width;
  UINT64                Address;
  UINTN                 Count;
  UINT8                 *Buffer;

  Width       = VA_ARG (Marker, EFI_BOOT_SCRIPT_WIDTH);
  Address     = VA_ARG (Marker, UINT64);
  Count       = VA_ARG (Marker, UINTN);
  Buffer      = VA_ARG (Marker, UINT8 *);
  
  return mS3SaveState->Write (
                        mS3SaveState,
                        EFI_BOOT_SCRIPT_IO_WRITE_OPCODE,
                        Width, 
                        Address, 
                        Count, 
                        Buffer
                        );
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
  EFI_BOOT_SCRIPT_WIDTH Width;
  UINT64                Address;
  UINT8                 *Data;
  UINT8                 *DataMask;
 
  Width       = VA_ARG (Marker, EFI_BOOT_SCRIPT_WIDTH);
  Address     = VA_ARG (Marker, UINT64);
  Data        = VA_ARG (Marker, UINT8 *);
  DataMask    = VA_ARG (Marker, UINT8 *);
  
   return mS3SaveState->Write (
                         mS3SaveState,
                         EFI_BOOT_SCRIPT_IO_READ_WRITE_OPCODE,
                         Width, 
                         Address, 
                         Data, 
                         DataMask
                         );
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
  EFI_BOOT_SCRIPT_WIDTH Width;
  UINT64                Address;
  UINTN                 Count;
  UINT8                 *Buffer;
 
  Width       = VA_ARG (Marker, EFI_BOOT_SCRIPT_WIDTH);
  Address     = VA_ARG (Marker, UINT64);
  Count       = VA_ARG (Marker, UINTN);
  Buffer      = VA_ARG (Marker, UINT8 *);

  return mS3SaveState->Write (
                        mS3SaveState,
                        EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE,
                        Width, 
                        Address, 
                        Count, 
                        Buffer
                        );
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
  EFI_BOOT_SCRIPT_WIDTH Width;
  UINT64                Address;
  UINT8                 *Data;
  UINT8                 *DataMask;
  
  Width       = VA_ARG (Marker, EFI_BOOT_SCRIPT_WIDTH);
  Address     = VA_ARG (Marker, UINT64);
  Data        = VA_ARG (Marker, UINT8 *);
  DataMask    = VA_ARG (Marker, UINT8 *);

 return mS3SaveState->Write (
                        mS3SaveState,
                        EFI_BOOT_SCRIPT_MEM_READ_WRITE_OPCODE,
                        Width, 
                        Address, 
                        Data, 
                        DataMask
                        );
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
  EFI_BOOT_SCRIPT_WIDTH Width;
  UINT64                Address;
  UINTN                 Count;
  UINT8                 *Buffer;

  Width       = VA_ARG (Marker, EFI_BOOT_SCRIPT_WIDTH);
  Address     = VA_ARG (Marker, UINT64);
  Count       = VA_ARG (Marker, UINTN);
  Buffer      = VA_ARG (Marker, UINT8 *);

  return mS3SaveState->Write (
                        mS3SaveState,
                        EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE,
                        Width, 
                        Address, 
                        Count, 
                        Buffer
                        );
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
  EFI_BOOT_SCRIPT_WIDTH Width;
  UINT64                Address;
  UINT8                 *Data;
  UINT8                 *DataMask;

  Width       = VA_ARG (Marker, EFI_BOOT_SCRIPT_WIDTH);
  Address     = VA_ARG (Marker, UINT64);
  Data        = VA_ARG (Marker, UINT8 *);
  DataMask    = VA_ARG (Marker, UINT8 *);

  return mS3SaveState->Write (
                          mS3SaveState,
                          EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE_OPCODE,
                          Width,
                          Address,
                          Data,
                          DataMask
                         );
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
  EFI_BOOT_SCRIPT_WIDTH Width;
  UINT64                Address;
  UINTN                 Count;
  UINT8                 *Buffer;
  UINT16                Segment;

  Width       = VA_ARG (Marker, EFI_BOOT_SCRIPT_WIDTH);
  Address     = VA_ARG (Marker, UINT64);
  Count       = VA_ARG (Marker, UINTN);
  Buffer      = VA_ARG (Marker, UINT8 *);
  Segment     = VA_ARG (Marker, UINT16);

  return mS3SaveState->Write (
                          mS3SaveState,
                          EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE_OPCODE,
                          Width, 
                          Segment, 
                          Address, 
                          Count, 
                          Buffer
                         );
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
  EFI_BOOT_SCRIPT_WIDTH Width;
  UINT16                Segment;
  UINT64                Address;
  UINT8                 *Data;
  UINT8                 *DataMask;
 
  Width       = VA_ARG (Marker, EFI_BOOT_SCRIPT_WIDTH);
  Address     = VA_ARG (Marker, UINT64);
  Segment     = VA_ARG (Marker, UINT16);
  Data        = VA_ARG (Marker, UINT8 *);
  DataMask    = VA_ARG (Marker, UINT8 *);
 
  return mS3SaveState->Write (
                          mS3SaveState,
                          EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE_OPCODE,
                          Width, 
                          Segment,
                          Address,
                          Data,
                          DataMask
                          );
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
  
  SlaveAddress.SmbusDeviceAddress = VA_ARG (Marker, UINTN);
  Command                         = VA_ARG (Marker, EFI_SMBUS_DEVICE_COMMAND);
  Operation                       = VA_ARG (Marker, EFI_SMBUS_OPERATION);
  PecCheck                        = VA_ARG (Marker, BOOLEAN);
  DataSize                        = VA_ARG (Marker, UINTN *);    
  Buffer                          = VA_ARG (Marker, VOID *);
 
  return mS3SaveState->Write (
                          mS3SaveState,
                          EFI_BOOT_SCRIPT_SMBUS_EXECUTE_OPCODE,
                          SlaveAddress,
                          Command, 
                          Operation, 
                          PecCheck,
                          DataSize, 
                          Buffer
                         );
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

  return mS3SaveState->Write (
                          mS3SaveState,
                          EFI_BOOT_SCRIPT_STALL_OPCODE,
                          Duration
                         );
}

/**
  Internal function to add Save jmp address according to DISPATCH_OPCODE. 
  We ignore "Context" parameter

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
  return mS3SaveState->Write (
                          mS3SaveState,
                          EFI_BOOT_SCRIPT_DISPATCH_OPCODE,
                          EntryPoint
                         );
}

/**
  Internal function to add Save jmp address according to DISPATCH_OPCODE. 
  We ignore "Context" parameter.
  We need create thunk stub to convert PEI entrypoint (used in Framework version)
  to DXE entrypoint (defined in PI spec).

  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enough resource to do operation.
  @retval EFI_SUCCESS           Opcode is added.

**/
EFI_STATUS
FrameworkBootScriptDispatch (
  IN VA_LIST                       Marker
  )
{
  VOID           *EntryPoint;
  VOID           *Context;

  EntryPoint = (VOID*)(UINTN)VA_ARG (Marker, EFI_PHYSICAL_ADDRESS);

  //
  // Register callback
  //
  Context    = EntryPoint;
  EntryPoint = (VOID *)(UINTN)FrameworkBootScriptDispatchStub;
  return mS3SaveState->Write (
                         mS3SaveState,
                         EFI_BOOT_SCRIPT_DISPATCH_2_OPCODE,
                         EntryPoint,
                         Context
                         );
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
  EFI_BOOT_SCRIPT_WIDTH Width;
  UINT64                Address;
  UINT8                 *BitMask;
  UINT8                 *BitValue;
  UINT64                Duration;
  UINT64                LoopTimes;
  UINT64                Delay;

  Width       = VA_ARG (Marker, EFI_BOOT_SCRIPT_WIDTH);
  Address     = VA_ARG (Marker, UINT64);
  BitMask     = VA_ARG (Marker, UINT8 *);
  BitValue    = VA_ARG (Marker, UINT8 *);
  Duration    = (UINT64)VA_ARG (Marker, UINT64);
  LoopTimes   = (UINT64)VA_ARG (Marker, UINT64);
  //
  // Framework version: Duration is used for Stall(), which is Microseconds.
  //                    Total time is: Duration(Microseconds) * LoopTimes.
  // PI version:        Duration is always 100ns. Delay is LoopTimes.
  //                    Total time is: 100ns * Delay.
  // So Delay = Duration(Microseconds) * LoopTimes / 100ns
  //          = Duration * 1000ns * LoopTimes / 100ns
  //          = Duration * 10 * LoopTimes
  //
  Delay       = MultU64x64 (MultU64x32 (Duration, 10), LoopTimes);
  
  //
  // Framework version: First BitMask, then BitValue
  // PI version: First Data, then DataMask
  // So we revert their order in function call
  //
  return mS3SaveState->Write (
                          mS3SaveState,
                          EFI_BOOT_SCRIPT_MEM_POLL_OPCODE,
                          Width,
                          Address,
                          BitValue,
                          BitMask,
                          Delay
                          );
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

 return mS3SaveState->Write (
                          mS3SaveState,
                          EFI_BOOT_SCRIPT_DISPATCH_2_OPCODE,
                          EntryPoint, 
                          Context
                         );
}
/**
  Internal function to add the opcode link node to the link
  list.
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
  
  return mS3SaveState->Write (
                          mS3SaveState,
                          EFI_BOOT_SCRIPT_INFORMATION_OPCODE,
                          InformationLength, 
                          (VOID*)(UINTN)Information
                         );
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
    // Only S3 boot script is supported for now
    //
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Build script according to opcode
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
    Status = FrameworkBootScriptDispatch (Marker);
    VA_END (Marker);
    break;

  case FRAMEWORK_EFI_BOOT_SCRIPT_DISPATCH_2_OPCODE:
    VA_START (Marker, OpCode);
    Status = BootScriptDispatch2 (Marker);
    VA_END (Marker);
    break;

  case EFI_BOOT_SCRIPT_INFORMATION_OPCODE:
    VA_START (Marker, OpCode);
    Status = BootScriptInformation (Marker);
    VA_END (Marker);
    break;

  case FRAMEWORK_EFI_BOOT_SCRIPT_MEM_POLL_OPCODE:
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
  @retval EFI_UNSUPPORTED       the table type is not EFI_ACPI_S3_RESUME_SCRIPT_TABLE
  
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
    // Only S3 boot script is supported for now
    //
    return EFI_NOT_FOUND;
  }
  //
  // Here the close table is not implemented. 
  //  
 
  return EFI_UNSUPPORTED;
}

/**
  Register image to memory profile.

  @param FileName       File name of the image.
  @param ImageBase      Image base address.
  @param ImageSize      Image size.
  @param FileType       File type of the image.

**/
VOID
RegisterMemoryProfileImage (
  IN EFI_GUID                       *FileName,
  IN PHYSICAL_ADDRESS               ImageBase,
  IN UINT64                         ImageSize,
  IN EFI_FV_FILETYPE                FileType
  )
{
  EFI_STATUS                        Status;
  EDKII_MEMORY_PROFILE_PROTOCOL     *ProfileProtocol;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *FilePath;
  UINT8                             TempBuffer[sizeof (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH) + sizeof (EFI_DEVICE_PATH_PROTOCOL)];

  if ((PcdGet8 (PcdMemoryProfilePropertyMask) & BIT0) != 0) {

    FilePath = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)TempBuffer;
    Status = gBS->LocateProtocol (&gEdkiiMemoryProfileGuid, NULL, (VOID **) &ProfileProtocol);
    if (!EFI_ERROR (Status)) {
      EfiInitializeFwVolDevicepathNode (FilePath, FileName);
      SetDevicePathEndNode (FilePath + 1);

      Status = ProfileProtocol->RegisterImage (
                                  ProfileProtocol,
                                  (EFI_DEVICE_PATH_PROTOCOL *) FilePath,
                                  ImageBase,
                                  ImageSize,
                                  FileType
                                  );
    }
  }
}

/**
  This routine is entry point of ScriptSave driver.

  @param  ImageHandle           Handle for this drivers loaded image protocol.
  @param  SystemTable           EFI system table.

  @retval EFI_OUT_OF_RESOURCES  No enough resource
  @retval EFI_SUCCESS           Succesfully installed the ScriptSave driver.
  @retval other                 Errors occured.

**/
EFI_STATUS
EFIAPI
InitializeScriptSaveOnS3SaveState (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  UINT8                                         *Buffer;
  UINTN                                         BufferSize;
  PE_COFF_LOADER_IMAGE_CONTEXT                  ImageContext;
  BOOT_SCRIPT_THUNK_DATA                        *BootScriptThunkData;
  EFI_STATUS                                    Status;
  VOID                                          *DevicePath;
  EFI_PHYSICAL_ADDRESS                          MemoryAddress;
  UINTN                                         PageNumber;
  EFI_HANDLE                                    NewImageHandle;

  //
  // Test if the gEfiCallerIdGuid of this image is already installed. if not, the entry
  // point is loaded by DXE code which is the first time loaded. or else, it is already
  // be reloaded be itself.This is a work-around
  //
  Status = gBS->LocateProtocol (&gEfiCallerIdGuid, NULL, &DevicePath);
  if (EFI_ERROR (Status)) {
    //
    // This is the first-time loaded by DXE core. reload itself to RESERVED mem
    //
    //
    // A workaround: Here we install a dummy handle
    //
    NewImageHandle = NULL;
    Status = gBS->InstallProtocolInterface (
                    &NewImageHandle,
                    &gEfiCallerIdGuid,
                    EFI_NATIVE_INTERFACE,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);

    Status = GetSectionFromAnyFv  (
               &gEfiCallerIdGuid,
               EFI_SECTION_PE32,
               0,
               (VOID **) &Buffer,
               &BufferSize
               );
    ASSERT_EFI_ERROR (Status);
    ImageContext.Handle    = Buffer;
    ImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;
    //
    // Get information about the image being loaded
    //
    Status = PeCoffLoaderGetImageInfo (&ImageContext);
    ASSERT_EFI_ERROR (Status);

    MemoryAddress = SIZE_4GB - 1;
    if (ImageContext.SectionAlignment > EFI_PAGE_SIZE) {
      PageNumber = EFI_SIZE_TO_PAGES ((UINTN) (ImageContext.ImageSize + ImageContext.SectionAlignment));
    } else {
      PageNumber = EFI_SIZE_TO_PAGES ((UINTN) ImageContext.ImageSize);
    }
    Status  = gBS->AllocatePages (
                     AllocateMaxAddress,
                     EfiReservedMemoryType,
                     PageNumber,
                     &MemoryAddress
                     );
    ASSERT_EFI_ERROR (Status);
    ImageContext.ImageAddress = (PHYSICAL_ADDRESS)(UINTN)MemoryAddress;
    //
    // Align buffer on section boundry
    //
    ImageContext.ImageAddress += ImageContext.SectionAlignment - 1;
    ImageContext.ImageAddress &= ~(ImageContext.SectionAlignment - 1);
    //
    // Load the image to our new buffer
    //
    Status = PeCoffLoaderLoadImage (&ImageContext);
    ASSERT_EFI_ERROR (Status);

    //
    // Relocate the image in our new buffer
    //
    Status = PeCoffLoaderRelocateImage (&ImageContext);
    ASSERT_EFI_ERROR (Status);

    //
    // Free the buffer allocated by ReadSection since the image has been relocated in the new buffer
    //
    gBS->FreePool (Buffer);

    //
    // Flush the instruction cache so the image data is written before we execute it
    //
    InvalidateInstructionCacheRange ((VOID *)(UINTN)ImageContext.ImageAddress, (UINTN)ImageContext.ImageSize);

    RegisterMemoryProfileImage (
      &gEfiCallerIdGuid,
      ImageContext.ImageAddress,
      ImageContext.ImageSize,
      EFI_FV_FILETYPE_DRIVER
    );

    Status = ((EFI_IMAGE_ENTRY_POINT)(UINTN)(ImageContext.EntryPoint)) (NewImageHandle, SystemTable);
    ASSERT_EFI_ERROR (Status);

    //
    // Additional step for BootScriptThunk integrity
    //

    //
    // Allocate BootScriptThunkData
    //
    BootScriptThunkData = AllocatePool (sizeof (BOOT_SCRIPT_THUNK_DATA));
    ASSERT (BootScriptThunkData != NULL);

    BootScriptThunkData->BootScriptThunkBase   = ImageContext.ImageAddress;
    BootScriptThunkData->BootScriptThunkLength = ImageContext.ImageSize;
    //
    // Set BootScriptThunkData
    //
    PcdSet64 (BootScriptThunkDataPtr, (UINT64)(UINTN)BootScriptThunkData); 
    return EFI_SUCCESS;
  } else {
    //
    // the entry point is invoked after reloading. following code only run in RESERVED mem
    //

    //
    // Locate and cache PI S3 Save State Protocol.
    //
    Status = gBS->LocateProtocol (
                    &gEfiS3SaveStateProtocolGuid, 
                    NULL, 
                    (VOID **) &mS3SaveState
                    );
    ASSERT_EFI_ERROR (Status);

    return gBS->InstallProtocolInterface (
                  &mHandle,
                  &gEfiBootScriptSaveProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mS3ScriptSave
                  );
  }
}


