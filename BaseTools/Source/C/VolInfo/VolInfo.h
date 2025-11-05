/** @file
Local Definitions for the VolInfo utility

Copyright (c) 1999 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

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

#define EFI_SECTION_LAST_LEAF_SECTION_TYPE  0x1C
#define EFI_SECTION_LAST_SECTION_TYPE       0x1C

#define OPENSSL_COMMAND_FORMAT_STRING       "%s sha1 -out %s %s"
#define EXTRACT_COMMAND_FORMAT_STRING       "%s -d -o %s %s"

#endif
