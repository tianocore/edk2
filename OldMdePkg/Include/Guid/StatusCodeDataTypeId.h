/** @file
  GUID used to identify id for the caller who is initiating the Status Code.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  StatusCodeDataTypeId.h

  @par Revision Reference:
  GUIDs defined in Status Codes Specification 0.92

**/

#ifndef __STATUS_CODE_DATA_TYPE_ID_GUID_H__
#define __STATUS_CODE_DATA_TYPE_ID_GUID_H__

//
// String Data Type defintion. This is part of Status Code Specification
//
#define EFI_STATUS_CODE_DATA_TYPE_STRING_GUID \
  { 0x92D11080, 0x496F, 0x4D95, { 0xBE, 0x7E, 0x03, 0x74, 0x88, 0x38, 0x2B, 0x0A } }

extern EFI_GUID gEfiStatusCodeDataTypeStringGuid;

//
// This GUID indicates that the format of the accompanying data depends
// upon the Status Code Value, but follows this Specification
//
#define EFI_STATUS_CODE_SPECIFIC_DATA_GUID \
  { 0x335984bd, 0xe805, 0x409a, { 0xb8, 0xf8, 0xd2, 0x7e, 0xce, 0x5f, 0xf7, 0xa6 } }

extern EFI_GUID gEfiStatusCodeSpecificDataGuid;

//
// Debug Assert Data. This is part of Status Code Specification
//
#define EFI_STATUS_CODE_DATA_TYPE_ASSERT_GUID \
 { 0xDA571595, 0x4D99, 0x487C, { 0x82, 0x7C, 0x26, 0x22, 0x67, 0x7D, 0x33, 0x07 } }


extern EFI_GUID gEfiStatusCodeDataTypeAssertGuid;

//
// Exception Data type (CPU REGS)
//
#define EFI_STATUS_CODE_DATA_TYPE_EXCEPTION_HANDLER_GUID \
  { 0x3BC2BD12, 0xAD2E, 0x11D5, { 0x87, 0xDD, 0x00, 0x06, 0x29, 0x45, 0xC3, 0xB9 } }

extern EFI_GUID gEfiStatusCodeDataTypeExceptionHandlerGuid;

//
// Debug DataType defintions. User Defined Data Types.
//
#define EFI_STATUS_CODE_DATA_TYPE_DEBUG_GUID \
  { 0x9A4E9246, 0xD553, 0x11D5, { 0x87, 0xE2, 0x00, 0x06, 0x29, 0x45, 0xC3, 0xb9 } }

extern EFI_GUID gEfiStatusCodeDataTypeDebugGuid;

//
// Progress Code. User Defined Data Type Guid.
//
#define EFI_STATUS_CODE_DATA_TYPE_ERROR_GUID \
  0xAB359CE3, 0x99B3, 0xAE18, { 0xC8, 0x9D, 0x95, 0xD3, 0xB0, 0x72, 0xE1, 0x9B } }

extern EFI_GUID gEfiStatusCodeDataTypeErrorGuid;


//
// Progress Code. User Defined Data Type Guid.
//
#define EFI_STATUS_CODE_DATA_TYPE_PROGRESS_CODE_GUID \
  { 0xA356AB39, 0x35C4, 0x35DA, { 0xB3, 0x7A, 0xF8, 0xEA, 0x9E, 0x8B, 0x36, 0xA3 } }

extern EFI_GUID gEfiStatusCodeDataTypeProgressCodeGuid;

#endif
