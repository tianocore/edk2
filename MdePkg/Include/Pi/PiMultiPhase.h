/** @file
  Include file matches things in PI for multiple module types.

  Copyright (c) 2006 - 2009, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  @par Revision Reference:
  These elements are defined in UEFI Platform Initialization Specification 1.2 

**/

#ifndef __PI_MULTIPHASE_H__
#define __PI_MULTIPHASE_H__

#include <Pi/PiFirmwareVolume.h>
#include <Pi/PiFirmwareFile.h>
#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>
#include <Pi/PiDependency.h>
#include <Pi/PiStatusCode.h>

/**
  Produces an error code in the range reserved for use by the Platform Initialization
  Architecture Specification.

  The supported 32-bit range is 0xA0000000-0xBFFFFFFF 
  The supported 64-bit range is 0xA000000000000000-0xBFFFFFFFFFFFFFFF 

  @param  StatusCode    The status code value to convert into a warning code.  
                        StatusCode must be in the range 0x00000000..0x1FFFFFFF.

  @return The value specified by StatusCode in the PI reserved range.

**/
#define DXE_ERROR(StatusCode)  (MAX_BIT | (MAX_BIT >> 2) | StatusCode)

///
/// If this value is returned by an EFI image, then the image should be unloaded.
///
#define EFI_REQUEST_UNLOAD_IMAGE  DXE_ERROR (1)

///
/// If this value is returned by an API, it means the capability is not yet 
/// installed/available/ready to use.
///
#define EFI_NOT_AVAILABLE_YET     DXE_ERROR (2)

///
/// Success and warning codes reserved for use by PI
/// Supported 32-bit range is 0x20000000-0x3fffffff
/// Supported 64-bit range is 0x2000000000000000-0x3fffffffffffffff
///
#define PI_ENCODE_WARNING(a)                ((MAX_BIT >> 2) | (a))

///
/// Error codes reserved for use by PI
/// Supported 32-bit range is 0xa0000000-0xbfffffff
/// Supported 64-bit range is 0xa000000000000000-0xbfffffffffffffff
///
#define PI_ENCODE_ERROR(a)                  (MAX_BIT | (MAX_BIT >> 2) | (a))

/// 
/// Return status codes defined in SMM CIS
/// 
#define EFI_INTERRUPT_PENDING               PI_ENCODE_ERROR (0)

#define EFI_WARN_INTERRUPT_SOURCE_PENDING   PI_ENCODE_WARNING (0)
#define EFI_WARN_INTERRUPT_SOURCE_QUIESCED  PI_ENCODE_WARNING (1)

///
/// Bitmask of values for Authentication Status.
/// Authentication Status is returned from EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL 
/// and the EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI
///
/// xx00 Image was not signed.
/// xxx1 Platform security policy override. Assumes same meaning as 0010 (the image was signed, the
///      signature was tested, and the signature passed authentication test).
/// 0010 Image was signed, the signature was tested, and the signature passed authentication test.
/// 0110 Image was signed and the signature was not tested.
/// 1010 Image was signed, the signature was tested, and the signature failed the authentication test.
///
///@{
#define EFI_AUTH_STATUS_PLATFORM_OVERRIDE   0x01
#define EFI_AUTH_STATUS_IMAGE_SIGNED        0x02
#define EFI_AUTH_STATUS_NOT_TESTED          0x04
#define EFI_AUTH_STATUS_TEST_FAILED         0x08
#define EFI_AUTH_STATUS_ALL                 0x0f
///@}

#endif
