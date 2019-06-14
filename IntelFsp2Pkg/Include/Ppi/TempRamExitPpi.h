/** @file
  This file defines the Silicon Temp Ram Exit PPI which implements the
  required programming steps for disabling temporary memory.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FSP_TEMP_RAM_EXIT_PPI_H_
#define _FSP_TEMP_RAM_EXIT_PPI_H_

///
/// Global ID for the FSP_TEMP_RAM_EXIT_PPI.
///
#define FSP_TEMP_RAM_EXIT_GUID \
  { \
    0xbc1cfbdb, 0x7e50, 0x42be, { 0xb4, 0x87, 0x22, 0xe0, 0xa9, 0x0c, 0xb0, 0x52 } \
  }

//
// Forward declaration for the FSP_TEMP_RAM_EXIT_PPI.
//
typedef struct _FSP_TEMP_RAM_EXIT_PPI FSP_TEMP_RAM_EXIT_PPI;

/**
  Silicon function for disabling temporary memory.
  @param[in] TempRamExitParamPtr - Pointer to the TempRamExit parameters structure.
                                   This structure is normally defined in the Integration
                                   Guide. If it is not defined in the Integration Guide,
                                   pass NULL.
  @retval EFI_SUCCESS            - FSP execution environment was initialized successfully.
  @retval EFI_INVALID_PARAMETER  - Input parameters are invalid.
  @retval EFI_UNSUPPORTED        - The FSP calling conditions were not met.
  @retval EFI_DEVICE_ERROR       - Temporary memory exit.
**/
typedef
EFI_STATUS
(EFIAPI *FSP_TEMP_RAM_EXIT) (
  IN  VOID    *TempRamExitParamPtr
  );

///
/// This PPI provides function to disable temporary memory.
///
struct _FSP_TEMP_RAM_EXIT_PPI {
  FSP_TEMP_RAM_EXIT   TempRamExit;
};

extern EFI_GUID gFspTempRamExitPpiGuid;

#endif // _FSP_TEMP_RAM_EXIT_PPI_H_
