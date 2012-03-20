/**
  @file
  Display the ACPI tables

  Copyright (c) 2011-2012, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <WebServer.h>
#include <Guid/Acpi.h>
#include <IndustryStandard/Acpi10.h>
#include <IndustryStandard/Acpi30.h>

#if defined(_MSC_VER)   //  Handle Microsoft VC++ compiler specifics.
#pragma warning ( disable : 4305 )
#endif  //  defined(_MSC_VER)

//
// Ensure proper structure formats
//
#pragma pack(1)

typedef struct {
  UINT8 AddressSpaceId;
  UINT8 RegisterBitWidth;
  UINT8 RegisterBitOffset;
  UINT8 AccessSize;
  UINT64 Address;
} GENERIC_ADDRESS;


typedef struct {
  UINT32 Signature;           //    0
  UINT32 Length;              //    4
  UINT8 Revision;             //    8
  UINT8 Checksum;             //    9
  UINT8 OemId[6];             //   10
  UINT8 OemTableId[8];        //   16
  UINT32 OemRevision;         //   24
  UINT32 CreatorId;           //   28
  UINT32 CreatorRevision;     //   32
  UINT8 DefinitionBlock[1];   //   36
} ACPI_DSDT;


typedef struct {
  UINT32 Signature;           //    0
  UINT32 Length;              //    4
  UINT8 Revision;             //    8
  UINT8 Checksum;             //    9
  UINT8 OemId[6];             //   10
  UINT8 OemTableId[8];        //   16
  UINT32 OemRevision;         //   24
  UINT32 CreatorId;           //   28
  UINT32 CreatorRevision;     //   32
  UINT32 FirmwareCtrl;        //   36
  UINT32 DSDT;                //   40
  UINT8 Reserved;             //   44
  UINT8 PreferredPmProfile;   //   45
  UINT16 SciInt;              //   46
  UINT32 SmiCmd;              //   48
  UINT8 AcpiEnable;           //   52
  UINT8 AcpiDisable;          //   53
  UINT8 S4BiosReq;            //   54
  UINT8 PStateCnt;            //   55
  UINT32 Pm1aEvtBlk;          //   56
  UINT32 Pm1bEvtBlk;          //   60
  UINT32 Pm1aCntBlk;          //   64
  UINT32 Pm1bCntBlk;          //   68
  UINT32 Pm2CntBlk;           //   72
  UINT32 PmTmrBlk;            //   76
  UINT32 Gpe0Blk;             //   80
  UINT32 Gpe1Blk;             //   84
  UINT8 Pm1EvtLen;            //   88
  UINT8 Pm1CntLen;            //   89
  UINT8 PM2CntLen;            //   90
  UINT8 PmTmrLen;             //   91
  UINT8 Gpe0BlkLen;           //   92
  UINT8 Gpe1BlkLen;           //   93
  UINT8 Gpe1Base;             //   94
  UINT8 CstCnt;               //   95
  UINT16 PLvl2Lat;            //   96
  UINT16 PLvl3Lat;            //   98
  UINT16 FlushSize;           //  100
  UINT16 FlushStride;         //  102
  UINT8 DutyOffset;           //  104
  UINT8 DutyWidth;            //  105
  UINT8 DayAlrm;              //  106
  UINT8 MonAlrm;              //  107
  UINT8 Century;              //  108
  UINT16 IapcBootArch;        //  109
  UINT8 Reserved2;            //  111
  UINT32 Flags;               //  112
  UINT32 ResetReg[3];         //  116
  UINT8 ResetValue;           //  128
  UINT8 Reserved3[3];         //  129
  UINT64 XFirmwareCtrl;       //  132
  UINT64 XDsdt;               //  140
  UINT32 XPm1aEvtBlk[3];      //  148
  UINT32 XPm1bEvtBlk[3];      //  160
  UINT32 XPm1aCntBlk[3];      //  172
  UINT32 XPm1bCntBlk[3];      //  184
  UINT32 XPm2CntBlk[3];       //  196
  UINT32 XPmTmrBlk[3];        //  208
  UINT32 XGpe0Blk[3];         //  220
  UINT32 XGpe1Blk[3];         //  232
} ACPI_FADT;


typedef struct {
  UINT32 Signature;
  UINT32 Length;
  UINT8 Revision;
  UINT8 Checksum;
  UINT8 OemId[6];
  UINT8 OemTableId[8];
  UINT32 OemRevision;
  UINT32 CreatorId;
  UINT32 CreatorRevision;
  UINT32 Entry[1];
} ACPI_RSDT;


typedef struct {
  UINT32 Signature;           //    0
  UINT32 Length;              //    4
} ACPI_UNKNOWN;

#pragma pack()


typedef struct {
  UINT32 Signature;
  CONST CHAR8 * pTableName;
  CONST CHAR16 * pWebPage;
} TABLE_SIGNATURE;


CONST TABLE_SIGNATURE mTableId[] = {
  { APIC_SIGNATURE, "APIC", PAGE_ACPI_APIC },
  { BGRT_SIGNATURE, "BGRT", PAGE_ACPI_BGRT },
  { DSDT_SIGNATURE, "DSDT", PAGE_ACPI_DSDT },
  { FADT_SIGNATURE, "FADT", PAGE_ACPI_FADT },
  { HPET_SIGNATURE, "HPET", PAGE_ACPI_HPET },
  { MCFG_SIGNATURE, "MCFG", PAGE_ACPI_MCFG },
  { SSDT_SIGNATURE, "SSDT", PAGE_ACPI_SSDT },
  { TCPA_SIGNATURE, "TCPA", PAGE_ACPI_TCPA },
  { UEFI_SIGNATURE, "UEFI", PAGE_ACPI_UEFI }
};


/**
  Locate the RSDT table

  @return  Table address or NULL if not found

**/
CONST ACPI_RSDT *
LocateRsdt (
  VOID
  )
{
  CONST EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER * pRsdp10b;
  CONST EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER * pRsdp30;
  CONST ACPI_RSDT * pRsdt;
  EFI_STATUS Status;

  //
  //  Use for/break instead of goto
  //
  pRsdt = NULL;
  for ( ; ; ) {
    //
    //  Locate the RSDT
    //
    Status = EfiGetSystemConfigurationTable ( &gEfiAcpiTableGuid, (VOID **)&pRsdp30 );
    if ( !EFI_ERROR ( Status )) {
      pRsdt = (ACPI_RSDT *)(UINTN)pRsdp30->RsdtAddress;
    }
    else {
      Status = EfiGetSystemConfigurationTable (&gEfiAcpi10TableGuid, (VOID **)&pRsdp10b );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      pRsdt = (ACPI_RSDT *)(UINTN)pRsdp10b->RsdtAddress;
    }
    break;
  }

  //
  //  The entry was not found
  //
  return pRsdt;
}


