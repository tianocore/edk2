/** @file
  Provides the parent dispatch service for a given SMI source generator.
  The EFI_SMM_ICHN_DISPATCH_PROTOCOL provides the ability to install child handlers for
  the given event types.

  Copyright (c) 2008 - 2009, Intel Corporation
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

#ifndef _EFI_SMM_ICHN_DISPATCH_H_
#define _EFI_SMM_ICHN_DISPATCH_H_

//
// Global ID for the Smm Status code Protocol
//
#define EFI_SMM_STATUS_CODE_PROTOCOL_GUID \
  { \
    0x6afd2b77, 0x98c1, 0x4acd, {0xa6, 0xf9, 0x8a, 0x94, 0x39, 0xde, 0xf, 0xb1 } \
  }

typedef struct _EFI_SMM_STATUS_CODE_PROTOCOL  EFI_SMM_STATUS_CODE_PROTOCOL;


/**
  Unregister a child SMI source dispatch function with a parent SMM driver

  @param  This                  Points to this instance of the EFI_SMM_STATUS_CODE_PROTOCOL.
  @param  CodeType              Indicates the type of status code being reported.
  @param  Value                 Describes the current status of a hardware or software entity. 
                                This status includes information about the class and subclass 
                                that is used to classify the entity, as well as an operation. 
                                For progress codes, the operation is the current activity. For 
                                error codes, it is the exception. For debug codes, it is not defined 
                                at this time.
  @param  Instance              The enumeration of a hardware or software entity within the system. 
                                A system may contain multiple entities that match a class/subclass pairing.                                 
  @param  CallerId              This optional parameter may be used to identify the caller. This parameter 
                                allows the status code driver to apply different rules to different callers.
  @param  Data                  This optional parameter may be used to pass additional data.
 
 
  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_DEVICE_ERROR      The function should not be completed due to a device error.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_REPORT_STATUS_CODE)(
  IN EFI_SMM_STATUS_CODE_PROTOCOL              *This,
  IN EFI_STATUS_CODE_TYPE                       CodeType,
  IN EFI_STATUS_CODE_VALUE                      Value, 
  IN UINT32                                     Instance,
  IN EFI_GUID                                  *CallerId,
  IN EFI_STATUS_CODE_DATA                      *Data OPTIONAL
);


/**
  The EFI_SMM_STATUS_CODE_PROTOCOL provides the basic status code services while in SMRAM.
 **/
struct _EFI_SMM_STATUS_CODE_PROTOCOL {
   EFI_SMM_REPORT_STATUS_CODE ReportStatusCode; ///< Allows for the SMM agent to produce a status code output.
};

extern EFI_GUID gEfiSmmStatusCodeProtocolGuid;

#endif
