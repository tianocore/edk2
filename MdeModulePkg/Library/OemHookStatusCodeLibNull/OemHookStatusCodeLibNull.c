/** @file
  Null instance of OEM Hook Status Code Library with empty functions.

  Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


/**
  Initialize OEM status code device .

  @retval  EFI_SUCCESS   Always return EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
OemHookStatusCodeInitialize (
  VOID
  )
{
  return EFI_SUCCESS;
}

/**
  Report status code to OEM device.
 
  @param  CodeType      Indicates the type of status code being reported.
  @param  Value         Describes the current status of a hardware or software entity.  
                        This included information about the class and subclass that is used to classify the entity 
                        as well as an operation.  For progress codes, the operation is the current activity. 
                        For error codes, it is the exception.  For debug codes, it is not defined at this time. 
  @param  Instance      The enumeration of a hardware or software entity within the system.  
                        A system may contain multiple entities that match a class/subclass pairing. 
                        The instance differentiates between them.  An instance of 0 indicates that instance information is unavailable, 
                        not meaningful, or not relevant.  Valid instance numbers start with 1.
  @param  CallerId      This optional parameter may be used to identify the caller. 
                        This parameter allows the status code driver to apply different rules to different callers. 
  @param  Data          This optional parameter may be used to pass additional data
 
  @retval EFI_SUCCESS   Always return EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
OemHookStatusCodeReport (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId, OPTIONAL
  IN EFI_STATUS_CODE_DATA     *Data      OPTIONAL
  )
{
  return EFI_SUCCESS;
}