/**
  Locate the specified table

  @param [in] Signature     Table signature

  @return  Table address or NULL if not found

**/
CONST VOID *
LocateTable (
  IN UINT32 Signature
  )
{
  CONST UINT32 * pEnd;
  CONST UINT32 * pEntry;
  CONST EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER * pRsdp10b;
  CONST EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER * pRsdp30;
  CONST ACPI_RSDT * pRsdt;
  CONST UINT32 * pSignature;
  EFI_STATUS Status;

  //
  //  Use for/break instead of goto
  //
  for ( ; ; ) {
    //
    //  Locate the RSDT
    //
    Status = EfiGetSystemConfigurationTable ( &gEfiAcpiTableGuid, (VOID **)&pRsdp30 );
    if ( !EFI_ERROR ( Status )) {
      pRsdt = (ACPI_RSDT *)(UINTN)pRsdp30->RsdtAddress;
    }
    else {
      Status = EfiGetSystemConfigurationTable (&gEfiAcpi10TableGuid, (VOID **)&pRsdp10b );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      pRsdt = (ACPI_RSDT *)(UINTN)pRsdp10b->RsdtAddress;
    }

    //
    //  Walk the list of entries
    //
    pEntry = &pRsdt->Entry[ 0 ];
    pEnd = &pEntry[(( pRsdt->Length - sizeof ( *pRsdt )) >> 2 ) + 1 ];
    while ( pEnd > pEntry ) {
      //
      //  The entry is actually a 32-bit physical table address
      //  The first entry in the table is the 32-bit table signature
      //
      pSignature = (UINT32 *)(UINTN)*pEntry;
      if ( *pSignature == Signature ) {
        return (CONST VOID *)(UINTN)*pEntry;
      }

      //
      //  Set the next entry
      //
      pEntry++;
    }
    break;
  }

  //
  //  The entry was not found
  //
  return NULL;
}


