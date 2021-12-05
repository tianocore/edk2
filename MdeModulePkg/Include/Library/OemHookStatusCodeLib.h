/** @file
  OEM hook status code library. Platform can implement an instance to
  initialize the OEM devices to report status code information.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __OEM_HOOK_STATUSCODE_LIB__
#define __OEM_HOOK_STATUSCODE_LIB__

/**

  Initialize OEM status code device.


  @return    Status of initialization of OEM status code device.

**/
EFI_STATUS
EFIAPI
OemHookStatusCodeInitialize (
  VOID
  );

/**
  Report status code to OEM device.

  @param  CodeType      Indicates the type of status code being reported.

  @param  Value         Describes the current status of a hardware or software entity.
                        This includes both an operation and classification information
                        about the class and subclass.
                        For progress codes, the operation is the current activity.
                        For error codes, it is the exception.  For debug codes,
                        it is not defined at this time.
                        Specific values are discussed in the Intel Platform Innovation
                        Framework for EFI Status Code Specification.

  @param  Instance      The enumeration of a hardware or software entity within the system.
                        A system may contain multiple entities that match a class/subclass
                        pairing.
                        The instance differentiates between them.  An instance of 0
                        indicates that instance information is unavailable,
                        not meaningful, or not relevant.  Valid instance numbers
                        start with 1.


  @param  CallerId      This optional parameter may be used to identify the caller.
                        This parameter allows the status code driver to apply
                        different rules to different callers.
                        Type EFI_GUID is defined in InstallProtocolInterface()
                        in the UEFI 2.0 Specification.


  @param  Data          This optional parameter may be used to pass additional data.

  @return               The function always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
OemHookStatusCodeReport (
  IN EFI_STATUS_CODE_TYPE   CodeType,
  IN EFI_STATUS_CODE_VALUE  Value,
  IN UINT32                 Instance,
  IN EFI_GUID               *CallerId  OPTIONAL,
  IN EFI_STATUS_CODE_DATA   *Data      OPTIONAL
  );

#endif // __OEM_HOOK_STATUSCODE_LIB__
