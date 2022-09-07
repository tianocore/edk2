/** @file
  Library to call the RISC-V SBI ecalls

  Copyright (c) 2021-2022, Hewlett Packard Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Hart - Hardware Thread, similar to a CPU core

  Currently, EDK2 needs to call SBI only to set the time and to do system reset.

**/

#ifndef RISCV_SBI_LIB_H_
#define RISCV_SBI_LIB_H_

#include <Uefi.h>

/* SBI Extension IDs */
#define SBI_EXT_TIME  0x54494D45
#define SBI_EXT_SRST  0x53525354

/* SBI function IDs for TIME extension*/
#define SBI_EXT_TIME_SET_TIMER  0x0

/* SBI function IDs for SRST extension */
#define SBI_EXT_SRST_RESET  0x0

#define SBI_SRST_RESET_TYPE_SHUTDOWN     0x0
#define SBI_SRST_RESET_TYPE_COLD_REBOOT  0x1
#define SBI_SRST_RESET_TYPE_WARM_REBOOT  0x2

#define SBI_SRST_RESET_REASON_NONE     0x0
#define SBI_SRST_RESET_REASON_SYSFAIL  0x1

/* SBI return error codes */
#define SBI_SUCCESS                0
#define SBI_ERR_FAILED             -1
#define SBI_ERR_NOT_SUPPORTED      -2
#define SBI_ERR_INVALID_PARAM      -3
#define SBI_ERR_DENIED             -4
#define SBI_ERR_INVALID_ADDRESS    -5
#define SBI_ERR_ALREADY_AVAILABLE  -6
#define SBI_ERR_ALREADY_STARTED    -7
#define SBI_ERR_ALREADY_STOPPED    -8

#define SBI_LAST_ERR  SBI_ERR_ALREADY_STOPPED

typedef struct {
  UINT64    BootHartId;
  VOID      *PeiServiceTable;    // PEI Service table
  VOID      *PrePiHobList;       // Pre PI Hob List
  UINT64    FlattenedDeviceTree; // Pointer to Flattened Device tree
} EFI_RISCV_FIRMWARE_CONTEXT;

//
// EDK2 OpenSBI firmware extension return status.
//
typedef struct {
  UINTN    Error; ///< SBI status code
  UINTN    Value; ///< Value returned
} SBI_RET;

VOID
EFIAPI
SbiSetTimer (
  IN  UINT64  Time
  );

EFI_STATUS
EFIAPI
SbiSystemReset (
  IN  UINTN  ResetType,
  IN  UINTN  ResetReason
  );

/**
  Get firmware context of the calling hart.

  @param[out] FirmwareContext      The firmware context pointer.
**/
VOID
EFIAPI
GetFirmwareContext (
  OUT EFI_RISCV_FIRMWARE_CONTEXT  **FirmwareContext
  );

/**
  Set firmware context of the calling hart.

  @param[in] FirmwareContext       The firmware context pointer.
**/
VOID
EFIAPI
SetFirmwareContext (
  IN EFI_RISCV_FIRMWARE_CONTEXT  *FirmwareContext
  );

/**
  Get pointer to OpenSBI Firmware Context

  Get the pointer of firmware context.

  @param    FirmwareContextPtr   Pointer to retrieve pointer to the
                                 Firmware Context.
**/
VOID
EFIAPI
GetFirmwareContextPointer (
  IN OUT EFI_RISCV_FIRMWARE_CONTEXT  **FirmwareContextPtr
  );

/**
  Set pointer to OpenSBI Firmware Context

  Set the pointer of firmware context.

  @param    FirmwareContextPtr   Pointer to Firmware Context.
**/
VOID
EFIAPI
SetFirmwareContextPointer (
  IN EFI_RISCV_FIRMWARE_CONTEXT  *FirmwareContextPtr
  );

/**
  Make ECALL in assembly

  Switch to M-mode

  @param[in,out]   Arg0
  @param[in,out]   Arg1
  @param[in]       Arg2
  @param[in]       Arg3
  @param[in]       Arg4
  @param[in]       Arg5
  @param[in]       FID
  @param[in]       EXT
**/
VOID
EFIAPI
RiscVSbiEcall (
  IN OUT UINTN  *Arg0,
  IN OUT UINTN  *Arg1,
  IN UINTN      Arg2,
  IN UINTN      Arg3,
  IN UINTN      Arg4,
  IN UINTN      Arg5,
  IN UINTN      Fid,
  IN UINTN      Ext
  );

#endif
