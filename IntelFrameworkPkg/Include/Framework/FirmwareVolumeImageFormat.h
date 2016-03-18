/** @file
  This file defines the data structures that are architecturally defined for file
  images loaded via the FirmwareVolume protocol.  The Firmware Volume specification
  is the basis for these definitions.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  @par Revision Reference:
  These definitions are from the Firmware Volume Spec 0.9.

**/

#ifndef __FIRMWARE_VOLUME_IMAGE_FORMAT_H__
#define __FIRMWARE_VOLUME_IMAGE_FORMAT_H__

//
// Bit values for AuthenticationStatus
//
#define EFI_AGGREGATE_AUTH_STATUS_PLATFORM_OVERRIDE 0x000001
#define EFI_AGGREGATE_AUTH_STATUS_IMAGE_SIGNED      0x000002
#define EFI_AGGREGATE_AUTH_STATUS_NOT_TESTED        0x000004
#define EFI_AGGREGATE_AUTH_STATUS_TEST_FAILED       0x000008
#define EFI_AGGREGATE_AUTH_STATUS_ALL               0x00000f

#define EFI_LOCAL_AUTH_STATUS_PLATFORM_OVERRIDE     0x010000
#define EFI_LOCAL_AUTH_STATUS_IMAGE_SIGNED          0x020000
#define EFI_LOCAL_AUTH_STATUS_NOT_TESTED            0x040000
#define EFI_LOCAL_AUTH_STATUS_TEST_FAILED           0x080000
#define EFI_LOCAL_AUTH_STATUS_ALL                   0x0f0000

#endif
