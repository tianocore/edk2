/** @file
*
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef _FDT_LOAD_LIB_H_
#define _FDT_LOAD_LIB_H_

/**
  Load and Install FDT from Semihosting

  @param Filename   Name of the file to load from semihosting

  @return EFI_SUCCESS           Fdt Blob was successfully installed into the configuration table
                                from semihosting
  @return EFI_NOT_FOUND         Fail to locate the file in semihosting
  @return EFI_OUT_OF_RESOURCES  Fail to allocate memory to contain the blob
**/
EFI_STATUS
InstallFdtFromSemihosting (
  IN  CONST CHAR16*   FileName
  );

/**
  Load and Install FDT from Firmware Volume

  @param Filename   Guid of the FDT blob to load from firmware volume

  @return EFI_SUCCESS           Fdt Blob was successfully installed into the configuration table
                                from firmware volume
  @return EFI_NOT_FOUND         Failed to locate the file in firmware volume
  @return EFI_OUT_OF_RESOURCES  Failed to allocate memory to contain the blob
**/
EFI_STATUS
InstallFdtFromFv (
  IN  CONST EFI_GUID *FileName
  );

#endif
