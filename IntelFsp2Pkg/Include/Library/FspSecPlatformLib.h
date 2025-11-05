/** @file

  Copyright (c) 2015 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FSP_SEC_PLATFORM_LIB_H_
#define _FSP_SEC_PLATFORM_LIB_H_

/**
  This function performs platform level initialization.

  This function must be in ASM file, because stack is not established yet.
  This function is optional. If a library instance does not provide this function, the default empty one will be used.

  The callee should not use XMM6/XMM7.
  The return address is saved in MM7.

  @retval in saved in EAX/RAX - 0 means platform initialization success.
                            other means platform initialization fail.
**/
UINTN
EFIAPI
SecPlatformInit (
  VOID
  );

/**
  This function loads Microcode.

  This function must be in ASM file, because stack is not established yet.
  This function is optional. If a library instance does not provide this function, the default one will be used.

  The callee should not use XMM6/XMM7.
  The return address is saved in MM7.

  @param[in] FsptUpdDataPtr     Address pointer to the FSPT_UPD data structure. It is saved in ESP.

  @retval in saved in EAX/RAX - 0 means Microcode is loaded successfully.
                            other means Microcode is not loaded successfully.
**/
UINTN
EFIAPI
LoadMicrocode (
  IN  VOID  *FsptUpdDataPtr
  );

/**
  This function initializes the CAR.

  This function must be in ASM file, because stack is not established yet.

  The callee should not use XMM6/XMM7.
  The return address is saved in MM7.

  @param[in] FsptUpdDataPtr     Address pointer to the FSPT_UPD data structure. It is saved in ESP.

  @retval in saved in EAX/RAX - 0 means CAR initialization success.
                            other means CAR initialization fail.
**/
UINTN
EFIAPI
SecCarInit (
  IN  VOID  *FsptUpdDataPtr
  );

/**
  This function check the signature of UPD.

  @param[in]  ApiIdx           Internal index of the FSP API.
  @param[in]  ApiParam         Parameter of the FSP API.

**/
EFI_STATUS
EFIAPI
FspUpdSignatureCheck (
  IN UINT32  ApiIdx,
  IN VOID    *ApiParam
  );

/**
  This function handles FspMultiPhaseSiInitApi.
  Starting from FSP 2.4 this function is obsolete and FspMultiPhaseSiInitApiHandlerV2 is the replacement.

  @param[in]  ApiIdx           Internal index of the FSP API.
  @param[in]  ApiParam         Parameter of the FSP API.

**/
EFI_STATUS
EFIAPI
FspMultiPhaseSiInitApiHandler (
  IN UINT32  ApiIdx,
  IN VOID    *ApiParam
  );

/**
  FSP MultiPhase Platform Get Number Of Phases Function.

  Allows an FSP binary to dynamically update the number of phases at runtime.
  For example, UPD settings could negate the need to enter the multi-phase flow
  in certain scenarios. If this function returns FALSE, the default number of phases
  provided by PcdMultiPhaseNumberOfPhases will be returned to the bootloader instead.

  @param[in] ApiIdx                  - Internal index of the FSP API.
  @param[in] NumberOfPhasesSupported - How many phases are supported by current FSP Component.

  @retval  TRUE  - NumberOfPhases are modified by Platform during runtime.
  @retval  FALSE - The Default build time NumberOfPhases should be used.

**/
BOOLEAN
EFIAPI
FspMultiPhasePlatformGetNumberOfPhases (
  IN     UINTN   ApiIdx,
  IN OUT UINT32  *NumberOfPhasesSupported
  );

#endif
