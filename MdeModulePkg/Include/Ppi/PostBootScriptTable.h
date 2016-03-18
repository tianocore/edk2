/** @file
  POST BootScript Table PPI definition.

  This PPI is used to be notification after boot script table execution.

  Copyright (c) 2010, Intel Corporation. All rights reserved. <BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions
  of the BSD License which accompanies this distribution.  The
  full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PEI_POST_BOOT_SCRIPT_TABLE_H_
#define _PEI_POST_BOOT_SCRIPT_TABLE_H_

#define PEI_POST_BOOT_SCRIPT_TABLE_PPI_GUID  \
  {0x88c9d306, 0x900, 0x4eb5, 0x82, 0x60, 0x3e, 0x2d, 0xbe, 0xda, 0x1f, 0x89};

extern EFI_GUID   gPeiPostScriptTablePpiGuid;

#endif
