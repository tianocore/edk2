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
  Display the configuration table

**/

#include <WebServer.h>
#include <Guid\Acpi.h>
#include <Guid\DebugImageInfoTable.h>
#include <Guid\DxeServices.h>
#include <Guid\HobList.h>
#include <Guid\MemoryTypeInformation.h>
#include <Guid\LoadModuleAtFixedAddress.h>


typedef struct {
  CHAR16 * GuidName;
  EFI_GUID * pGuid;
  CHAR16 * pWebPage;
} GUID_NAME;

CONST GUID_NAME mGuidName[] = {
  { L"gEfiAcpi10TableGuid", &gEfiAcpi10TableGuid, PAGE_ACPI_RSDP_10B },
  { L"gEfiAcpiTableGuid", &gEfiAcpiTableGuid, PAGE_ACPI_RSDP_30 },
  { L"gEfiDebugImageInfoTableGuid", &gEfiDebugImageInfoTableGuid, NULL },
  { L"gEfiDxeServicesTableGuid", &gEfiDxeServicesTableGuid, PAGE_DXE_SERVICES_TABLE },
  { L"gEfiHobListGuid", &gEfiHobListGuid, NULL },
  { L"gEfiMemoryTypeInformationGuid", &gEfiMemoryTypeInformationGuid, NULL },
  { L"gLoadFixedAddressConfigurationTableGuid", &gLoadFixedAddressConfigurationTableGuid, NULL }
};

/**
  Display a row containing a GUID value

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pName         Address of a zero terminated name string
  @param [in] pGuid         Address of the GUID to display

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
RowGuid (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST CHAR8 * pName,
  IN CONST EFI_GUID * pGuid
  )
{
  CONST GUID_NAME * pGuidName;
  CONST GUID_NAME * pGuidNameEnd;
  EFI_STATUS Status;
  UINTN Value;

  DBG_ENTER ( );
  
  //
  //  Use for/break instead of goto
  //
  for ( ; ; ) {
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<tr><td>" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  pName );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</td><td><code>" );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Determine if this is a known GUID
    //
    pGuidName = &mGuidName[0];
    pGuidNameEnd = &pGuidName[ sizeof ( mGuidName ) / sizeof ( mGuidName[0])];
    while ( pGuidNameEnd > pGuidName ) {
      if ( CompareGuid ( pGuidName->pGuid, pGuid )) {
        //
        //  Display the web link if available
        //
        if ( NULL != pGuidName->pWebPage ) {
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "<a target=\"_blank\" href=\"" );
          if ( EFI_ERROR ( Status )) {
            break;
          }
          Status = HttpSendUnicodeString ( SocketFD,
                                           pPort,
                                           pGuidName->pWebPage );
          if ( EFI_ERROR ( Status )) {
            break;
          }
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "\">" );
          if ( EFI_ERROR ( Status )) {
            break;
          }
        }

        //
        //  Display the GUID name
        //
        Status = HttpSendUnicodeString ( SocketFD,
                                         pPort,
                                         pGuidName->GuidName );

        //
        //  Complete the web link if available
        //
        if ( NULL != pGuidName->pWebPage ) {
          if ( EFI_ERROR ( Status )) {
            break;
          }
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "</a>" );
        }
        break;
      }
    
      //
      //  Set the next GUID name
      //
      pGuidName += 1;
    }
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Only if the entry is not known, display the GUID and type
    //
    if ( pGuidNameEnd <= pGuidName ) {
      //
      //  Display the GUID
      //
      Status = HttpSendGuid ( SocketFD,
                              pPort,
                              pGuid );
      if ( EFI_ERROR ( Status )) {
        break;
      }

      //
      //  Display the GUID type
      //
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "<br/><a target=\"_blank\" href=\"http://www.ietf.org/rfc/rfc4122.txt\">Guid Type</a>: " );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Value = pGuid->Data4[1];
      Value >>= 5;
      if ( 3 >= Value ) {
        //
        //  Network type
        //
        Status = HttpSendAnsiString ( SocketFD,
                                      pPort,
                                      "Network " );
      }
      else if ( 5 >= Value ) {
        //
        //  Standard type
        //
        Status = HttpSendAnsiString ( SocketFD,
                                      pPort,
                                      "Standard " );
        if ( EFI_ERROR ( Status )) {
          break;
        }

        //
        //  Decode the standard type using RFC 4122
        //
        Value = pGuid->Data3;
        Value >>= 12;
        switch ( Value ) {
        default:
          //
          //  Display the MAC address
          //
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "Version " );
          if ( EFI_ERROR ( Status )) {
            break;
          }
          Status = HttpSendValue ( SocketFD,
                                   pPort,
                                   pGuid->Data3 >> 12 );
          break;

        case 1:
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "MAC address" );
          break;

        case 2:
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "DCE Security" );
          break;

        case 3:
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "MD5 hash" );
          break;

        case 4:
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "Random" );
          break;

        case 5:
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "SHA-1 hash" );
          break;
        }
      }
      else if ( 6 == Value ) {
        //
        //  Microsoft's Component Object Model (COM) type
        //
        Status = HttpSendAnsiString ( SocketFD,
                                      pPort,
                                      "Microsoft COM" );
      }
      else {
        //
        //  Reserved type
        //
        Status = HttpSendAnsiString ( SocketFD,
                                      pPort,
                                      "Reserved" );
      }
    }

    //
    //  Done with this entry
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</code></td></tr>\r\n" );
    break;
  }
  
  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Respond with the configuration tables

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
ConfigurationTablePage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  EFI_CONFIGURATION_TABLE * pEnd;
  EFI_CONFIGURATION_TABLE * pTable;
  EFI_STATUS Status;

  DBG_ENTER ( );
  
  //
  //  Send the system table page
  //
  for ( ; ; ) {
    //
    //  Send the page and table header
    //
    Status = TableHeader ( SocketFD, pPort, L"Configuration Tables", gST );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the table size
    //
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "Entries",
                               gST->NumberOfTableEntries );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Determine the location of the configuration tables
    //
    pTable = gST->ConfigurationTable;
    pEnd = &pTable[ gST->NumberOfTableEntries ];
    while ( pEnd > pTable ) {
      Status = RowGuid ( SocketFD,
                         pPort,
                         "VendorGuid",
                         &pTable->VendorGuid );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = RowPointer ( SocketFD,
                            pPort,
                            "VendorTable",
                            (VOID *)pTable->VendorTable,
                            NULL );
      if ( EFI_ERROR ( Status )) {
        break;
      }

      //
      //  Set the next row
      //
      pTable += 1;
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
