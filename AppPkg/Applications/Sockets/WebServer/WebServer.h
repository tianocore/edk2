/** @file
  Definitions for the web server.

  Copyright (c) 2011-2012, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _WEB_SERVER_H_
#define _WEB_SERVER_H_

#include <errno.h>
#include <Uefi.h>

#include <Guid/EventGroup.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/BlockIo.h>

#include <netinet/in.h>

#include <sys/EfiSysCall.h>
#include <sys/poll.h>
#include <sys/socket.h>

#if defined(_MSC_VER)   //  Handle Microsoft VC++ compiler specifics.
#pragma warning ( disable : 4054 )
#pragma warning ( disable : 4152 )
#endif  //  defined(_MSC_VER)

//------------------------------------------------------------------------------
//  Pages
//------------------------------------------------------------------------------

#define PAGE_ACPI_APIC                  L"/APIC"
#define PAGE_ACPI_BGRT                  L"/BGRT"
#define PAGE_ACPI_DSDT                  L"/DSDT"
#define PAGE_ACPI_FADT                  L"/FADT"
#define PAGE_ACPI_HPET                  L"/HPET"
#define PAGE_ACPI_MCFG                  L"/MCFG"
#define PAGE_ACPI_RSDP_10B              L"/RSDP1.0b"
#define PAGE_ACPI_RSDP_30               L"/RSDP3.0"
#define PAGE_ACPI_RSDT                  L"/RSDT"
#define PAGE_ACPI_SSDT                  L"/SSDT"
#define PAGE_ACPI_TCPA                  L"/TCPA"
#define PAGE_ACPI_UEFI                  L"/UEFI"
#define PAGE_BOOT_SERVICES_TABLE        L"/BootServicesTable"
#define PAGE_CONFIGURATION_TABLE        L"/ConfigurationTable"
#define PAGE_DXE_SERVICES_TABLE         L"/DxeServicesTable"
#define PAGE_RUNTIME_SERVICES_TABLE     L"/RuntimeServicesTable"

//------------------------------------------------------------------------------
//  Signatures
//------------------------------------------------------------------------------

#define APIC_SIGNATURE        0x43495041
#define BGRT_SIGNATURE        0x54524742
#define DSDT_SIGNATURE        0x54445344
#define FADT_SIGNATURE        0x50434146
#define HPET_SIGNATURE        0x54455048
#define MCFG_SIGNATURE        0x4746434d
#define SSDT_SIGNATURE        0x54445353
#define TCPA_SIGNATURE        0x41504354
#define UEFI_SIGNATURE        0x49464555

//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------

#if defined(_MSC_VER)           /* Handle Microsoft VC++ compiler specifics. */
#define DBG_ENTER()             DEBUG (( DEBUG_INFO, "Entering " __FUNCTION__ "\n" )) ///<  Display routine entry
#define DBG_EXIT()              DEBUG (( DEBUG_INFO, "Exiting " __FUNCTION__ "\n" ))  ///<  Display routine exit
#define DBG_EXIT_DEC(Status)    DEBUG (( DEBUG_INFO, "Exiting " __FUNCTION__ ", Status: %d\n", Status ))      ///<  Display routine exit with decimal value
#define DBG_EXIT_HEX(Status)    DEBUG (( DEBUG_INFO, "Exiting " __FUNCTION__ ", Status: 0x%08x\n", Status ))  ///<  Display routine exit with hex value
#define DBG_EXIT_STATUS(Status) DEBUG (( DEBUG_INFO, "Exiting " __FUNCTION__ ", Status: %r\n", Status ))      ///<  Display routine exit with status value
#define DBG_EXIT_TF(Status)     DEBUG (( DEBUG_INFO, "Exiting " __FUNCTION__ ", returning %s\n", (FALSE == Status) ? L"FALSE" : L"TRUE" ))  ///<  Display routine with TRUE/FALSE value
#else   //  _MSC_VER
#define DBG_ENTER()
#define DBG_EXIT()
#define DBG_EXIT_DEC(Status)
#define DBG_EXIT_HEX(Status)
#define DBG_EXIT_STATUS(Status)
#define DBG_EXIT_TF(Status)
#endif  //  _MSC_VER

#define DIM(x)    ( sizeof ( x ) / sizeof ( x[0] ))   ///<  Compute the number of entries in an array

//------------------------------------------------------------------------------
//  Constants
//------------------------------------------------------------------------------

