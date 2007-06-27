/** @file
  Status code Runtime Protocol as defined in the DXE CIS

  The StatusCode () Tiano service is added to the EFI system table and the 
  EFI_STATUS_CODE_ARCH_PROTOCOL_GUID protocol is registered with a NULL 
  pointer.

  No CRC of the EFI system table is required, as it is done in the DXE core.

  This code abstracts Status Code reporting.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  @par Revision Reference:
  Version 0.91B.

**/

#ifndef __STATUS_CODE_RUNTIME_PROTOCOL_H__
#define __STATUS_CODE_RUNTIME_PROTOCOL_H__

#define EFI_STATUS_CODE_RUNTIME_PROTOCOL_GUID  \
{ 0xd2b2b828, 0x826, 0x48a7,  { 0xb3, 0xdf, 0x98, 0x3c, 0x0, 0x60, 0x24, 0xf0 } }

/**
  Provides an interface that a software module can call to report a status code.

  @param  Type             Indicates the type of status code being reported.
  @param  Value            Describes the current status of a hardware or software entity.
                           This included information about the class and subclass that is used to
                           classify the entity as well as an operation.
  @param  Instance         The enumeration of a hardware or software entity within
                           the system. Valid instance numbers start with 1.
  @param  CallerId         This optional parameter may be used to identify the caller.
                           This parameter allows the status code driver to apply different rules to
                           different callers.
  @param  Data             This optional parameter may be used to pass additional data.

  @retval EFI_SUCCESS           The function completed successfully
  @retval EFI_DEVICE_ERROR      The function should not be completed due to a device error.

**/
typedef
EFI_STATUS 
(EFIAPI *EFI_REPORT_STATUS_CODE) (
  IN EFI_STATUS_CODE_TYPE     Type,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId  OPTIONAL,
  IN EFI_STATUS_CODE_DATA     *Data      OPTIONAL
  );

/**
  @par Protocol Description:
  Provides the service required to report a status code to the platform firmware.
  This protocol must be produced by a runtime DXE driver and may be consumed 
  only by the DXE Foundation.

  @param  ReportStatusCode Emit a status code.

**/
typedef struct _EFI_STATUS_CODE_PROTOCOL {
  EFI_REPORT_STATUS_CODE         ReportStatusCode;
} EFI_STATUS_CODE_PROTOCOL;

extern EFI_GUID gEfiStatusCodeRuntimeProtocolGuid;

#endif
