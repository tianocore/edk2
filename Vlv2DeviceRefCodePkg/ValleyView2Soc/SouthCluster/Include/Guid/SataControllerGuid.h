/*++

Copyright (c)  2009  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



Module Name:

  SataControllerGuid.h

Abstract:

  GUID for use in describing SataController

--*/
#ifndef _SERIAL_ATA_CONTROLLER_GUID_H_
#define _SERIAL_ATA_CONTROLLER_GUID_H_

#ifdef ECP_FLAG
#define PCH_SATA_CONTROLLER_DRIVER_GUID \
  { \
    0xbb929da9, 0x68f7, 0x4035, 0xb2, 0x2c, 0xa3, 0xbb, 0x3f, 0x23, 0xda, 0x55 \
  }
#else
#define PCH_SATA_CONTROLLER_DRIVER_GUID \
  {\
    0xbb929da9, 0x68f7, 0x4035, 0xb2, 0x2c, 0xa3, 0xbb, 0x3f, 0x23, 0xda, 0x55 \}

#endif

extern EFI_GUID gSataControllerDriverGuid;
#endif
