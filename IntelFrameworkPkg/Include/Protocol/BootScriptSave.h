/** @file
  This protocol is used to store or record various boot scripts into boot 
  script tables.

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  This protocol defined in the Boot Script Specification, Version 0.91.

**/

#ifndef _BOOT_SCRIPT_SAVE_PROTOCOL_H_
#define _BOOT_SCRIPT_SAVE_PROTOCOL_H_

///
/// S3 Save Protocol GUID.
///
#define EFI_BOOT_SCRIPT_SAVE_PROTOCOL_GUID \
  { \
    0x470e1529, 0xb79e, 0x4e32, {0xa0, 0xfe, 0x6a, 0x15, 0x6d, 0x29, 0xf9, 0xb2 } \
  }

typedef struct _EFI_BOOT_SCRIPT_SAVE_PROTOCOL EFI_BOOT_SCRIPT_SAVE_PROTOCOL;

/**
  Adds a record into a specified Framework boot script table.

  @param  This                  A pointer to the EFI_BOOT_SCRIPT_SAVE_PROTOCOL instance.
  @param  TableName             The name of the script table. Currently, the only meaningful
                                value is EFI_ACPI_S3_RESUME_SCRIPT_TABLE.
  @param  OpCode                The operation code (opcode) number.
  @param  ...                   The argument list that is specific to each opcode.

  @retval EFI_SUCCESS           The operation succeeded. A record was added into the specified script table.
  @retval EFI_INVALID_PARAMETER The parameter is illegal, or the given boot script is not supported.
  @retval EFI_OUT_OF_RESOURCES  There is insufficient memory to store the boot script.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_BOOT_SCRIPT_WRITE)(
  IN EFI_BOOT_SCRIPT_SAVE_PROTOCOL            *This,
  IN UINT16                                   TableName,
  IN UINT16                                   OpCode,
  ...
  );

/**
  Closes the specified script table.

  @param  This                  A pointer to the EFI_BOOT_SCRIPT_SAVE_PROTOCOL instance.
  @param  TableName             The name of the script table.
  @param  Address               A pointer to the physical address where the table begins.

  @retval EFI_SUCCESS           The table was successfully returned.
  @retval EFI_NOT_FOUND         The specified table was not created previously.
  @retval EFI_OUT_OF_RESOURCES  Memory is insufficient to hold the reorganized boot script table.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_BOOT_SCRIPT_CLOSE_TABLE)(
  IN EFI_BOOT_SCRIPT_SAVE_PROTOCOL            *This,
  IN UINT16                                   TableName,
  OUT EFI_PHYSICAL_ADDRESS                    *Address
  );

///
/// The EFI_BOOT_SCRIPT_SAVE_PROTOCOL publishes the Framework boot script abstractions
/// to store or record various boot scripts into boot script tables.
///
struct _EFI_BOOT_SCRIPT_SAVE_PROTOCOL {
  EFI_BOOT_SCRIPT_WRITE        Write;      ///< Writes various boot scripts to a boot script table.
  EFI_BOOT_SCRIPT_CLOSE_TABLE  CloseTable; ///< Retrieves and closes a script table.
};

extern EFI_GUID gEfiBootScriptSaveProtocolGuid;

#endif
