/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  WinNtIo.c

Abstract:

  This protocol allows an EFI driver (DLL) in the NT emulation envirnment
  to make Win32 API calls.

--*/

#include "EfiWinNt.h"
#include EFI_PROTOCOL_DEFINITION (WinNtIo)

EFI_GUID  gEfiWinNtIoProtocolGuid = EFI_WIN_NT_IO_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiWinNtIoProtocolGuid, "EFI Win NT I/O Protocol", "Win32 API I/O protocol");

EFI_GUID  gEfiWinNtVirtualDisksGuid   = EFI_WIN_NT_VIRTUAL_DISKS_GUID;
EFI_GUID  gEfiWinNtPhysicalDisksGuid  = EFI_WIN_NT_PHYSICAL_DISKS_GUID;
EFI_GUID  gEfiWinNtFileSystemGuid     = EFI_WIN_NT_FILE_SYSTEM_GUID;
EFI_GUID  gEfiWinNtSerialPortGuid     = EFI_WIN_NT_SERIAL_PORT_GUID;
EFI_GUID  gEfiWinNtUgaGuid            = EFI_WIN_NT_UGA_GUID;
EFI_GUID  gEfiWinNtGopGuid            = EFI_WIN_NT_GOP_GUID;
EFI_GUID  gEfiWinNtConsoleGuid        = EFI_WIN_NT_CONSOLE_GUID;
EFI_GUID  gEfiWinNtMemoryGuid         = EFI_WIN_NT_MEMORY_GUID;
EFI_GUID  gEfiWinNtCPUModelGuid       = EFI_WIN_NT_CPU_MODEL_GUID;
EFI_GUID  gEfiWinNtCPUSpeedGuid       = EFI_WIN_NT_CPU_SPEED_GUID;
EFI_GUID  gEfiWinNtPassThroughGuid    = EFI_WIN_NT_PASS_THROUGH_GUID;