/**
  Display a row containing a hex value

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pName         Address of a zero terminated name string
  @param [in] Length        Length in bytes
  @param [in] pChar         Address of the first character

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
RowAnsiArray (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST CHAR8 * pName,
  IN UINTN Length,
  IN CONST CHAR8 * pChar
  )
{
  CONST CHAR8 * pData;
  CONST CHAR8 * pEnd;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Use for/break instead of goto
  //
  for ( ; ; ) {
    //
    //  Start the row
    //
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
    //  Display the characters
    //
    pData = pChar;
    pEnd = &pChar[ Length ];
    while ( pEnd > pData ) {
      Status = HttpSendCharacter ( SocketFD,
                                   pPort,
                                   *pData++,
                                   " " );
      if ( EFI_ERROR ( Status )) {
        break;
      }
    }
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the byte values
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<br/>0x" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    pData = pChar;
    while ( pEnd > pData ) {
      Status = HttpSendHexBits ( SocketFD,
                                 pPort,
                                 8,
                                 *pData++ );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      if ( pEnd > pData ) {
        Status = HttpSendAnsiString ( SocketFD,
                                      pPort,
                                      " 0x" );
        if ( EFI_ERROR ( Status )) {
          break;
        }
      }
    }

    //
    //  Terminate the row
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
  Format a row with a list of bytes

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pName         Zero terminated name string
  @param [in] ByteCount     The number of bytes to display
  @param [in] pData         Address of the byte array

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
RowBytes (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CHAR8 * pName,
  IN UINTN ByteCount,
  IN CONST UINT8 * pData
  )
{
  CONST UINT8 * pEnd;
  EFI_STATUS Status;

  //
  //  Use for/break instead of goto
  //
  for ( ; ; ) {
    //
    //  Start the row
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<tr><td>" );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the field name
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  pName );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the field value
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</td><td><code>0x" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    pEnd = &pData[ ByteCount ];
    while ( pEnd > pData ) {
      Status = HttpSendHexBits ( SocketFD,
                                 pPort,
                                 8,
                                 *pData++ );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      if ( pEnd > pData ) {
        Status = HttpSendAnsiString ( SocketFD,
                                      pPort,
                                      " 0x" );
        if ( EFI_ERROR ( Status )) {
          break;
        }
      }
    }
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Terminate the row
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</code></td></tr>\r\n" );
    break;
  }

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Format a row with a list of bytes

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pName         Zero terminated name string
  @param [in] ByteCount     The number of bytes to display
  @param [in] pData         Address of the byte array

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
RowDump (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CHAR8 * pName,
  IN UINTN ByteCount,
  IN CONST UINT8 * pData
  )
{
  EFI_STATUS Status;

  //
  //  Use for/break instead of goto
  //
  for ( ; ; ) {
    //
    //  Start the row
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<tr><td>" );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the field name
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  pName );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Start the field value
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</td><td>" );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Dump the buffer
    //
    Status = HttpSendDump ( SocketFD,
                            pPort,
                            ByteCount,
                            pData );

    //
    //  Terminate the field value and row
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</td></tr>\r\n" );
    break;
  }

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Format a row with a general address

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pName         Zero terminated name string
  @param [in] pAddr         Address of the general address buffer
  @param [in] pWebPage      Zero terminated web page address

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
RowGenericAddress (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CHAR8 * pName,
  IN CONST UINT32 * pAddr,
  IN CONST CHAR16 * pWebPage
  )
{
  CONST GENERIC_ADDRESS * pGenericAddress;
  EFI_STATUS Status;

  //
  //  Use for/break instead of goto
  //
  for ( ; ; ) {
    //
    //  Start the row
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<tr><td>" );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the field name
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  pName );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the field value
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</td><td><code>" );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Determine the type of address
    //
    pGenericAddress = (CONST GENERIC_ADDRESS *)pAddr;
    if ( 0 == pGenericAddress->AddressSpaceId ) {
      Status = HttpSendAnsiString ( SocketFD, pPort, "System Memory" );
    }
    else if ( 1 == pGenericAddress->AddressSpaceId ) {
      Status = HttpSendAnsiString ( SocketFD, pPort, "I/O Space" );
    }
    else if ( 2 == pGenericAddress->AddressSpaceId ) {
      Status = HttpSendAnsiString ( SocketFD, pPort, "PCI Configuration Space" );
    }
    else if ( 3 == pGenericAddress->AddressSpaceId ) {
      Status = HttpSendAnsiString ( SocketFD, pPort, "Embedded Controller" );
    }
    else if ( 4 == pGenericAddress->AddressSpaceId ) {
      Status = HttpSendAnsiString ( SocketFD, pPort, "SMBus" );
    }
    else if ( 0x7f == pGenericAddress->AddressSpaceId ) {
      Status = HttpSendAnsiString ( SocketFD, pPort, "Functional Fixed Hardware" );
    }
    else if (( 0xc0 <= pGenericAddress->AddressSpaceId )
      && ( 0xff >= pGenericAddress->AddressSpaceId )) {
      Status = HttpSendAnsiString ( SocketFD, pPort, "OEM Defined" );
    }
    else {
      Status = HttpSendAnsiString ( SocketFD, pPort, "Reserved" );
    }
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<br/>Register Bit Width: " );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendValue ( SocketFD,
                             pPort,
                             pGenericAddress->RegisterBitWidth );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<br/>Register Bit Offset: " );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendHexValue ( SocketFD,
                                pPort,
                                pGenericAddress->RegisterBitOffset );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<br/>Access Size: " );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendValue ( SocketFD,
                             pPort,
                             pGenericAddress->AccessSize );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<br/>Address: " );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Add the web-page link if necessary
    //
    if ( NULL != pWebPage ) {
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "<a target=\"_blank\" href=\"" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendUnicodeString ( SocketFD,
                                       pPort,
                                       pWebPage );
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
    //  Display the address
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "0x" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendHexBits ( SocketFD,
                               pPort,
                               64,
                               pGenericAddress->Address );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Finish the web-page link if necessary
    //
    if ( NULL != pWebPage ) {
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "</a>" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
    }

    //
    //  Terminate the row
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</code></td></tr>\r\n" );
    break;
  }

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Translate a table address into a web page

  @param [in] pSignature      Address of the table signature
  @param [out] ppTableName    Address to receive the table name address

  @return  Zero terminated web page address or NULL if not found

**/
CONST CHAR16 *
SignatureLookup (
  IN UINT32 * pSignature,
  OUT CONST CHAR8 ** ppTableName
  )
{
  CONST TABLE_SIGNATURE * pTableId;
  CONST TABLE_SIGNATURE * pEnd;
  UINT32 Signature;

  //
  //  Walk the list of tables
  //
  Signature = *pSignature;
  pTableId = &mTableId[ 0 ];
  pEnd = &pTableId[ sizeof ( mTableId ) / sizeof ( mTableId[ 0 ])];
  while ( pEnd > pTableId ) {
    //
    //  Attempt to locate the table signature
    //
    if ( pTableId->Signature == Signature ) {
      //
      //  The signature was found
      //  Return the web page
      //
      *ppTableName = pTableId->pTableName;
      return pTableId->pWebPage;
    }

    //
    //  Set the next table
    //
    pTableId += 1;
  }

  //
  //  The table was not found
  //
  *ppTableName = (CONST CHAR8 *)pSignature;
  return NULL;
}


