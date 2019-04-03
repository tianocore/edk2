/** @file

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
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

  @retval in saved in EAX - 0 means platform initialization success.
                            other means platform initialization fail.
**/
UINT32
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

  @param TempRamInitParamPtr A data structure to hold microcode parameter. It is saved in ESP.

  @retval in saved in EAX - 0 means Microcode is loaded successfully.
                            other means Microcode is not loaded successfully.
**/
UINT32
EFIAPI
LoadMicrocode (
  IN FSP_TEMP_RAM_INIT_PARAMS  *TempRamInitParamPtr
  );

/**
  This function initializes the CAR.

  This function must be in ASM file, because stack is not established yet.

  The callee should not use XMM6/XMM7.
  The return address is saved in MM7.

  @param TempRamInitParamPtr A data structure to hold microcode parameter. It is saved in ESP.

  @retval in saved in EAX - 0 means CAR initialization success.
                            other means CAR initialization fail.
**/
UINT32
EFIAPI
SecCarInit (
  IN FSP_TEMP_RAM_INIT_PARAMS  *TempRamInitParamPtr
  );

/**
  This function check the signture of UPD.

  @param[in]  ApiIdx           Internal index of the FSP API.
  @param[in]  ApiParam         Parameter of the FSP API.

**/
EFI_STATUS
EFIAPI
FspUpdSignatureCheck (
  IN UINT32   ApiIdx,
  IN VOID     *ApiParam
  );

#endif
