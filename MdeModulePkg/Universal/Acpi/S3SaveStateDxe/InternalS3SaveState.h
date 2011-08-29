/** @file
  Internal header file for S3 Boot Script Saver state driver.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions
  of the BSD License which accompanies this distribution.  The
  full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _INTERNAL_S3_SAVE_STATE_H_
#define _INTERNAL_S3_SAVE_STATE_H_
#include <PiDxe.h>

#include <Protocol/S3SaveState.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/S3BootScriptLib.h>
#include <Library/PcdLib.h>
#include <Library/SmbusLib.h>
#include <IndustryStandard/SmBus.h>
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
  IN CONST EFI_S3_SAVE_STATE_PROTOCOL      *This,
  IN       UINT16                           OpCode,
  ...
  );
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
  IN CONST EFI_S3_SAVE_STATE_PROTOCOL    *This,
  IN       BOOLEAN                          BeforeOrAfter,
  IN OUT   EFI_S3_BOOT_SCRIPT_POSITION     *Position OPTIONAL,
  IN       UINT16                           OpCode,
  ...
  );
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
  @retval EFI_INVALID_PARAMETER The Opcode is an invalid opcode value or the Position is not a valid position in the boot script table..
  
**/
EFI_STATUS
EFIAPI
BootScriptLabel (
  IN CONST EFI_S3_SAVE_STATE_PROTOCOL           *This,
  IN       BOOLEAN                               BeforeOrAfter,
  IN       BOOLEAN                               CreateIfNotFound,
  IN OUT   EFI_S3_BOOT_SCRIPT_POSITION          *Position OPTIONAL,
  IN CONST CHAR8                                *Label
  );
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

**/
EFI_STATUS
EFIAPI 
BootScriptCompare (
  IN CONST EFI_S3_SAVE_STATE_PROTOCOL      *This,
  IN       EFI_S3_BOOT_SCRIPT_POSITION      Position1,
  IN       EFI_S3_BOOT_SCRIPT_POSITION      Position2,
  OUT      UINTN                           *RelativePosition
  );
  
#endif //_INTERNAL_S3_SAVE_STATE_H_
