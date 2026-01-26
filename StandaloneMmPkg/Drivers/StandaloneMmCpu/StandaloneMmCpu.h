/** @file
  Private header with declarations and definitions specific to the MM Standalone
  CPU driver

  Copyright (c) 2017 - 2021, Arm Limited. All rights reserved.
  Copyright (c) 2025, Rivos Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_CPU_DRIVER_H_
#define MM_CPU_DRIVER_H_

#include <Protocol/MmCommunication2.h>
#include <Protocol/MmConfiguration.h>
#include <Protocol/MmCpu.h>
#include <Guid/MpInformation.h>
#include <Protocol/PiMmCpuDriverEp.h>

//
// CPU driver initialization specific declarations
//
extern EFI_MM_SYSTEM_TABLE  *mMmst;

//
// CPU State Save protocol specific declarations
//
extern EFI_MM_CPU_PROTOCOL  mMmCpuState;

//
// MM event handling specific declarations
//
extern EFI_MM_COMMUNICATE_HEADER      **PerCpuGuidedEventContext;
extern EFI_MMRAM_DESCRIPTOR           mSCommBuffer;
extern MP_INFORMATION_HOB_DATA        *mMpInformationHobData;
extern EFI_MM_CONFIGURATION_PROTOCOL  mMmConfig;

/**
  The PI Standalone MM entry point for the CPU driver.

  @param  [in] EventId            The event Id.
  @param  [in] CpuNumber          The CPU number.
  @param  [in] NsCommBufferAddr   Address of the NS common buffer. Optional

  @retval   EFI_SUCCESS             Success.
  @retval   EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval   EFI_ACCESS_DENIED       Access not permitted.
  @retval   EFI_OUT_OF_RESOURCES    Out of resources.
  @retval   EFI_UNSUPPORTED         Operation not supported.
**/
EFI_STATUS
PiMmStandaloneMmCpuDriverEntry (
  IN UINTN  EventId,
  IN UINTN  CpuNumber,
  IN UINTN  NsCommBufferAddr // Optional parameter
  );

#endif /* MM_CPU_DRIVER_H_ */
