/** @file
  Implementation for S3 Boot Script Saver state driver.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "InternalS3SaveState.h"

EFI_HANDLE                  mHandle      = NULL;
EFI_S3_SAVE_STATE_PROTOCOL  mS3SaveState = {
  BootScriptWrite,
  BootScriptInsert,
  BootScriptLabel,
  BootScriptCompare
};

/**
  Internal function to add IO write opcode to the table.

  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enough resource to do operation.
  @retval EFI_SUCCESS           Opcode is added.

**/
EFI_STATUS
BootScriptWriteIoWrite (
  IN VA_LIST  Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH  Width;
  UINT64                    Address;
  UINTN                     Count;
  UINT8                     *Buffer;

  Width   = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Address = VA_ARG (Marker, UINT64);
  Count   = VA_ARG (Marker, UINTN);
  Buffer  = VA_ARG (Marker, UINT8 *);

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
BootScriptWriteIoReadWrite (
  IN VA_LIST  Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH  Width;
  UINT64                    Address;
  UINT8                     *Data;
  UINT8                     *DataMask;

  Width    = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Address  = VA_ARG (Marker, UINT64);
  Data     = VA_ARG (Marker, UINT8 *);
  DataMask = VA_ARG (Marker, UINT8 *);

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
BootScriptWriteMemWrite (
  IN VA_LIST  Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH  Width;
  UINT64                    Address;
  UINTN                     Count;
  UINT8                     *Buffer;

  Width   = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Address = VA_ARG (Marker, UINT64);
  Count   = VA_ARG (Marker, UINTN);
  Buffer  = VA_ARG (Marker, UINT8 *);

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
BootScriptWriteMemReadWrite (
  IN VA_LIST  Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH  Width;
  UINT64                    Address;
  UINT8                     *Data;
  UINT8                     *DataMask;

  Width    = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Address  = VA_ARG (Marker, UINT64);
  Data     = VA_ARG (Marker, UINT8 *);
  DataMask = VA_ARG (Marker, UINT8 *);

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
BootScriptWritePciCfgWrite (
  IN VA_LIST  Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH  Width;
  UINT64                    Address;
  UINTN                     Count;
  UINT8                     *Buffer;

  Width   = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Address = VA_ARG (Marker, UINT64);
  Count   = VA_ARG (Marker, UINTN);
  Buffer  = VA_ARG (Marker, UINT8 *);

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
BootScriptWritePciCfgReadWrite (
  IN VA_LIST  Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH  Width;
  UINT64                    Address;
  UINT8                     *Data;
  UINT8                     *DataMask;

  Width    = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Address  = VA_ARG (Marker, UINT64);
  Data     = VA_ARG (Marker, UINT8 *);
  DataMask = VA_ARG (Marker, UINT8 *);

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
BootScriptWritePciCfg2Write (
  IN VA_LIST  Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH  Width;
  UINT64                    Address;
  UINTN                     Count;
  UINT8                     *Buffer;
  UINT16                    Segment;

  Width   = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Segment = VA_ARG (Marker, UINT16);
  Address = VA_ARG (Marker, UINT64);
  Count   = VA_ARG (Marker, UINTN);
  Buffer  = VA_ARG (Marker, UINT8 *);

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
BootScriptWritePciCfg2ReadWrite (
  IN VA_LIST  Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH  Width;
  UINT16                    Segment;
  UINT64                    Address;
  UINT8                     *Data;
  UINT8                     *DataMask;

  Width    = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Segment  = VA_ARG (Marker, UINT16);
  Address  = VA_ARG (Marker, UINT64);
  Data     = VA_ARG (Marker, UINT8 *);
  DataMask = VA_ARG (Marker, UINT8 *);

  return S3BootScriptSavePciCfg2ReadWrite (Width, Segment, Address, Data, DataMask);
}

/**
  Internal function to add smbus execute opcode to the table.

  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enough resource to do operation.
  @retval EFI_SUCCESS           Opcode is added.

**/
EFI_STATUS
BootScriptWriteSmbusExecute (
  IN VA_LIST  Marker
  )
{
  EFI_SMBUS_DEVICE_ADDRESS  SlaveAddress;
  EFI_SMBUS_DEVICE_COMMAND  Command;
  EFI_SMBUS_OPERATION       Operation;
  BOOLEAN                   PecCheck;
  VOID                      *Buffer;
  UINTN                     *DataSize;
  UINTN                     SmBusAddress;

  SlaveAddress.SmbusDeviceAddress = VA_ARG (Marker, UINTN);
  Command                         = VA_ARG (Marker, EFI_SMBUS_DEVICE_COMMAND);
  Operation                       = VA_ARG (Marker, EFI_SMBUS_OPERATION);
  PecCheck                        = VA_ARG (Marker, BOOLEAN);
  SmBusAddress                    = SMBUS_LIB_ADDRESS (SlaveAddress.SmbusDeviceAddress, Command, 0, PecCheck);
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
BootScriptWriteStall (
  IN VA_LIST  Marker
  )
{
  UINT32  Duration;

  Duration = VA_ARG (Marker, UINT32);

  return S3BootScriptSaveStall (Duration);
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
BootScriptWriteDispatch (
  IN VA_LIST  Marker
  )
{
  VOID  *EntryPoint;

  EntryPoint = (VOID *)(UINTN)VA_ARG (Marker, EFI_PHYSICAL_ADDRESS);
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
BootScriptWriteMemPoll (
  IN VA_LIST  Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH  Width;
  UINT64                    Address;
  VOID                      *Data;
  VOID                      *DataMask;
  UINT64                    Delay;
  UINT64                    LoopTimes;
  UINT32                    Remainder;

  Width    = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Address  = VA_ARG (Marker, UINT64);
  Data     = VA_ARG (Marker, VOID *);
  DataMask = VA_ARG (Marker, VOID *);
  Delay    = VA_ARG (Marker, UINT64);
  //
  // According to the spec, the interval between 2 polls is 100ns,
  // but the unit of Duration for S3BootScriptSaveMemPoll() is microsecond(1000ns).
  // Duration * 1000ns * LoopTimes = Delay * 100ns
  // Duration will be minimum 1(microsecond) to be minimum deviation,
  // so LoopTimes = Delay / 10.
  //
  LoopTimes = DivU64x32Remainder (
                Delay,
                10,
                &Remainder
                );
  if (Remainder != 0) {
    //
    // If Remainder is not zero, LoopTimes will be rounded up by 1.
    //
    LoopTimes += 1;
  }

  return S3BootScriptSaveMemPoll (Width, Address, DataMask, Data, 1, LoopTimes);
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
BootScriptWriteDispatch2 (
  IN VA_LIST  Marker
  )
{
  VOID  *EntryPoint;
  VOID  *Context;

  EntryPoint = (VOID *)(UINTN)VA_ARG (Marker, EFI_PHYSICAL_ADDRESS);
  Context    = (VOID *)(UINTN)VA_ARG (Marker, EFI_PHYSICAL_ADDRESS);

  return S3BootScriptSaveDispatch2 (EntryPoint, Context);
}

/**
  Internal function to add INFORAMTION opcode node to the table
  list.
  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enought resource to complete the operations.
  @retval EFI_SUCCESS           The opcode entry is added to the  table
                                successfully.
**/
EFI_STATUS
BootScriptWriteInformation (
  IN VA_LIST  Marker
  )
{
  UINT32                InformationLength;
  EFI_PHYSICAL_ADDRESS  Information;

  InformationLength = VA_ARG (Marker, UINT32);
  Information       = VA_ARG (Marker, EFI_PHYSICAL_ADDRESS);
  return S3BootScriptSaveInformation (InformationLength, (VOID *)(UINTN)Information);
}

/**
  Internal function to add IO poll opcode node  to the table
  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enought resource to complete the operations.
  @retval EFI_SUCCESS           The opcode entry is added to the  table
                                successfully.
**/
EFI_STATUS
BootScriptWriteIoPoll (
  IN VA_LIST  Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH  Width;
  UINT64                    Address;
  VOID                      *Data;
  VOID                      *DataMask;
  UINT64                    Delay;

  Width    = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Address  = VA_ARG (Marker, UINT64);
  Data     = VA_ARG (Marker, VOID *);
  DataMask = VA_ARG (Marker, VOID *);
  Delay    = (UINT64)VA_ARG (Marker, UINT64);

  return S3BootScriptSaveIoPoll (Width, Address, Data, DataMask, Delay);
}

/**
  Internal function to add PCI config poll opcode node to the table

  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enought resource to complete the operations.
  @retval EFI_SUCCESS           The opcode entry is added to the  table
                                successfully.
**/
EFI_STATUS
BootScriptWritePciConfigPoll (
  IN VA_LIST  Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH  Width;
  UINT64                    Address;
  VOID                      *Data;
  VOID                      *DataMask;
  UINT64                    Delay;

  Width    = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Address  = VA_ARG (Marker, UINT64);
  Data     = VA_ARG (Marker, VOID *);
  DataMask = VA_ARG (Marker, VOID *);
  Delay    = (UINT64)VA_ARG (Marker, UINT64);

  return S3BootScriptSavePciPoll (Width, Address, Data, DataMask, Delay);
}

/**
  Internal function to add PCI config 2 poll opcode node to the table

  @param  Marker                The variable argument list to get the opcode
                                and associated attributes.

  @retval EFI_OUT_OF_RESOURCES  Not enought resource to complete the operations.
  @retval EFI_SUCCESS           The opcode entry is added to the  table
                                successfully.
**/
EFI_STATUS
BootScriptWritePciConfig2Poll (
  IN VA_LIST  Marker
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH  Width;
  UINT16                    Segment;
  UINT64                    Address;
  VOID                      *Data;
  VOID                      *DataMask;
  UINT64                    Delay;

  Width    = VA_ARG (Marker, S3_BOOT_SCRIPT_LIB_WIDTH);
  Segment  = VA_ARG (Marker, UINT16);
  Address  = VA_ARG (Marker, UINT64);
  Data     = VA_ARG (Marker, VOID *);
  DataMask = VA_ARG (Marker, VOID *);
  Delay    = (UINT64)VA_ARG (Marker, UINT64);

  return S3BootScriptSavePci2Poll (Width, Segment, Address, Data, DataMask, Delay);
}

/**
  Adds a record into S3 boot script table.

  This function is used to store a boot script record into a given boot
  script table. If the table specified by TableName is nonexistent in the
  system, a new table will automatically be created and then the script record
  will be added into the new table. This function is responsible for allocating
  necessary memory for the script.

  This function has a variable parameter list. The exact parameter list depends on
  the OpCode that is passed into the function. If an unsupported OpCode or illegal
  parameter list is passed in, this function returns EFI_INVALID_PARAMETER.
  If there are not enough resources available for storing more scripts, this function returns
  EFI_OUT_OF_RESOURCES.

  @param  This                  A pointer to the EFI_S3_SAVE_STATE_PROTOCOL instance.
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
  IN CONST EFI_S3_SAVE_STATE_PROTOCOL  *This,
  IN       UINTN                       OpCode,
  ...
  )
{
  EFI_STATUS  Status;
  VA_LIST     Marker;

  //
  // Build script according to opcode
  //
  switch (OpCode) {
    case EFI_BOOT_SCRIPT_IO_WRITE_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteIoWrite (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_IO_READ_WRITE_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteIoReadWrite (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteMemWrite (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_MEM_READ_WRITE_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteMemReadWrite (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWritePciCfgWrite (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWritePciCfgReadWrite (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_SMBUS_EXECUTE_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteSmbusExecute (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_STALL_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteStall (Marker);
      VA_END (Marker);

      break;

    case EFI_BOOT_SCRIPT_DISPATCH_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteDispatch (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_DISPATCH_2_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteDispatch2 (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_INFORMATION_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteInformation (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_MEM_POLL_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteMemPoll (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWritePciCfg2Write (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWritePciCfg2ReadWrite (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_IO_POLL_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteIoPoll (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_PCI_CONFIG_POLL_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWritePciConfigPoll (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWritePciConfig2Poll (Marker);
      VA_END (Marker);
      break;

    default:
      Status = EFI_INVALID_PARAMETER;
      break;
  }

  return Status;
}

/**
  Insert a record into a specified Framework boot script table.

  This function is used to store an OpCode to be replayed as part of the S3 resume boot path. It is
  assumed this protocol has platform specific mechanism to store the OpCode set and replay them
  during the S3 resume.
  The opcode is inserted before or after the specified position in the boot script table. If Position is
  NULL then that position is after the last opcode in the table (BeforeOrAfter is FALSE) or before
  the first opcode in the table (BeforeOrAfter is TRUE). The position which is pointed to by
  Position upon return can be used for subsequent insertions.

  @param  This                  A pointer to the EFI_S3_SAVE_STATE_PROTOCOL instance.
  @param  BeforeOrAfter         Specifies whether the opcode is stored before (TRUE) or after (FALSE) the position
                                in the boot script table specified by Position. If Position is NULL or points to
                                NULL then the new opcode is inserted at the beginning of the table (if TRUE) or end
                                of the table (if FALSE).
  @param  Position              On entry, specifies the position in the boot script table where the opcode will be
                                inserted, either before or after, depending on BeforeOrAfter. On exit, specifies
                                the position of the inserted opcode in the boot script table.
  @param  OpCode                The operation code (opcode) number.
  @param  ...                   Argument list that is specific to each opcode.

  @retval EFI_SUCCESS           The operation succeeded. A record was added into the
                                specified script table.
  @retval EFI_INVALID_PARAMETER The Opcode is an invalid opcode value or the Position is not a valid position in the boot script table..
  @retval EFI_OUT_OF_RESOURCES  There is insufficient memory to store the boot script.

**/
EFI_STATUS
EFIAPI
BootScriptInsert (
  IN CONST EFI_S3_SAVE_STATE_PROTOCOL   *This,
  IN       BOOLEAN                      BeforeOrAfter,
  IN OUT   EFI_S3_BOOT_SCRIPT_POSITION  *Position OPTIONAL,
  IN       UINTN                        OpCode,
  ...
  )
{
  EFI_STATUS  Status;
  VA_LIST     Marker;

  //
  // Build script according to opcode
  //
  switch (OpCode) {
    case EFI_BOOT_SCRIPT_IO_WRITE_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteIoWrite (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_IO_READ_WRITE_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteIoReadWrite (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteMemWrite (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_MEM_READ_WRITE_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteMemReadWrite (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWritePciCfgWrite (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWritePciCfgReadWrite (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_SMBUS_EXECUTE_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteSmbusExecute (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_STALL_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteStall (Marker);
      VA_END (Marker);

      break;

    case EFI_BOOT_SCRIPT_DISPATCH_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteDispatch (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_DISPATCH_2_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteDispatch2 (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_INFORMATION_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteInformation (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_MEM_POLL_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteMemPoll (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWritePciCfg2Write (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWritePciCfg2ReadWrite (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_IO_POLL_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWriteIoPoll (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_PCI_CONFIG_POLL_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWritePciConfigPoll (Marker);
      VA_END (Marker);
      break;

    case EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL_OPCODE:
      VA_START (Marker, OpCode);
      Status = BootScriptWritePciConfig2Poll (Marker);
      VA_END (Marker);
      break;

    default:
      Status = EFI_INVALID_PARAMETER;
      break;
  }

  if (!EFI_ERROR (Status)) {
    Status = S3BootScriptMoveLastOpcode (BeforeOrAfter, (VOID **)Position);
  }

  return Status;
}

/**
  Find a label within the boot script table and, if not present, optionally create it.

  If the label Label is already exists in the boot script table, then no new label is created, the
  position of the Label is returned in *Position and EFI_SUCCESS is returned.
  If the label Label does not already exist and CreateIfNotFound is TRUE, then it will be
  created before or after the specified position and EFI_SUCCESS is returned.
  If the label Label does not already exist and CreateIfNotFound is FALSE, then
  EFI_NOT_FOUND is returned.

  @param  This                  A pointer to the EFI_S3_SAVE_STATE_PROTOCOL instance.
  @param  BeforeOrAfter         Specifies whether the label is stored before (TRUE) or after (FALSE) the position in
                                the boot script table specified by Position. If Position is NULL or points to
                                NULL then the new label is inserted at the beginning of the table (if TRUE) or end of
                                the table (if FALSE).
  @param  CreateIfNotFound      Specifies whether the label will be created if the label does not exists (TRUE) or not
                                (FALSE).
  @param  Position              On entry, specifies the position in the boot script table where the label will be inserted,
                                either before or after, depending on BeforeOrAfter. On exit, specifies the position
                                of the inserted label in the boot script table.
  @param  Label                 Points to the label which will be inserted in the boot script table.

  @retval EFI_SUCCESS           The label already exists or was inserted.
  @retval EFI_INVALID_PARAMETER The Label is NULL or points to an empty string.
  @retval EFI_INVALID_PARAMETER The Position is not a valid position in the boot script table.

**/
EFI_STATUS
EFIAPI
BootScriptLabel (
  IN CONST EFI_S3_SAVE_STATE_PROTOCOL   *This,
  IN       BOOLEAN                      BeforeOrAfter,
  IN       BOOLEAN                      CreateIfNotFound,
  IN OUT   EFI_S3_BOOT_SCRIPT_POSITION  *Position OPTIONAL,
  IN CONST CHAR8                        *Label
  )
{
  return S3BootScriptLabel (BeforeOrAfter, CreateIfNotFound, (VOID **)Position, Label);
}

/**
  Compare two positions in the boot script table and return their relative position.

  This function compares two positions in the boot script table and returns their relative positions. If
  Position1 is before Position2, then -1 is returned. If Position1 is equal to Position2,
  then 0 is returned. If Position1 is after Position2, then 1 is returned.

  @param  This                  A pointer to the EFI_S3_SAVE_STATE_PROTOCOL instance.
  @param  Position1             The positions in the boot script table to compare
  @param  Position2             The positions in the boot script table to compare
  @param  RelativePosition      On return, points to the result of the comparison

  @retval EFI_SUCCESS           The operation succeeded.
  @retval EFI_INVALID_PARAMETER The Position1 or Position2 is not a valid position in the boot script table.
  @retval EFI_INVALID_PARAMETER The RelativePosition is NULL.

**/
EFI_STATUS
EFIAPI
BootScriptCompare (
  IN CONST EFI_S3_SAVE_STATE_PROTOCOL   *This,
  IN       EFI_S3_BOOT_SCRIPT_POSITION  Position1,
  IN       EFI_S3_BOOT_SCRIPT_POSITION  Position2,
  OUT      UINTN                        *RelativePosition
  )
{
  return S3BootScriptCompare (Position1, Position2, RelativePosition);
}

/**
  This routine is entry point of ScriptSave driver.

  @param  ImageHandle           Handle for this drivers loaded image protocol.
  @param  SystemTable           EFI system table.

  @retval EFI_OUT_OF_RESOURCES  No enough resource
  @retval EFI_SUCCESS           Succesfully installed the ScriptSave driver.
  @retval other                 Errors occurred.

**/
EFI_STATUS
EFIAPI
InitializeS3SaveState (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   EndOfDxeEvent;

  if (!PcdGetBool (PcdAcpiS3Enable)) {
    return EFI_UNSUPPORTED;
  }

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  AcpiS3ContextSaveOnEndOfDxe,
                  NULL,
                  &gEfiEndOfDxeEventGroupGuid,
                  &EndOfDxeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return gBS->InstallProtocolInterface (
                &mHandle,
                &gEfiS3SaveStateProtocolGuid,
                EFI_NATIVE_INTERFACE,
                &mS3SaveState
                );
}
