/** @file
  Include file for definitions in the Intel Platform Innovation Framework for EFI
  Driver Execution Environment Core Interface Specification (DXE CIS) Version 0.91.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _DXECIS_H_
#define _DXECIS_H_

#include <Protocol/StatusCode.h>

/**
  Functions of this type are used with the Framework MP Services Protocol and
  the  SMM Services Table to execute a procedure on enabled APs.  The context
  the AP should use durng execution is specified by Buffer.

  @param[in]  Buffer   The pointer to the procedure's argument.

**/
typedef
VOID
(EFIAPI *FRAMEWORK_EFI_AP_PROCEDURE)(
  IN  VOID  *Buffer
  );

///
/// The Framework EFI Runtime Services Table as an extension to the EFI 1.10 Runtime Services Table.
///
typedef struct {
  //
  // Table header for the Framework EFI Runtime Services Table
  //
  EFI_TABLE_HEADER                Hdr;
  //
  // Time services
  //
  EFI_GET_TIME                    GetTime;
  EFI_SET_TIME                    SetTime;
  EFI_GET_WAKEUP_TIME             GetWakeupTime;
  EFI_SET_WAKEUP_TIME             SetWakeupTime;
  //
  // Virtual memory services
  //
  EFI_SET_VIRTUAL_ADDRESS_MAP     SetVirtualAddressMap;
  EFI_CONVERT_POINTER             ConvertPointer;
  //
  // Variable services
  //
  EFI_GET_VARIABLE                GetVariable;
  EFI_GET_NEXT_VARIABLE_NAME      GetNextVariableName;
  EFI_SET_VARIABLE                SetVariable;
  //
  // Misc
  //
  EFI_GET_NEXT_HIGH_MONO_COUNT    GetNextHighMonotonicCount;
  EFI_RESET_SYSTEM                ResetSystem;
  ///
  /// A Framework extension to the EFI 1.10 runtime table.
  /// It was moved to a protocol to avoid conflict with UEFI 2.0.
  ///
  EFI_REPORT_STATUS_CODE          ReportStatusCode;
} FRAMEWORK_EFI_RUNTIME_SERVICES;

///
/// The Framework EFI Boot Services Table. Complies with the DxeCis specification.
///
typedef struct {
  ///
  /// The table header for the EFI Boot Services Table.
  ///
  EFI_TABLE_HEADER                              Hdr;

  //
  // Task Priority Services
  //
  EFI_RAISE_TPL                                 RaiseTPL;
  EFI_RESTORE_TPL                               RestoreTPL;

  //
  // Memory Services
  //
  EFI_ALLOCATE_PAGES                            AllocatePages;
  EFI_FREE_PAGES                                FreePages;
  EFI_GET_MEMORY_MAP                            GetMemoryMap;
  EFI_ALLOCATE_POOL                             AllocatePool;
  EFI_FREE_POOL                                 FreePool;

  //
  // Event & Timer Services
  //
  EFI_CREATE_EVENT                              CreateEvent;
  EFI_SET_TIMER                                 SetTimer;
  EFI_WAIT_FOR_EVENT                            WaitForEvent;
  EFI_SIGNAL_EVENT                              SignalEvent;
  EFI_CLOSE_EVENT                               CloseEvent;
  EFI_CHECK_EVENT                               CheckEvent;

  //
  // Protocol Handler Services
  //
  EFI_INSTALL_PROTOCOL_INTERFACE                InstallProtocolInterface;
  EFI_REINSTALL_PROTOCOL_INTERFACE              ReinstallProtocolInterface;
  EFI_UNINSTALL_PROTOCOL_INTERFACE              UninstallProtocolInterface;
  EFI_HANDLE_PROTOCOL                           HandleProtocol;
  EFI_HANDLE_PROTOCOL                           PcHandleProtocol;
  EFI_REGISTER_PROTOCOL_NOTIFY                  RegisterProtocolNotify;
  EFI_LOCATE_HANDLE                             LocateHandle;
  EFI_LOCATE_DEVICE_PATH                        LocateDevicePath;
  EFI_INSTALL_CONFIGURATION_TABLE               InstallConfigurationTable;

  //
  // Image Services
  //
  EFI_IMAGE_LOAD                                LoadImage;
  EFI_IMAGE_START                               StartImage;
  EFI_EXIT                                      Exit;
  EFI_IMAGE_UNLOAD                              UnloadImage;
  EFI_EXIT_BOOT_SERVICES                        ExitBootServices;

  //
  // Miscellaneous Services
  //
  EFI_GET_NEXT_MONOTONIC_COUNT                  GetNextMonotonicCount;
  EFI_STALL                                     Stall;
  EFI_SET_WATCHDOG_TIMER                        SetWatchdogTimer;

  //
  // DriverSupport Services
  //
  EFI_CONNECT_CONTROLLER                        ConnectController;
  EFI_DISCONNECT_CONTROLLER                     DisconnectController;

  //
  // Open and Close Protocol Services
  //
  EFI_OPEN_PROTOCOL                             OpenProtocol;
  EFI_CLOSE_PROTOCOL                            CloseProtocol;
  EFI_OPEN_PROTOCOL_INFORMATION                 OpenProtocolInformation;

  //
  // Library Services
  //
  EFI_PROTOCOLS_PER_HANDLE                      ProtocolsPerHandle;
  EFI_LOCATE_HANDLE_BUFFER                      LocateHandleBuffer;
  EFI_LOCATE_PROTOCOL                           LocateProtocol;
  EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES      InstallMultipleProtocolInterfaces;
  EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES    UninstallMultipleProtocolInterfaces;

  //
  // 32-bit CRC Services
  //
  EFI_CALCULATE_CRC32                           CalculateCrc32;

  //
  // Miscellaneous Services
  //
  EFI_COPY_MEM                                  CopyMem;
  EFI_SET_MEM                                   SetMem;
} FRAMEWORK_EFI_BOOT_SERVICES;

#define EFI_EVENT_RUNTIME_CONTEXT       0x20000000
#define EFI_EVENT_NOTIFY_SIGNAL_ALL     0x00000400
#define EFI_EVENT_SIGNAL_READY_TO_BOOT  0x00000203
#define EFI_EVENT_SIGNAL_LEGACY_BOOT    0x00000204

#endif