#define DEBUG_SOCKET_POLL       0x00080000  ///<  Display the socket poll messages
#define DEBUG_PORT_WORK         0x00040000  ///<  Display the port work messages
#define DEBUG_SERVER_LISTEN     0x00020000  ///<  Display the socket poll messages
#define DEBUG_HTTP_PORT         0x00010000  ///<  Display HTTP port related messages
#define DEBUG_REQUEST           0x00008000  ///<  Display the HTTP request messages

#define HTTP_PORT_POLL_DELAY  ( 2 * 1000 )  ///<  Delay in milliseconds for attempts to open the HTTP port
#define CLIENT_POLL_DELAY     50            ///<  Delay in milliseconds between client polls

#define TPL_WEB_SERVER        TPL_CALLBACK  ///<  TPL for routine synchronization

/**
  Verify new TPL value

  This macro which is enabled when debug is enabled verifies that
  the new TPL value is >= the current TPL value.
**/
#ifdef VERIFY_TPL
#undef VERIFY_TPL
#endif  //  VERIFY_TPL

#if !defined(MDEPKG_NDEBUG)

#define VERIFY_TPL(tpl)                           \
{                                                 \
  EFI_TPL PreviousTpl;                            \
                                                  \
  PreviousTpl = gBS->RaiseTPL ( TPL_HIGH_LEVEL ); \
  gBS->RestoreTPL ( PreviousTpl );                \
  if ( PreviousTpl > tpl ) {                      \
    DEBUG (( DEBUG_ERROR, "Current TPL: %d, New TPL: %d\r\n", PreviousTpl, tpl ));  \
    ASSERT ( PreviousTpl <= tpl );                \
  }                                               \
}

#else   //  MDEPKG_NDEBUG

#define VERIFY_TPL(tpl)

#endif  //  MDEPKG_NDEBUG

#define WEB_SERVER_SIGNATURE        SIGNATURE_32 ('W','e','b','S')  ///<  DT_WEB_SERVER memory signature

#define SPACES_ADDRESS_TO_DATA      2
#define BYTES_ON_A_LINE             16
#define SPACES_BETWEEN_BYTES        1
#define SPACES_DATA_TO_ASCII        2


//------------------------------------------------------------------------------
// Protocol Declarations
//------------------------------------------------------------------------------

extern EFI_COMPONENT_NAME_PROTOCOL gComponentName;    ///<  Component name protocol declaration
extern EFI_COMPONENT_NAME2_PROTOCOL gComponentName2;  ///<  Component name 2 protocol declaration
extern EFI_DRIVER_BINDING_PROTOCOL gDriverBinding;    ///<  Driver binding protocol declaration

//------------------------------------------------------------------------------
//  Data Types
//------------------------------------------------------------------------------

/**
  Port control structure
**/
typedef struct {
  //
  //  Buffer management
  //
  size_t    RequestLength;      ///<  Request length in bytes
  size_t    TxBytes;            ///<  Bytes in the TX buffer
  UINT8     Request[ 65536 ];   ///<  Page request
  UINT8     RxBuffer[ 65536 ];  ///<  Receive buffer
  UINT8     TxBuffer[ 65536 ];  ///<  Transmit buffer
} WSDT_PORT;

/**
  Web server control structure
**/
typedef struct {
  UINTN Signature;              ///<  Structure identification

  //
  //  Image attributes
  //
  EFI_HANDLE ImageHandle;       ///<  Image handle

  //
  //  HTTP port management
  //
  BOOLEAN   bRunning;           ///<  Web server running
  EFI_EVENT TimerEvent;         ///<  Timer to open HTTP port
  int       HttpListenPort;     ///<  File descriptor for the HTTP listen port over TCP4
  int       HttpListenPort6;    ///<  File descriptor for the HTTP listen port over TCP6

  //
  //  Client port management
  //
  nfds_t    MaxEntries;         ///<  Maximum entries in the PortList array
  nfds_t    Entries;            ///<  The current number of entries in the PortList array
  struct pollfd * pFdList;      ///<  List of socket file descriptors
  WSDT_PORT ** ppPortList;      ///<  List of port management structures
} DT_WEB_SERVER;

//#define SERVER_FROM_SERVICE(a) CR (a, DT_WEB_SERVER, ServiceBinding, WEB_SERVER_SIGNATURE)  ///< Locate DT_LAYER from service binding

extern DT_WEB_SERVER mWebServer;

/**
  Process an HTTP request

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
typedef
EFI_STATUS
(* PFN_RESPONSE) (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN BOOLEAN * pbDone
  );

/**
  Data structure to delcare page support routines
**/
typedef struct {
  UINT16 * pPageName;         ///<  Name of the page
  PFN_RESPONSE pfnResponse;   ///<  Routine to generate the response
  UINT16 * pDescription;      ///<  Description of the page
} DT_PAGE;

