/** @file
  Guid definition for Boot Maintainence Formset.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef __HII_BOOT_MAINTENANCE_FORMSET_H__
#define __HII_BOOT_MAINTENANCE_FORMSET_H__

///
/// Guid define to group the item show on the Boot Menaintenance Manager Menu.
///
#define EFI_IFR_BOOT_MAINTENANCE_GUID \
  { 0xb2dedc91, 0xd59f, 0x48d2, { 0x89, 0x8a, 0x12, 0x49, 0xc, 0x74, 0xa4, 0xe0 } }


extern EFI_GUID gEfiIfrBootMaintenanceGuid;

#endif
