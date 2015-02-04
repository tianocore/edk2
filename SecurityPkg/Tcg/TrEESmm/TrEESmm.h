/** @file
  The header file for TrEE SMM driver.
  
Copyright (c) 2013 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __TREE_SMM_H__
#define __TREE_SMM_H__

#include <PiDxe.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/Tpm2Acpi.h>

#include <Guid/TrEEPhysicalPresenceData.h>
#include <Guid/MemoryOverwriteControl.h>
#include <Guid/TpmInstance.h>

#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/SmmVariable.h>
#include <Protocol/TrEEProtocol.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/TpmMeasurementLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/TrEEPpVendorLib.h>

#pragma pack(1)
typedef struct {
  UINT8                  SoftwareSmi;
  UINT32                 Parameter;
  UINT32                 Response;
  UINT32                 Request;
  UINT32                 LastRequest;
  UINT32                 ReturnCode;
} PHYSICAL_PRESENCE_NVS;

typedef struct {
  UINT8                  SoftwareSmi;
  UINT32                 Parameter;
  UINT32                 Request;
  UINT32                 ReturnCode;
} MEMORY_CLEAR_NVS;

typedef struct {
  PHYSICAL_PRESENCE_NVS  PhysicalPresence;
  MEMORY_CLEAR_NVS       MemoryClear;
} TCG_NVS;

typedef struct {
  UINT8                  OpRegionOp;
  UINT32                 NameString;
  UINT8                  RegionSpace;
  UINT8                  DWordPrefix;
  UINT32                 RegionOffset;
  UINT8                  BytePrefix;
  UINT8                  RegionLen;
} AML_OP_REGION_32_8;
#pragma pack()

//
// The definition for TCG physical presence ACPI function
//
#define ACPI_FUNCTION_GET_PHYSICAL_PRESENCE_INTERFACE_VERSION      1
#define ACPI_FUNCTION_SUBMIT_REQUEST_TO_BIOS                       2
#define ACPI_FUNCTION_GET_PENDING_REQUEST_BY_OS                    3
#define ACPI_FUNCTION_GET_PLATFORM_ACTION_TO_TRANSITION_TO_BIOS    4
#define ACPI_FUNCTION_RETURN_REQUEST_RESPONSE_TO_OS                5
#define ACPI_FUNCTION_SUBMIT_PREFERRED_USER_LANGUAGE               6
#define ACPI_FUNCTION_SUBMIT_REQUEST_TO_BIOS_2                     7
#define ACPI_FUNCTION_GET_USER_CONFIRMATION_STATUS_FOR_REQUEST     8

//
// The return code for Return TPM Operation Response to OS Environment
//
#define PP_RETURN_TPM_OPERATION_RESPONSE_SUCCESS                   0
#define PP_RETURN_TPM_OPERATION_RESPONSE_FAILURE                   1

//
// The definition for TCG MOR
//
#define ACPI_FUNCTION_DSM_MEMORY_CLEAR_INTERFACE                   1
#define ACPI_FUNCTION_PTS_CLEAR_MOR_BIT                            2

//
// The return code for Memory Clear Interface Functions
//
#define MOR_REQUEST_SUCCESS                                        0
#define MOR_REQUEST_GENERAL_FAILURE                                1

#endif  // __TCG_SMM_H__