extern CONST DT_PAGE mPageList[];   ///<  List of pages
extern CONST UINTN mPageCount;      ///<  Number of pages

//------------------------------------------------------------------------------
// Web Pages
//------------------------------------------------------------------------------

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  Respond with the DHCP options

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
DhcpOptionsPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  );

/**
  Respond with the DXE services table

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
DxeServicesTablePage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  );

/**
  Respond with the Exit page

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
ExitPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  );

/**
  Respond with the firmware status

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
FirmwarePage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  );

/**
  Respond with the handles in the system

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HandlePage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  );

/**
  Respond with the Hello World page

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HelloPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  );

/**
  Respond with the list of known pages

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
IndexPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  );

/**
  Page to display the memory map

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
MemoryMapPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  );

/**
  Display the memory type registers

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
MemoryTypeRegistersPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  );

/**
  Respond with the Ports page

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
PortsPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  );

/**
  Page to reboot the system

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
RebootPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  );

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
  );

/**
  Respond with the system table

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
SystemTablePage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  );

//------------------------------------------------------------------------------
// Support routines
//------------------------------------------------------------------------------

/**
  Display the EFI Table Header

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pHeader       Address of the EFI_TABLE_HEADER structure

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
EfiTableHeader (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN EFI_TABLE_HEADER * pHeader
  );

/**
  Buffer the HTTP page header

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pTitle        A zero terminated Unicode title string

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpPageHeader (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST CHAR16 * pTitle
  );

/**
  Buffer and send the HTTP page trailer

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpPageTrailer (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN BOOLEAN * pbDone
  );

/**
  Process an HTTP request

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpRequest (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN BOOLEAN * pbDone
  );

/**
  Buffer data for sending

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] LengthInBytes Length of valid data in the buffer
  @param [in] pBuffer       Buffer of data to send

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpSend (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN size_t LengthInBytes,
  IN CONST UINT8 * pBuffer
  );

/**
  Send an ANSI string

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pString       A zero terminated Unicode string

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpSendAnsiString (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST char * pString
  );

/**
  Buffer a single byte

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] Data          The data byte to send

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpSendByte (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN UINT8 Data
  );

/**
  Display a character

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] Character     Character to display
  @param [in] pReplacement  Replacement character string

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpSendCharacter (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CHAR8 Character,
  IN CHAR8 * pReplacement
  );

/**
  Send a buffer dump
  
  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] ByteCount     The number of bytes to display
  @param [in] pData         Address of the byte array

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpSendDump (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN UINTN ByteCount,
  IN CONST UINT8 * pData
  );

/**
  Display a row containing a GUID value

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pGuid         Address of the GUID to display

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpSendGuid (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST EFI_GUID * pGuid
  );

/**
  Output a hex value to the HTML page

  @param [in] SocketFD    Socket file descriptor
  @param [in] pPort       The WSDT_PORT structure address
  @param [in] Bits        Number of bits to display
  @param [in] Value       Value to display

  @retval EFI_SUCCESS Successfully displayed the address
**/
EFI_STATUS
HttpSendHexBits (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN INT32 Bits,
  IN UINT64 Value
  );

/**
  Output a hex value to the HTML page

  @param [in] SocketFD    Socket file descriptor
  @param [in] pPort       The WSDT_PORT structure address
  @param [in] Value       Value to display

  @retval EFI_SUCCESS Successfully displayed the address
**/
EFI_STATUS
HttpSendHexValue (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN UINT64 Value
  );

/**
  Output an IP address to the HTML page

  @param [in] SocketFD    Socket file descriptor
  @param [in] pPort       The WSDT_PORT structure address
  @param [in] pAddress    Address of the socket address

  @retval EFI_SUCCESS Successfully displayed the address
**/
EFI_STATUS
HttpSendIpAddress (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN struct sockaddr_in6 * pAddress
  );

/**
  Send a Unicode string

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pString       A zero terminated Unicode string

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpSendUnicodeString (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST UINT16 * pString
  );

/**
  Output a value to the HTML page

  @param [in] SocketFD    Socket file descriptor
  @param [in] pPort       The WSDT_PORT structure address
  @param [in] Value       Value to display

  @retval EFI_SUCCESS Successfully displayed the address
**/
EFI_STATUS
HttpSendValue (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN UINT64 Value
  );

