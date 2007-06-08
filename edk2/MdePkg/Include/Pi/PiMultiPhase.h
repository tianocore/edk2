/** @file
  Include file matches things in PI for multiple module types.

  Copyright (c) 2006 - 2007, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  PiMultiPhase.h

  @par Revision Reference:
  Version 1.0.

**/

#ifndef __PI_MULTIPHASE_H__
#define __PI_MULTIPHASE_H__

#include <Uefi/UefiMultiPhase.h>

#include <Pi/PiFirmwareVolume.h>
#include <Pi/PiFirmwareFile.h>
#include <Pi/PiBootMode.h>

#include <Pi/PiHob.h>
#include <Pi/PiDependency.h>

//
// Status Code Type Definition
//
typedef UINT32 	EFI_STATUS_CODE_TYPE;

//
// A Status Code Type is made up of the code type and severity
// All values masked by EFI_STATUS_CODE_RESERVED_MASK are
// reserved for use by this specification.
//
#define EFI_STATUS_CODE_TYPE_MASK 		0x000000FF
#define EFI_STATUS_CODE_SEVERITY_MASK 0xFF000000
#define EFI_STATUS_CODE_RESERVED_MASK 0x00FFFF00

//
// Definition of code types, all other values masked by
// EFI_STATUS_CODE_TYPE_MASK are reserved for use by
// this specification.
//
#define EFI_PROGRESS_CODE 						0x00000001
#define EFI_ERROR_CODE 								0x00000002
#define EFI_DEBUG_CODE 								0x00000003

//
// Definitions of severities, all other values masked by
// EFI_STATUS_CODE_SEVERITY_MASK are reserved for use by
// this specification.
// Uncontained errors are major errors that could not contained
// to the specific component that is reporting the error
// For example, if a memory error was not detected early enough,
// the bad data could be consumed by other drivers.
//
#define EFI_ERROR_MINOR 							0x40000000
#define EFI_ERROR_MAJOR 							0x80000000
#define EFI_ERROR_UNRECOVERED 				0x90000000
#define EFI_ERROR_UNCONTAINED 				0xa0000000

//
// Status Code Value Definition
//
typedef UINT32 EFI_STATUS_CODE_VALUE;
//
// A Status Code Value is made up of the class, subclass, and
// an operation.
//
#define EFI_STATUS_CODE_CLASS_MASK 			0xFF000000
#define EFI_STATUS_CODE_SUBCLASS_MASK 	0x00FF0000
#define EFI_STATUS_CODE_OPERATION_MASK 	0x0000FFFF
//
// Definition of Status Code extended data header.
// The data will follow HeaderSize bytes from the beginning of
// the structure and is Size bytes long.
//
typedef struct {
	UINT16 		HeaderSize;
	UINT16 		Size;
	EFI_GUID 	Type;
} EFI_STATUS_CODE_DATA;

#endif
