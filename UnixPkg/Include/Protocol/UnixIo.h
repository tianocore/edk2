/*++

Copyright (c) 2004 - 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  UnixIo.h

Abstract:

--*/

#ifndef _UNIX_IO_H_
#define _UNIX_IO_H_

#define EFI_UNIX_IO_PROTOCOL_GUID \
  { \
    0xf2e23f54, 0x8985, 0x11db, {0xac, 0x79, 0x00, 0x40, 0xd0, 0x2b, 0x18, 0x35 } \
  }

typedef struct {
  EFI_UNIX_THUNK_PROTOCOL  *UnixThunk;
  EFI_GUID                  *TypeGuid;
  UINT16                    *EnvString;
  UINT16                    InstanceNumber;
} EFI_UNIX_IO_PROTOCOL;

extern EFI_GUID gEfiUnixIoProtocolGuid;

//
// The following GUIDs are used in EFI_UNIX_IO_PROTOCOL_GUID
// Device paths. They map 1:1 with UNIX envirnment variables. The variables
// define what virtual hardware the emulator/UnixBusDriver will produce.
//
//
// EFI_UNIX_VIRTUAL_DISKS
//
#define EFI_UNIX_VIRTUAL_DISKS_GUID \
  { \
    0xf2ba331a, 0x8985, 0x11db, {0xa4, 0x06, 0x00, 0x40, 0xd0, 0x2b, 0x18, 0x35 } \
  }

extern EFI_GUID gEfiUnixVirtualDisksGuid;

//
// EFI_UNIX_PHYSICAL_DISKS
//
#define EFI_UNIX_PHYSICAL_DISKS_GUID \
  { \
    0xf2bdcc96, 0x8985, 0x11db, {0x87, 0x19, 0x00, 0x40, 0xd0, 0x2b, 0x18, 0x35 } \
  }

extern EFI_GUID gEfiUnixPhysicalDisksGuid;

//
// EFI_UNIX_FILE_SYSTEM
//
#define EFI_UNIX_FILE_SYSTEM_GUID \
  { \
    0xf2c16b9e, 0x8985, 0x11db, {0x92, 0xc8, 0x00, 0x40, 0xd0, 0x2b, 0x18, 0x35 } \
  }

extern EFI_GUID gEfiUnixFileSystemGuid;

//
// EFI_WIN_NT_SERIAL_PORT
//
#define EFI_UNIX_SERIAL_PORT_GUID \
  { \
    0x6d3a727d, 0x66c8, 0x4d19, {0x87, 0xe6, 0x2, 0x15, 0x86, 0x14, 0x90, 0xf3} \
  }

extern EFI_GUID gEfiUnixSerialPortGuid;

//
// EFI_UNIX_UGA
//
#define EFI_UNIX_UGA_GUID \
  { \
    0xf2c8b80e, 0x8985, 0x11db, {0x93, 0xf1, 0x00, 0x40, 0xd0, 0x2b, 0x18, 0x35 } \
  }

extern EFI_GUID gEfiUnixUgaGuid;

//
// EFI_UNIX_GOP
//
#define EFI_UNIX_GOP_GUID \
  { \
    0xbace07c2, 0x8987, 0x11db, {0xa5, 0x9a, 0x00, 0x40, 0xd0, 0x2b, 0x18, 0x35 } \
  }

extern EFI_GUID gEfiUnixGopGuid;

//
// EFI_UNIX_CONSOLE
//
#define EFI_UNIX_CONSOLE_GUID \
  { \
    0xf2cc5d06, 0x8985, 0x11db, {0xbb, 0x19, 0x00, 0x40, 0xd0, 0x2b, 0x18, 0x35 } \
  }

extern EFI_GUID gEfiUnixConsoleGuid;

//
// EFI_UNIX_MEMORY
//
#define EFI_UNIX_MEMORY_GUID \
  { \
    0xf2d006cc, 0x8985, 0x11db, {0xa4, 0x72, 0x00, 0x40, 0xd0, 0x2b, 0x18, 0x35 } \
  }

extern EFI_GUID gEfiUnixMemoryGuid;

//
// EFI_UNIX_CPU_MODEL
//
#define EFI_UNIX_CPU_MODEL_GUID \
  { \
    0xf2d3b330, 0x8985, 0x11db, {0x8a, 0xa3, 0x00, 0x40, 0xd0, 0x2b, 0x18, 0x35 } \
  }

extern EFI_GUID gEfiUnixCPUModelGuid;

//
// EFI_UNIX_CPU_SPEED
//
#define EFI_UNIX_CPU_SPEED_GUID \
  { \
    0xf2d74e5a, 0x8985, 0x11db, {0x97, 0x05, 0x00, 0x40, 0xd0, 0x2b, 0x18, 0x35 } \
  }

extern EFI_GUID gEfiUnixCPUSpeedGuid;

#endif
