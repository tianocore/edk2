/** @file
  The header file for Tcg2 SMM driver.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __TCG2_SMM_H__
#define __TCG2_SMM_H__

#include <PiDxe.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/Tpm2Acpi.h>

#include <Guid/MemoryOverwriteControl.h>
#include <Guid/TpmInstance.h>

#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/SmmVariable.h>
#include <Protocol/Tcg2Protocol.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/TpmMeasurementLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/Tcg2PhysicalPresenceLib.h>
#include <Library/IoLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/Tpm2DeviceLib.h>

#include <IndustryStandard/TpmPtp.h>

#pragma pack(1)
typedef struct {
  UINT8                  SoftwareSmi;
  UINT32                 Parameter;
  UINT32                 Response;
  UINT32                 Request;
  UINT32                 RequestParameter;
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
  UINT32                 PPRequestUserConfirm;
  UINT32                 TpmIrqNum;
  BOOLEAN                IsShortFormPkgLength;
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
// The definition for TCG MOR
//
#define ACPI_FUNCTION_DSM_MEMORY_CLEAR_INTERFACE                   1
#define ACPI_FUNCTION_PTS_CLEAR_MOR_BIT                            2

//
// The return code for Memory Clear Interface Functions
//
#define MOR_REQUEST_SUCCESS                                        0
#define MOR_REQUEST_GENERAL_FAILURE                                1

//
// Physical Presence Interface Version supported by Platform
//
#define PHYSICAL_PRESENCE_VERSION_TAG                              "$PV"
#define PHYSICAL_PRESENCE_VERSION_SIZE                             4

//
// PNP _HID for TPM2 device
//
#define TPM_HID_TAG                                                "NNNN0000"
#define TPM_HID_PNP_SIZE                                           8
#define TPM_HID_ACPI_SIZE                                          9

#define TPM_PRS_RESL                                               "RESL"
#define TPM_PRS_RESS                                               "RESS"
#define TPM_PRS_RES_NAME_SIZE                                      4
//
// Minimum PRS resource template size
//  1 byte    for  BufferOp
//  1 byte    for  PkgLength
//  2 bytes   for  BufferSize
//  12 bytes  for  Memory32Fixed descriptor
//  5 bytes   for  Interrupt descriptor
//  2 bytes   for  END Tag
//
#define TPM_POS_RES_TEMPLATE_MIN_SIZE                              (1 + 1 + 2 + 12 + 5 + 2)

//
// Max Interrupt buffer size for PRS interrupt resource
// Now support 15 interrupts in maxmum
//
#define MAX_PRS_INT_BUF_SIZE                                       (15*4)
#endif  // __TCG_SMM_H__
