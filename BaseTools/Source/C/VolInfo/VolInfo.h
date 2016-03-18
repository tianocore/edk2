/** @file
Local Definitions for the VolInfo utility

Copyright (c) 1999 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _VOLINFO_H_
#define _VOLINFO_H_ 1

#define PI_SPECIFICATION_VERSION  0x00010000

#define EFI_DEP_BEFORE    0x00
#define EFI_DEP_AFTER     0x01
#define EFI_DEP_PUSH      0x02
#define EFI_DEP_AND       0x03
#define EFI_DEP_OR        0x04
#define EFI_DEP_NOT       0x05
#define EFI_DEP_TRUE      0x06
#define EFI_DEP_FALSE     0x07
#define EFI_DEP_END       0x08
#define EFI_DEP_SOR       0x09

#define EFI_SECTION_LAST_LEAF_SECTION_TYPE  0x1B
#define EFI_SECTION_LAST_SECTION_TYPE       0x1B

#endif