/**
  Respond with the APIC table

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
AcpiApicPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  CONST ACPI_UNKNOWN * pApic;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Send the APIC page
  //
  for ( ; ; ) {
    //
    //  Locate the APIC
    //
    pApic = (ACPI_UNKNOWN *)LocateTable ( APIC_SIGNATURE );
    if ( NULL == pApic ) {
      Status = EFI_NOT_FOUND;
      break;
    }

    //
    //  Send the page and table header
    //
    Status = TableHeader ( SocketFD, pPort, L"APIC Table", pApic );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the header
    //
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "Signature",
                            sizeof ( pApic->Signature ),
                            (CHAR8 *)&pApic->Signature );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "Length",
                               pApic->Length );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the data from the table
    //
    Status = RowDump ( SocketFD,
                       pPort,
                       "Data",
                       pApic->Length - sizeof ( *pApic ) + 1,
                       (UINT8 *)( pApic + 1 ));
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


/**
  Respond with the BGRT table

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
AcpiBgrtPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  CONST ACPI_UNKNOWN * pBgrt;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Send the BGRT page
  //
  for ( ; ; ) {
    //
    //  Locate the BGRT
    //
    pBgrt = (ACPI_UNKNOWN *)LocateTable ( BGRT_SIGNATURE );
    if ( NULL == pBgrt ) {
      Status = EFI_NOT_FOUND;
      break;
    }

    //
    //  Send the page and table header
    //
    Status = TableHeader ( SocketFD, pPort, L"BGRT Table", pBgrt );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the header
    //
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "Signature",
                            sizeof ( pBgrt->Signature ),
                            (CHAR8 *)&pBgrt->Signature );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "Length",
                               pBgrt->Length );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the data from the table
    //
    Status = RowDump ( SocketFD,
                       pPort,
                       "Data",
                       pBgrt->Length - sizeof ( *pBgrt ) + 1,
                       (UINT8 *)( pBgrt + 1 ));
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


/**
  Respond with the ACPI DSDT table

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
AcpiDsdtPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  CONST ACPI_DSDT * pDsdt;
  CONST ACPI_FADT * pFadt;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Send the DADT page
  //
  for ( ; ; ) {
    //
    //  Locate the DADT
    //
    pFadt = (ACPI_FADT *)LocateTable ( FADT_SIGNATURE );
    if ( NULL == pFadt ) {
      Status = EFI_NOT_FOUND;
      break;
    }
    pDsdt = (VOID *)(UINTN)pFadt->XDsdt;

    //
    //  Send the page and table header
    //
    Status = TableHeader ( SocketFD, pPort, L"DSDT - Differentiated System Description Table", pDsdt );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the DSDT header
    //
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "Signature",
                            sizeof ( pDsdt->Signature ),
                            (CHAR8 *)&pDsdt->Signature );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "Length",
                               pDsdt->Length );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "Revision",
                               pDsdt->Revision );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "Checksum",
                           pDsdt->Checksum,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "OEMID",
                            sizeof ( pDsdt->OemId ),
                            (CONST CHAR8 *)&pDsdt->OemId[ 0 ]);
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "OEM Table ID",
                            sizeof ( pDsdt->OemTableId ),
                            (CONST CHAR8 *)&pDsdt->OemTableId[ 0 ]);
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowRevision ( SocketFD,
                           pPort,
                           "OEM Revision",
                           pDsdt->OemRevision );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "Creator ID",
                            sizeof ( pDsdt->CreatorId ),
                            (CHAR8 *)&pDsdt->CreatorId );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowRevision ( SocketFD,
                           pPort,
                           "Creator Revision",
                           pDsdt->CreatorRevision );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the data from the DSDT
    //
    Status = RowDump ( SocketFD,
                       pPort,
                       "Definition Block",
                       pDsdt->Length - sizeof ( *pDsdt ) + 1,
                       &pDsdt->DefinitionBlock[0]);
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


/**
  Respond with the ACPI FADT table

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
AcpiFadtPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  CONST ACPI_FADT * pFadt;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Send the FADT page
  //
  for ( ; ; ) {
    //
    //  Locate the FADT
    //
    pFadt = (ACPI_FADT *)LocateTable ( FADT_SIGNATURE );
    if ( NULL == pFadt ) {
      Status = EFI_NOT_FOUND;
      break;
    }

    //
    //  Send the page and table header
    //
    Status = TableHeader ( SocketFD, pPort, L"FADT - Fixed ACPI Description Table", pFadt );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the FSDT header
    //
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "Signature",
                            sizeof ( pFadt->Signature ),
                            (CHAR8 *)&pFadt->Signature );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "Length",
                               pFadt->Length );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "Revision",
                               pFadt->Revision );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "Checksum",
                           pFadt->Checksum,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "OEMID",
                            sizeof ( pFadt->OemId ),
                            (CONST CHAR8 *)&pFadt->OemId[ 0 ]);
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "OEM Table ID",
                            sizeof ( pFadt->OemTableId ),
                            (CONST CHAR8 *)&pFadt->OemTableId[ 0 ]);
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowRevision ( SocketFD,
                           pPort,
                           "OEM Revision",
                           pFadt->OemRevision );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "Creator ID",
                            sizeof ( pFadt->CreatorId ),
                            (CHAR8 *)&pFadt->CreatorId );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowRevision ( SocketFD,
                           pPort,
                           "Creator Revision",
                           pFadt->CreatorRevision );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the data from the FADT
    //
    Status = RowPointer ( SocketFD,
                          pPort,
                          "FIRMWARE_CTRL",
                          (CONST VOID *)(UINTN)pFadt->FirmwareCtrl,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "DSDT",
                          (CONST VOID *)(UINTN)pFadt->DSDT,
                          ( pFadt->DSDT == pFadt->XDsdt ) ? PAGE_ACPI_DSDT : NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "Reserved",
                           pFadt->Reserved,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "Preferred_PM_Profile",
                           pFadt->PreferredPmProfile,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "SCI_INT",
                           pFadt->SciInt,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "SMI_CMD",
                           pFadt->SmiCmd,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "ACPI_ENABLE",
                           pFadt->AcpiEnable,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "ACPI_DISABLE",
                           pFadt->AcpiDisable,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "S4BIOS_REQ",
                           pFadt->S4BiosReq,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "PSTATE_CNT",
                           pFadt->PStateCnt,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "PM1a_EVT_BLK",
                           pFadt->Pm1aEvtBlk,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "PM1b_EVT_BLK",
                           pFadt->Pm1bEvtBlk,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "PM1a_CNT_BLK",
                           pFadt->Pm1aCntBlk,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "PM1b_CNT_BLK",
                           pFadt->Pm1bCntBlk,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "PM2_CNT_BLK",
                           pFadt->Pm2CntBlk,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "PM_TMR_BLK",
                           pFadt->PmTmrBlk,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    Status = RowHexValue ( SocketFD,
                           pPort,
                           "GPE0_BLK",
                           pFadt->Gpe0Blk,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "GPE1_BLK",
                           pFadt->Gpe1Blk,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "PM1_EVT_LEN",
                               pFadt->Pm1EvtLen );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "PM1_CNT_LEN",
                               pFadt->Pm1CntLen );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "PM2_CNT_LEN",
                               pFadt->PM2CntLen );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "PM_TMR_LEN",
                               pFadt->PmTmrLen );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "GPE0_BLK_LEN",
                               pFadt->Gpe0BlkLen );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "GPE1_BLK_LEN",
                               pFadt->Gpe1BlkLen );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "GPE1_BASE",
                           pFadt->Gpe1Base,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "CST_CNT",
                               pFadt->CstCnt );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "P_LVL2_LAT",
                           pFadt->PLvl2Lat,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "P_LVL3_LAT",
                           pFadt->PLvl3Lat,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "FLUSH_SIZE",
                               pFadt->FlushSize );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "FLUSH_Stride",
                               pFadt->FlushStride );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "DUTY_OFFSET",
                           pFadt->DutyOffset,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "DUTY_WIDTH",
                           pFadt->DutyWidth,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "DAY_ALRM",
                           pFadt->DayAlrm,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "MON_ALRM",
                           pFadt->MonAlrm,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "CENTURY",
                           pFadt->Century,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "IAPC_BOOT_ARCH",
                           pFadt->IapcBootArch,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "Reserved",
                           pFadt->Reserved2,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "Flags",
                           pFadt->Flags,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowGenericAddress ( SocketFD,
                                 pPort,
                                 "RESET_REG",
                                 &pFadt->ResetReg[0],
                                 NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "RESET_VALUE",
                           pFadt->ResetValue,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "Reserved",
                           pFadt->Reserved3[0],
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "Reserved",
                           pFadt->Reserved3[1],
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "Reserved",
                           pFadt->Reserved3[2],
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "X_FIRMWARE_CTRL",
                           pFadt->XFirmwareCtrl,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "X_DSDT",
                           pFadt->XDsdt,
                           PAGE_ACPI_DSDT );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowGenericAddress ( SocketFD,
                                 pPort,
                                 "X_PM1a_EVT_BLK",
                                 &pFadt->XPm1aEvtBlk[0],
                                 NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowGenericAddress ( SocketFD,
                                 pPort,
                                 "X_PM1b_EVT_BLK",
                                 &pFadt->XPm1bEvtBlk[0],
                                 NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowGenericAddress ( SocketFD,
                                 pPort,
                                 "X_PM1a_CNT_BLK",
                                 &pFadt->XPm1aCntBlk[0],
                                 NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowGenericAddress ( SocketFD,
                                 pPort,
                                 "X_PM1b_CNT_BLK",
                                 &pFadt->XPm1bCntBlk[0],
                                 NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowGenericAddress ( SocketFD,
                                 pPort,
                                 "X_PM2_CNT_BLK",
                                 &pFadt->XPm2CntBlk[0],
                                 NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowGenericAddress ( SocketFD,
                                 pPort,
                                 "X_PM_TMR_BLK",
                                 &pFadt->XPmTmrBlk[0],
                                 NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowGenericAddress ( SocketFD,
                                 pPort,
                                 "X_GPE0_BLK",
                                 &pFadt->XGpe0Blk[0],
                                 NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowGenericAddress ( SocketFD,
                                 pPort,
                                 "X_GPE1_BLK",
                                 &pFadt->XGpe1Blk[0],
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


/**
  Respond with the HPET table

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
AcpiHpetPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  CONST ACPI_UNKNOWN * pHpet;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Send the HPET page
  //
  for ( ; ; ) {
    //
    //  Locate the HPET
    //
    pHpet = (ACPI_UNKNOWN *)LocateTable ( HPET_SIGNATURE );
    if ( NULL == pHpet ) {
      Status = EFI_NOT_FOUND;
      break;
    }

    //
    //  Send the page and table header
    //
    Status = TableHeader ( SocketFD, pPort, L"HPET Table", pHpet );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the header
    //
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "Signature",
                            sizeof ( pHpet->Signature ),
                            (CHAR8 *)&pHpet->Signature );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "Length",
                               pHpet->Length );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the data from the table
    //
    Status = RowDump ( SocketFD,
                       pPort,
                       "Data",
                       pHpet->Length - sizeof ( *pHpet ) + 1,
                       (UINT8 *)( pHpet + 1 ));
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


/**
  Respond with the MCFG table

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
AcpiMcfgPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  CONST ACPI_UNKNOWN * pMcfg;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Send the MCFG page
  //
  for ( ; ; ) {
    //
    //  Locate the MCFG
    //
    pMcfg = (ACPI_UNKNOWN *)LocateTable ( MCFG_SIGNATURE );
    if ( NULL == pMcfg ) {
      Status = EFI_NOT_FOUND;
      break;
    }

    //
    //  Send the page and table header
    //
    Status = TableHeader ( SocketFD, pPort, L"MCFG Table", pMcfg );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the header
    //
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "Signature",
                            sizeof ( pMcfg->Signature ),
                            (CHAR8 *)&pMcfg->Signature );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "Length",
                               pMcfg->Length );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the data from the table
    //
    Status = RowDump ( SocketFD,
                       pPort,
                       "Data",
                       pMcfg->Length - sizeof ( *pMcfg ) + 1,
                       (UINT8 *)( pMcfg + 1 ));
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


/**
  Respond with the ACPI RSDP 1.0b table

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
AcpiRsdp10Page (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  CONST EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER * pRsdp10b;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Send the RSDP page
  //
  for ( ; ; ) {
    //
    //  Locate the RSDP
    //
    Status = EfiGetSystemConfigurationTable ( &gEfiAcpi10TableGuid, (VOID **) &pRsdp10b );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Send the page and table header
    //
    Status = TableHeader ( SocketFD, pPort, L"RSDP - ACPI 1.0b Root System Description Pointer", pRsdp10b );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the RSDP
    //
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "Signature",
                            sizeof ( pRsdp10b->Signature ),
                            (CHAR8 *)&pRsdp10b->Signature );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "Checksum",
                           pRsdp10b->Checksum,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "OemId",
                            sizeof ( pRsdp10b->OemId ),
                            (CONST CHAR8 *)&pRsdp10b->OemId[ 0 ]);
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "Reserved",
                           pRsdp10b->Reserved,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "RsdtAddress",
                          (VOID *)(UINTN)pRsdp10b->RsdtAddress,
                          PAGE_ACPI_RSDT );
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


/**
  Respond with the ACPI RSDP 3.0 table

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
AcpiRsdp30Page (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  CONST EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER * pRsdp30;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Send the RSDP page
  //
  for ( ; ; ) {
    //
    //  Locate the RSDP
    //
    Status = EfiGetSystemConfigurationTable ( &gEfiAcpiTableGuid, (VOID **) &pRsdp30 );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Send the page and table header
    //
    Status = TableHeader ( SocketFD, pPort, L"RSDP - ACPI 3.0 Root System Description Pointer", pRsdp30 );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the RSDP
    //
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "Signature",
                            sizeof ( pRsdp30->Signature ),
                            (CHAR8 *)&pRsdp30->Signature );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "Checksum",
                           pRsdp30->Checksum,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "OemId",
                            sizeof ( pRsdp30->OemId ),
                            (CONST CHAR8 *)&pRsdp30->OemId[ 0 ]);
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "Revision",
                           pRsdp30->Revision,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "RsdtAddress",
                          (VOID *)(UINTN)pRsdp30->RsdtAddress,
                          PAGE_ACPI_RSDT );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "Length",
                               pRsdp30->Length );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowPointer ( SocketFD,
                          pPort,
                          "XsdtAddress",
                          (VOID *)(UINTN)pRsdp30->XsdtAddress,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "ExtendedChecksum",
                           pRsdp30->ExtendedChecksum,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowBytes ( SocketFD,
                        pPort,
                        "Reserved",
                        sizeof ( pRsdp30->Reserved ),
                        &pRsdp30->Reserved[ 0 ]);
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


/**
  Respond with the ACPI RSDT table

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
AcpiRsdtPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  CONST UINT32 * pEnd;
  CONST UINT32 * pEntry;
  CONST ACPI_RSDT * pRsdt;
  CONST CHAR8 * pTableName;
  CONST CHAR16 * pWebPage;
  EFI_STATUS Status;
  UINT32 TableName[ 2 ];

  DBG_ENTER ( );

  //
  //  Send the RSDT page
  //
  for ( ; ; ) {
    //
    //  Locate the RSDT
    //
    pRsdt = LocateRsdt ( );
    if ( NULL == pRsdt ) {
      Status = EFI_NOT_FOUND;
      break;
    }

    //
    //  Send the page and table header
    //
    Status = TableHeader ( SocketFD, pPort, L"RSDT - ACPI Root System Description Table", pRsdt );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the RSDT
    //
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "Signature",
                            sizeof ( pRsdt->Signature ),
                            (CHAR8 *)&pRsdt->Signature );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "Length",
                               pRsdt->Length );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "Revision",
                               pRsdt->Revision );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "Checksum",
                           pRsdt->Checksum,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "OEMID",
                            sizeof ( pRsdt->OemId ),
                            (CONST CHAR8 *)&pRsdt->OemId[ 0 ]);
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "OEM Table ID",
                            sizeof ( pRsdt->OemTableId ),
                            (CONST CHAR8 *)&pRsdt->OemTableId[ 0 ]);
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowRevision ( SocketFD,
                           pPort,
                           "OEM Revision",
                           pRsdt->OemRevision );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "Creator ID",
                            sizeof ( pRsdt->CreatorId ),
                            (CHAR8 *)&pRsdt->CreatorId );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowRevision ( SocketFD,
                           pPort,
                           "Creator Revision",
                           pRsdt->CreatorRevision );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Walk the list of entries
    //
    pEntry = &pRsdt->Entry[ 0 ];
    pEnd = &pEntry[(( pRsdt->Length - sizeof ( *pRsdt )) >> 2 ) + 1 ];
    TableName[ 1 ] = 0;
    while ( pEnd > pEntry ) {
      //
      //  The entry is actually a 32-bit physical table address
      //  The first entry in the table is the 32-bit table signature
      //
      TableName[ 0 ] = *(UINT32 *)(UINTN)*pEntry;
      pWebPage = SignatureLookup ( &TableName[ 0 ], &pTableName );

      //
      //  Display the table address
      //
      Status = RowPointer ( SocketFD,
                            pPort,
                            pTableName,
                            (VOID *)(UINTN)*pEntry,
                            pWebPage );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      pEntry++;
    }
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


/**
  Respond with the SSDT table

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
AcpiSsdtPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  CONST ACPI_UNKNOWN * pSsdt;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Send the SSDT page
  //
  for ( ; ; ) {
    //
    //  Locate the SSDT
    //
    pSsdt = (ACPI_UNKNOWN *)LocateTable ( SSDT_SIGNATURE );
    if ( NULL == pSsdt ) {
      Status = EFI_NOT_FOUND;
      break;
    }

    //
    //  Send the page and table header
    //
    Status = TableHeader ( SocketFD, pPort, L"SSDT Table", pSsdt );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the header
    //
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "Signature",
                            sizeof ( pSsdt->Signature ),
                            (CHAR8 *)&pSsdt->Signature );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "Length",
                               pSsdt->Length );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the data from the table
    //
    Status = RowDump ( SocketFD,
                       pPort,
                       "Data",
                       pSsdt->Length - sizeof ( *pSsdt ) + 1,
                       (UINT8 *)( pSsdt + 1 ));
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


/**
  Respond with the TCPA table

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
AcpiTcpaPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  CONST ACPI_UNKNOWN * pTcpa;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Send the TCPA page
  //
  for ( ; ; ) {
    //
    //  Locate the TCPA
    //
    pTcpa = (ACPI_UNKNOWN *)LocateTable ( TCPA_SIGNATURE );
    if ( NULL == pTcpa ) {
      Status = EFI_NOT_FOUND;
      break;
    }

    //
    //  Send the page and table header
    //
    Status = TableHeader ( SocketFD, pPort, L"TCPA Table", pTcpa );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the header
    //
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "Signature",
                            sizeof ( pTcpa->Signature ),
                            (CHAR8 *)&pTcpa->Signature );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "Length",
                               pTcpa->Length );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the data from the table
    //
    Status = RowDump ( SocketFD,
                       pPort,
                       "Data",
                       pTcpa->Length - sizeof ( *pTcpa ) + 1,
                       (UINT8 *)( pTcpa + 1 ));
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


/**
  Respond with the UEFI table

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
AcpiUefiPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  CONST ACPI_UNKNOWN * pUefi;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Send the UEFI page
  //
  for ( ; ; ) {
    //
    //  Locate the UEFI
    //
    pUefi = (ACPI_UNKNOWN *)LocateTable ( UEFI_SIGNATURE );
    if ( NULL == pUefi ) {
      Status = EFI_NOT_FOUND;
      break;
    }

    //
    //  Send the page and table header
    //
    Status = TableHeader ( SocketFD, pPort, L"UEFI Table", pUefi );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the header
    //
    Status = RowAnsiArray ( SocketFD,
                            pPort,
                            "Signature",
                            sizeof ( pUefi->Signature ),
                            (CHAR8 *)&pUefi->Signature );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "Length",
                               pUefi->Length );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the data from the table
    //
    Status = RowDump ( SocketFD,
                       pPort,
                       "Data",
                       pUefi->Length - sizeof ( *pUefi ) + 1,
                       (UINT8 *)( pUefi + 1 ));
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
