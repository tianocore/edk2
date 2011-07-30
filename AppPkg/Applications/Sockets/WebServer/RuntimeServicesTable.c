/*++
  This file contains an 'Intel UEFI Application' and is        
  licensed for Intel CPUs and chipsets under the terms of your  
  license agreement with Intel or your vendor.  This file may   
  be modified by the user, subject to additional terms of the   
  license agreement                                             
--*/
/*++

Copyright (c)  2011 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

--*/

/** @file
  Display the runtime services table

**/

#include <WebServer.h>
#include <Library/UefiRuntimeServicesTableLib.h>

/**
  Respond with the runtime services table

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
RuntimeSservicesTablePage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );
  
  //
  //  Send the runtime services page
  //
  for ( ; ; ) {
    //
    //  Send the page and table header
    //
    Status = TableHeader ( SocketFD, pPort, L"Runtime Services Table", gRT );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    ///
    /// The table header for the EFI Runtime Services Table.
    ///
    Status = EfiTableHeader ( SocketFD,
                              pPort,
                              &gRT->Hdr );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    
    //
    // Time Services
    //
    Status = RowPointer ( SocketFD,
                          pPort,
                          "GetTime",
                          (VOID *)gRT->GetTime,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "SetTime",
                          (VOID *)gRT->SetTime,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "GetWakeupTime",
                          (VOID *)gRT->GetWakeupTime,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "SetWakeupTime",
                          (VOID *)gRT->SetWakeupTime,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    
    //
    // Virtual Memory Services
    //
    Status = RowPointer ( SocketFD,
                          pPort,
                          "SetVirtualAddressMap",
                          (VOID *)gRT->SetVirtualAddressMap,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "ConvertPointer",
                          (VOID *)gRT->ConvertPointer,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    
    //
    // Variable Services
    //
    Status = RowPointer ( SocketFD,
                          pPort,
                          "GetVariable",
                          (VOID *)gRT->GetVariable,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "GetNextVariableName",
                          (VOID *)gRT->GetNextVariableName,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "SetVariable",
                          (VOID *)gRT->SetVariable,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    
    //
    // Miscellaneous Services
    //
    Status = RowPointer ( SocketFD,
                          pPort,
                          "GetNextHighNonotonicCount",
                          (VOID *)gRT->GetNextHighMonotonicCount,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "ResetSystem",
                          (VOID *)gRT->ResetSystem,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Determine if the structures supports 2.0 services
    //
    if ( 2 <= ( gRT->Hdr.Revision >> 16 )) {
      //
      // UEFI 2.0 Capsule Services
      //
      Status = RowPointer ( SocketFD,
                            pPort,
                            "UpdateCapsule",
                            (VOID *)gRT->UpdateCapsule,
                            NULL );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = RowPointer ( SocketFD,
                            pPort,
                            "QueryCapsuleCapabilities",
                            (VOID *)gRT->QueryCapsuleCapabilities,
                            NULL );
      if ( EFI_ERROR ( Status )) {
        break;
      }
    
      //
      // Miscellaneous UEFI 2.0 Service
      //
      Status = RowPointer ( SocketFD,
                            pPort,
                            "QueryVariableInfo",
                            (VOID *)gRT->QueryVariableInfo,
                            NULL );
      if ( EFI_ERROR ( Status )) {
        break;
      }
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
