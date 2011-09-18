/** @file
  GUIDs used as HII FormSet and HII Package list GUID in PlatDriOverride driver. 
  
Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PLATFORM_DRIVER_OVERRIDE_HII_GUID_H__
#define __PLATFORM_DRIVER_OVERRIDE_HII_GUID_H__

#define PLAT_OVER_MNGR_GUID \
  { \
    0x8614567d, 0x35be, 0x4415, {0x8d, 0x88, 0xbd, 0x7d, 0xc, 0x9c, 0x70, 0xc0} \
  }

extern EFI_GUID gPlatformOverridesManagerGuid;

#endif
