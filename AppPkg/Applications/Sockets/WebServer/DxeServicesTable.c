/** @file
    Display the DXE services table

    Copyright (c)  2011 - 2012 Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials
    are licensed and made available under the terms and conditions of the BSD License
    which accompanies this distribution. The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
--*/
#include <WebServer.h>
#include <Guid/DxeServices.h>
#include <Pi/PiDxeCis.h>

/**
  Respond with the DXE services table

  @param[in]  SocketFD      The socket's file descriptor to add to the list.
  @param[in]  pPort         The WSDT_PORT structure address
  @param[out] pbDone        Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
DxeServicesTablePage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  EFI_DXE_SERVICES * pDS;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Send the DXE services page
  //
  for ( ; ; ) {
    //
    //  Get the DXE services table
    //
    Status = EfiGetSystemConfigurationTable (&gEfiDxeServicesTableGuid, (VOID **) &pDS);
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Send the page and table header
    //
    Status = TableHeader ( SocketFD, pPort, L"DXE Services Table", pDS );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    ///
    /// The table header for the DXE Services Table.
    /// This header contains the DXE_SERVICES_SIGNATURE and DXE_SERVICES_REVISION values.
    ///
    Status = EfiTableHeader ( SocketFD,
                              pPort,
                              &pDS->Hdr );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    // Global Coherency Domain Services
    //
    Status = RowPointer ( SocketFD,
                          pPort,
                          "AddMemorySpace",
                          (VOID *)pDS->AddMemorySpace,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "AllocateMemorySpace",
                          (VOID *)pDS->AllocateMemorySpace,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "FreeMemorySpace",
                          (VOID *)pDS->FreeMemorySpace,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "RemoveMemorySpace",
                          (VOID *)pDS->RemoveMemorySpace,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "GetMemorySpaceDescriptor",
                          (VOID *)pDS->GetMemorySpaceDescriptor,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "SetMemorySpaceAttributes",
                          (VOID *)pDS->SetMemorySpaceAttributes,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "GetMemorySpaceMap",
                          (VOID *)pDS->GetMemorySpaceMap,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "AddIoSpace",
                          (VOID *)pDS->AddIoSpace,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "AllocateIoSpace",
                          (VOID *)pDS->AllocateIoSpace,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "FreeIoSpace",
                          (VOID *)pDS->FreeIoSpace,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "RemoveIoSpace",
                          (VOID *)pDS->RemoveIoSpace,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "GetIoSpaceDescriptor",
                          (VOID *)pDS->GetIoSpaceDescriptor,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "GetIoSpaceMap",
                          (VOID *)pDS->GetIoSpaceMap,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    // Dispatcher Services
    //
    Status = RowPointer ( SocketFD,
                          pPort,
                          "Dispatch",
                          (VOID *)pDS->Dispatch,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "Schedule",
                          (VOID *)pDS->Schedule,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "Trust",
                          (VOID *)pDS->Trust,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    // Service to process a single firmware volume found in a capsule
    //
    Status = RowPointer ( SocketFD,
                          pPort,
                          "ProcessFirmwareVolume",
                          (VOID *)pDS->ProcessFirmwareVolume,
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
