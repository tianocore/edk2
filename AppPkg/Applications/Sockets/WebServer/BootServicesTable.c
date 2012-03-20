/**
  @file
  Display the boot services table

  Copyright (c) 2011-2012, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <WebServer.h>

/**
  Respond with the boot services table

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
BootServicesTablePage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );
  
  //
  //  Send the boot services page
  //
  for ( ; ; ) {
    //
    //  Send the page and table header
    //
    Status = TableHeader ( SocketFD, pPort, L"Boot Services Table", gBS );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    ///
    /// The table header for the EFI Boot Services Table.
    ///
    Status = EfiTableHeader ( SocketFD,
                              pPort,
                              &gBS->Hdr );
    if ( EFI_ERROR ( Status )) {
      break;
    }
  
    //
    // Task Priority Services
    //
    Status = RowPointer ( SocketFD,
                          pPort,
                          "RaiseTPL",
                          (CONST VOID *)gBS->RaiseTPL,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "RestoreTPL",
                          (CONST VOID *)gBS->RestoreTPL,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
  
    //
    // Memory Services
    //
    Status = RowPointer ( SocketFD,
                          pPort,
                          "AllocatePages",
                          (CONST VOID *)gBS->AllocatePages,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "FreePages",
                          (CONST VOID *)gBS->FreePages,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "GetMemoryMap",
                          (CONST VOID *)gBS->GetMemoryMap,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "AllocatePool",
                          (CONST VOID *)gBS->AllocatePool,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "FreePool",
                          (CONST VOID *)gBS->FreePool,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
  
    //
    // Event & Timer Services
    //
    Status = RowPointer ( SocketFD,
                          pPort,
                          "CreateEvent",
                          (CONST VOID *)gBS->CreateEvent,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "SetTimer",
                          (CONST VOID *)gBS->SetTimer,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "WaitForEvent",
                          (CONST VOID *)gBS->WaitForEvent,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "SignalEvent",
                          (CONST VOID *)gBS->SignalEvent,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "CloseEvent",
                          (CONST VOID *)gBS->CloseEvent,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "CheckEvent",
                          (CONST VOID *)gBS->CheckEvent,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
  
    //
    // Protocol Handler Services
    //
    Status = RowPointer ( SocketFD,
                          pPort,
                          "InstallProtocolInterface",
                          (CONST VOID *)gBS->InstallProtocolInterface,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "ReinstallProtocolInterface",
                          (CONST VOID *)gBS->ReinstallProtocolInterface,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "UninstallProtocolInterface",
                          (CONST VOID *)gBS->UninstallProtocolInterface,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "HandleProtocol",
                          (CONST VOID *)gBS->HandleProtocol,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "Reserved",
                          (CONST VOID *)gBS->Reserved,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "RegisterProtocolNotify",
                          (CONST VOID *)gBS->RegisterProtocolNotify,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "LocateHandle",
                          (CONST VOID *)gBS->LocateHandle,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "LocateDevicePath",
                          (CONST VOID *)gBS->LocateDevicePath,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "InstallConfigurationTable",
                          (CONST VOID *)gBS->InstallConfigurationTable,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
  
    //
    // Image Services
    //
    Status = RowPointer ( SocketFD,
                          pPort,
                          "LoadImage",
                          (CONST VOID *)gBS->LoadImage,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "StartImage",
                          (CONST VOID *)gBS->StartImage,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "Exit",
                          (CONST VOID *)gBS->Exit,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "UnloadImage",
                          (CONST VOID *)gBS->UnloadImage,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "ExitBootServices",
                          (CONST VOID *)gBS->ExitBootServices,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
  
    //
    // Miscellaneous Services
    //
    Status = RowPointer ( SocketFD,
                          pPort,
                          "GetNextMonotonicCount",
                          (CONST VOID *)gBS->GetNextMonotonicCount,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "Stall",
                          (CONST VOID *)gBS->Stall,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "SetWatchdogTimer",
                          (CONST VOID *)gBS->SetWatchdogTimer,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
  
    //
    // DriverSupport Services
    //
    Status = RowPointer ( SocketFD,
                          pPort,
                          "ConnectController",
                          (CONST VOID *)gBS->ConnectController,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "DisconnectController",
                          (CONST VOID *)gBS->DisconnectController,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
  
    //
    // Open and Close Protocol Services
    //
    Status = RowPointer ( SocketFD,
                          pPort,
                          "OpenProtocol",
                          (CONST VOID *)gBS->OpenProtocol,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "CloseProtocol",
                          (CONST VOID *)gBS->CloseProtocol,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "OpenProtocolInformation",
                          (CONST VOID *)gBS->OpenProtocolInformation,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
  
    //
    // Library Services
    //
    Status = RowPointer ( SocketFD,
                          pPort,
                          "ProtocolsPerHandle",
                          (CONST VOID *)gBS->ProtocolsPerHandle,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "LocateHandleBuffer",
                          (CONST VOID *)gBS->LocateHandleBuffer,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "LocateProtocol",
                          (CONST VOID *)gBS->LocateProtocol,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "InstallMultipleProtocolInterfaces",
                          (CONST VOID *)gBS->InstallMultipleProtocolInterfaces,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "UninstallMultipleProtocolInterfaces",
                          (CONST VOID *)gBS->UninstallMultipleProtocolInterfaces,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
  
    //
    // 32-bit CRC Services
    //
    Status = RowPointer ( SocketFD,
                          pPort,
                          "CalculateCrc32",
                          (CONST VOID *)gBS->CalculateCrc32,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
  
    //
    // Miscellaneous Services
    //
    Status = RowPointer ( SocketFD,
                          pPort,
                          "CopyMem",
                          (CONST VOID *)gBS->CopyMem,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "SetMem",
                          (CONST VOID *)gBS->SetMem,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "CreateEventEx",
                          (CONST VOID *)gBS->CreateEventEx,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Build the table trailer
    //
    Status = TableTrailer ( SocketFD,
                            pPort,
                            pbDone );
    break;
  }
    
  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}