/**
  Display a row containing a decimal value

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pName         Address of a zero terminated name string
  @param [in] Value         The value to display

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
RowDecimalValue (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST CHAR8 * pName,
  IN UINT64 Value
  );

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
  );

/**
  Display a row containing a hex value

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pName         Address of a zero terminated name string
  @param [in] Value         The value to display
  @param [in] pWebPage      Address of a zero terminated web page name

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
RowHexValue (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST CHAR8 * pName,
  IN UINT64 Value,
  IN CONST CHAR16 * pWebPage
  );

/**
  Display a row containing a pointer

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pName         Address of a zero terminated name string
  @param [in] pAddress      The address to display
  @param [in] pWebPage      Address of a zero terminated web page name

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
RowPointer (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST CHAR8 * pName,
  IN CONST VOID * pAddress,
  IN CONST CHAR16 * pWebPage
  );

/**
  Display a row containing a revision

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pName         Address of a zero terminated name string
  @param [in] Revision      The revision to display

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
RowRevision (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST CHAR8 * pName,
  IN UINT32 Revision
  );

/**
  Display a row containing a unicode string

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pName         Address of a zero terminated name string
  @param [in] pString       Address of a zero terminated unicode string

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
RowUnicodeString (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST CHAR8 * pName,
  IN CONST CHAR16 * pString
  );

/**
  Start the table page

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pName         Address of a zero terminated name string
  @param [in] pTable        Address of the table

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
TableHeader (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST CHAR16 * pName,
  IN CONST VOID * pTable
  );

/**
  End the table page

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
TableTrailer (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN *pbDone
  );

/**
  HTTP port creation timer routine

  This routine polls the socket layer waiting for the initial network connection
  which will enable the creation of the HTTP port.  The socket layer will manage
  the coming and going of the network connections after that until the last network
  connection is broken.

  @param [in] pWebServer  The web server control structure address.

**/
VOID
WebServerTimer (
  IN DT_WEB_SERVER * pWebServer
  );

/**
  Start the web server port creation timer

  @param [in] pWebServer  The web server control structure address.

  @retval EFI_SUCCESS         The timer was successfully started.
  @retval EFI_ALREADY_STARTED The timer is already running.
  @retval Other               The timer failed to start.

**/
EFI_STATUS
WebServerTimerStart (
  IN DT_WEB_SERVER * pWebServer
  );

/**
  Stop the web server port creation timer

  @param [in] pWebServer  The web server control structure address.

  @retval EFI_SUCCESS   The HTTP port timer is stopped
  @retval Other         Failed to stop the HTTP port timer

**/
EFI_STATUS
WebServerTimerStop (
  IN DT_WEB_SERVER * pWebServer
  );

//------------------------------------------------------------------------------
// Driver Binding Protocol Support
//------------------------------------------------------------------------------

/**
  Stop this driver on Controller by removing NetworkInterfaceIdentifier protocol and
  closing the DevicePath and PciIo protocols on Controller.

  @param [in] pThis                Protocol instance pointer.
  @param [in] Controller           Handle of device to stop driver on.
  @param [in] NumberOfChildren     How many children need to be stopped.
  @param [in] pChildHandleBuffer   Not used.

  @retval EFI_SUCCESS          This driver is removed Controller.
  @retval EFI_DEVICE_ERROR     The device could not be stopped due to a device error.
  @retval other                This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
DriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL * pThis,
  IN  EFI_HANDLE Controller,
  IN  UINTN NumberOfChildren,
  IN  EFI_HANDLE * pChildHandleBuffer
  );

//------------------------------------------------------------------------------
// EFI Component Name Protocol Support
//------------------------------------------------------------------------------

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param [in] pThis             A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param [in] pLanguage         A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 3066 or ISO 639-2 language code format.
  @param [out] ppDriverName     A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER DriverName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
GetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL * pThis,
  IN  CHAR8 * pLanguage,
  OUT CHAR16 ** ppDriverName
  );


/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param [in] pThis             A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param [in] ControllerHandle  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.
  @param [in] ChildHandle       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.
  @param [in] pLanguage         A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 3066 or ISO 639-2 language code format.
  @param [out] ppControllerName A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.
  @retval EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER ControllerName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
GetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL * pThis,
  IN  EFI_HANDLE ControllerHandle,
  IN OPTIONAL EFI_HANDLE ChildHandle,
  IN  CHAR8 * pLanguage,
  OUT CHAR16 ** ppControllerName
  );

//------------------------------------------------------------------------------

#endif  //  _WEB_SERVER_H_
