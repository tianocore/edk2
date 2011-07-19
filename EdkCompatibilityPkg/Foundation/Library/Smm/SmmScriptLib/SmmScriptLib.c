/** @file
  Thunk implmentation for SmmScriptLib.

  SmmScriptLib in Framework implementation is to save S3 Boot Script in SMM runtime.
  The thunk implementation for SmmScriptLib will ultilize the SmmSaveState Protocol to save SMM
  runtime s3 boot Script.  

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions
  of the BSD License which accompanies this distribution.  The
  full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include "SmmScriptLib.h"

//
// Define the SmmS3SaveState Protocol GUID. 
//
EFI_GUID mEfiS3SmmSaveStateProtocolGuid = EFI_S3_SMM_SAVE_STATE_PROTOCOL_GUID;
EFI_GUID mEfiSmmBase2ProtocolGuid = EFI_SMM_BASE2_PROTOCOL_GUID;
EFI_S3_SMM_SAVE_STATE_PROTOCOL  *mS3SmmSaveState = NULL;
EFI_SMM_SYSTEM_TABLE2           *gSmst              = NULL;

/**
  Internal function to add IO write opcode to the table.

  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enough resource to do operation.
  @retval EFI_SUCCESS           Opcode is added.

**/
EFI_STATUS
BootScriptIoWrite (
  IN VA_LIST                  Marker
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

  return mS3SmmSaveState->Write (
                        mS3SmmSaveState,
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
  
  return mS3SmmSaveState->Write (
                         mS3SmmSaveState,
                         EFI_BOOT_SCRIPT_IO_READ_WRITE_OPCODE,
                         Width, 
                         Address, 
                         Data, 
                         DataMask
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

  return mS3SmmSaveState->Write (
                        mS3SmmSaveState,
                        EFI_BOOT_SCRIPT_MEM_READ_WRITE_OPCODE,
                        Width, 
                        Address, 
                        Data, 
                        DataMask
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

  return mS3SmmSaveState->Write (
                          mS3SmmSaveState,
                          EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE_OPCODE,
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
  IN VA_LIST                     Marker
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
  
  return  mS3SmmSaveState->Write (
                        mS3SmmSaveState,
                        EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE,
                        Width, 
                        Address, 
                        Count, 
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

  return mS3SmmSaveState->Write (
                          mS3SmmSaveState,
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
  return mS3SmmSaveState->Write (
                          mS3SmmSaveState,
                          EFI_BOOT_SCRIPT_DISPATCH_OPCODE,
                          EntryPoint
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
  
  return mS3SmmSaveState->Write (
                        mS3SmmSaveState,
                        EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE,
                        Width, 
                        Address, 
                        Count, 
                        Buffer
                        ); 
}
/**
  Adds a record into a specified Framework boot script table.

  This function is used to store a boot script record into a given boot
  script table in SMM runtime. The parameter is the same with definitionin BootScriptSave Protocol.
  
  @param  ScriptTable           Pointer to the script table to write to. In the thunk implementation, this parameter is ignored 
                                since the boot script table is maintained by BootScriptLib.
  @param  Type                  Not used.
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
SmmBootScriptWrite (
  IN OUT   EFI_SMM_SCRIPT_TABLE        *ScriptTable,
  IN       UINTN                       Type,
  IN       UINT16                      OpCode,
  ...
  )
{
  EFI_STATUS  Status;
  VA_LIST     Marker;
  
  if (mS3SmmSaveState == NULL) {
    return EFI_UNSUPPORTED;
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
 
  case EFI_BOOT_SCRIPT_MEM_READ_WRITE_OPCODE:
    VA_START (Marker, OpCode);
    Status = BootScriptMemReadWrite (Marker);
    VA_END (Marker);
    break;
  
  case EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE_OPCODE:
    VA_START (Marker, OpCode);
    Status = BootScriptPciCfgReadWrite (Marker);
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


  case EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE:
    VA_START (Marker, OpCode);
    Status = BootScriptPciCfgWrite (Marker);
    VA_END (Marker);
    break;

  case EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE:
    VA_START (Marker, OpCode);
    Status = BootScriptMemWrite (Marker);
    VA_END (Marker);
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
    break;
  }

  return Status;
}
/**
  Intialize Boot Script table. 
  
  This function should be called in SMM mode. The Thunk implementation is try to 
  locate SmmSaveState protocol.
  
  @param  SystemTable         Pointer to the EFI sytem table
  @param  SmmScriptTablePages The expected ScriptTable page number
  @param  SmmScriptTableBase  The returned ScriptTable base address

  @retval EFI_OUT_OF_RESOURCES    No resource to do the initialization.
  @retval EFI_SUCCESS             Function has completed successfully.
  
**/
EFI_STATUS
EFIAPI
InitializeSmmScriptLib (
  IN  EFI_SYSTEM_TABLE     *SystemTable,
  IN  UINTN                SmmScriptTablePages,
  OUT EFI_PHYSICAL_ADDRESS *SmmScriptTableBase
  )
{
  EFI_STATUS        Status;
  BOOLEAN           InSmm;
  EFI_SMM_BASE2_PROTOCOL          *InternalSmmBase2;
  
  InternalSmmBase2  = NULL;
  //
  // The boot Script table is maintained by DxeBootScript Lib. so there is no need to
  // allocate the table here and directly return zero
  //
  if (SmmScriptTableBase == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *SmmScriptTableBase = 0;
  //
  // Retrieve SMM Base2 Protocol
  //
  Status = SystemTable->BootServices->LocateProtocol (
                  &mEfiSmmBase2ProtocolGuid,
                  NULL,
                  (VOID **) &InternalSmmBase2
                  );
  ASSERT_EFI_ERROR (Status);
  ASSERT (InternalSmmBase2 != NULL);

  //
  // Check to see if we are already in SMM
  //
  InternalSmmBase2->InSmm (InternalSmmBase2, &InSmm);

  if (!InSmm) {
    //
    // We are not in SMM, Directly return;
    //
    return EFI_SUCCESS;
  }

  //
  // We are in SMM, retrieve the pointer to SMM System Table
  //
  InternalSmmBase2->GetSmstLocation (InternalSmmBase2, &gSmst);

  ASSERT (gSmst != NULL);
  //
  // Locate Smm S3 Save State protocol to do the boot script save operation.
  //
  Status = gSmst->SmmLocateProtocol (
                                 &mEfiS3SmmSaveStateProtocolGuid,
                                  NULL,
                                 (VOID**)&mS3SmmSaveState
                                );
  return Status;
}
/**
  Create Boot Script table.
  
  It will be ignore and just return EFI_SUCCESS since the boot script table is 
  maintained by DxeBootScriptLib. Create Table is not needed.
  
  @param  ScriptTable     Pointer to the boot script table to create.
  @param  Type            The type of table to creat.

 
  @retval EFI_SUCCESS     Function has completed successfully.
  
**/
EFI_STATUS
EFIAPI
SmmBootScriptCreateTable (
  IN OUT   EFI_SMM_SCRIPT_TABLE        *ScriptTable,
  IN       UINTN                       Type
  )
{
  return EFI_SUCCESS;
}
/**
  Close Boot Script table. 
  
  It will be ignore and just return EFI_SUCCESS since the boot script table 
  is maintained by DxeBootScriptLib. 

  @param  ScriptTableBase    Pointer to the boot script table to create.
  @param  ScriptTablePtr     Pointer to the script table to write to.
  @param  Type               The type of table to creat.
    
  @retval EFI_SUCCESS        Function has completed successfully.
  
**/
EFI_STATUS
EFIAPI
SmmBootScriptCloseTable (
  IN EFI_SMM_SCRIPT_TABLE        ScriptTableBase,
  IN EFI_SMM_SCRIPT_TABLE        ScriptTablePtr,
  IN UINTN                       Type
  )
{
  return EFI_SUCCESS;
}
