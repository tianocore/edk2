/** @file
  Emulator Thunk to abstract OS services from pure EFI code

  Copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>
  
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EMU_PEI_SERVICE_TABLE_UPDATE_PPI_H__
#define __EMU_PEI_SERVICE_TABLE_UPDATE_PPI_H__

#define _EMU_PEI_SERVICE_TABLE_UPDATE_PPI_GUID  \
 { 0xFA93020C, 0x6CDF, 0x1946, { 0x86, 0x35, 0x72, 0xCB, 0x51, 0x9E, 0xCF, 0xFD } }




extern EFI_GUID gEmuPeiServicesTableUpdatePpiGuid;

#endif
