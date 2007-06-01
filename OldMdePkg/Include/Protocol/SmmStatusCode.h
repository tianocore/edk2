/** @file
  This file declares SMM Status code Protocol.

  This code abstracts SMM Status Code reporting.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  SmmStatusCode.h

  @par Revision Reference:
  This Protocol is defined in Framework of EFI SMM Core Interface Spec
  Version 0.9.

**/

#ifndef _PROTOCOL_SMM_STATUS_CODE_H__
#define _PROTOCOL_SMM_STATUS_CODE_H__

//
// Global ID for the Smm Status Code Protocol
//
#define EFI_SMM_STATUS_CODE_PROTOCOL_GUID \
  { \
    0x6afd2b77, 0x98c1, 0x4acd, {0xa6, 0xf9, 0x8a, 0x94, 0x39, 0xde, 0xf, 0xb1 } \
  }

typedef struct _EFI_SMM_STATUS_CODE_PROTOCOL  EFI_SMM_STATUS_CODE_PROTOCOL;

/**
  Service to emit the status code in SMM.

  @param  This                  Pointer to EFI_SMM_STATUS_CODE_PROTOCOL instance.
  @param  CodeType              Indicates the type of status code being reported.
  @param  Value                 Describes the current status of a hardware or software entity.
                                This included information about the class and subclass that is used to
                                classify the entity as well as an operation.
  @param  Instance              The enumeration of a hardware or software entity within
                                the system. Valid instance numbers start with 1.
  @param  CallerId              This optional parameter may be used to identify the caller.
                                This parameter allows the status code driver to apply different rules to
                                different callers.
  @param  Data                  This optional parameter may be used to pass additional data.

  @retval EFI_SUCCESS           The function completed successfully
  @retval EFI_DEVICE_ERROR      The function should not be completed due to a device error.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_REPORT_STATUS_CODE) (
  IN EFI_SMM_STATUS_CODE_PROTOCOL *This,
  IN EFI_STATUS_CODE_TYPE         CodeType,
  IN EFI_STATUS_CODE_VALUE        Value,
  IN UINT32                       Instance,
  IN EFI_GUID                     *CallerId,
  IN EFI_STATUS_CODE_DATA         *Data OPTIONAL
  );

/**
  @par Protocol Description:
  Provides status code services from SMM.

  @param ReportStatusCode
  Allows for the SMM agent to produce a status code output.

**/
struct _EFI_SMM_STATUS_CODE_PROTOCOL {
  EFI_SMM_REPORT_STATUS_CODE  ReportStatusCode;
};

extern EFI_GUID gEfiSmmStatusCodeProtocolGuid;

#endif
