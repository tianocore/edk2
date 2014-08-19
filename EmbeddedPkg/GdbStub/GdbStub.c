/** @file
  UEFI driver that implements a GDB stub

  Note: Any code in the path of the Serial IO output can not call DEBUG as will
  will blow out the stack. Serial IO calls DEBUG, debug calls Serail IO, ...


  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <GdbStubInternal.h>
#include <Protocol/DebugPort.h>


UINTN     gMaxProcessorIndex = 0;

//
// Buffers for basic gdb communication
//
CHAR8 gInBuffer[MAX_BUF_SIZE];
CHAR8 gOutBuffer[MAX_BUF_SIZE];

// Assume gdb does a "qXfer:libraries:read::offset,length" when it connects so we can default
// this value to FALSE. Since gdb can reconnect its self a global default is not good enough
BOOLEAN   gSymbolTableUpdate = FALSE;
EFI_EVENT gEvent;
VOID      *gGdbSymbolEventHandlerRegistration = NULL;

//
// Globals for returning XML from qXfer:libraries:read packet
//
UINTN                             gPacketqXferLibraryOffset = 0;
UINTN                             gEfiDebugImageTableEntry = 0;
EFI_DEBUG_IMAGE_INFO_TABLE_HEADER *gDebugImageTableHeader = NULL;
EFI_DEBUG_IMAGE_INFO              *gDebugTable = NULL;
CHAR8                             gXferLibraryBuffer[2000];

GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 mHexToStr[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};


VOID
EFIAPI
GdbSymbolEventHandler (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
{
}


/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the image goes into a library that calls this
  function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
GdbStubEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  EFI_DEBUG_SUPPORT_PROTOCOL  *DebugSupport;
  UINTN                       HandleCount;
  EFI_HANDLE                  *Handles;
  UINTN                       Index;
  UINTN                       Processor;
  BOOLEAN                     IsaSupported;

  Status = EfiGetSystemConfigurationTable (&gEfiDebugImageInfoTableGuid, (VOID **)&gDebugImageTableHeader);
  if (EFI_ERROR (Status)) {
    gDebugImageTableHeader = NULL;
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDebugSupportProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Debug Support Protocol not found\n"));

    return Status;
  }

  DebugSupport = NULL;
  IsaSupported = FALSE;
  do {
    HandleCount--;
    Status = gBS->HandleProtocol (
                    Handles[HandleCount],
                    &gEfiDebugSupportProtocolGuid,
                    (VOID **) &DebugSupport
                    );
    if (!EFI_ERROR (Status)) {
      if (CheckIsa (DebugSupport->Isa)) {
        // We found what we are looking for so break out of the loop
        IsaSupported = TRUE;
        break;
      }
    }
  } while (HandleCount > 0);
  FreePool (Handles);

  if (!IsaSupported) {
    DEBUG ((EFI_D_ERROR, "Debug Support Protocol does not support our ISA\n"));

    return EFI_NOT_FOUND;
  }

  Status = DebugSupport->GetMaximumProcessorIndex (DebugSupport, &gMaxProcessorIndex);
  ASSERT_EFI_ERROR (Status);

  DEBUG ((EFI_D_INFO, "Debug Support Protocol ISA %x\n", DebugSupport->Isa));
  DEBUG ((EFI_D_INFO, "Debug Support Protocol Processor Index %d\n", gMaxProcessorIndex));

  // Call processor-specific init routine
  InitializeProcessor ();

  for (Processor = 0; Processor <= gMaxProcessorIndex; Processor++) {
    for (Index = 0; Index < MaxEfiException (); Index++) {
      Status = DebugSupport->RegisterExceptionCallback (DebugSupport, Processor,  GdbExceptionHandler, gExceptionType[Index].Exception);
      ASSERT_EFI_ERROR (Status);
    }
    //
    // Current edk2 DebugPort is not interrupt context safe so we can not use it
    //
    Status = DebugSupport->RegisterPeriodicCallback (DebugSupport, Processor, GdbPeriodicCallBack);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // This even fires every time an image is added. This allows the stub to know when gdb needs
  // to update the symbol table.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  GdbSymbolEventHandler,
                  NULL,
                  &gEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for protocol notifications on this event
  //
  Status = gBS->RegisterProtocolNotify (
                  &gEfiLoadedImageProtocolGuid,
                  gEvent,
                  &gGdbSymbolEventHandlerRegistration
                  );
  ASSERT_EFI_ERROR (Status);


 if (PcdGetBool (PcdGdbSerial)) {
   GdbInitializeSerialConsole ();
 }

  return EFI_SUCCESS;
}

/**
 Transfer length bytes of input buffer, starting at Address, to memory.

 @param     length                  the number of the bytes to be transferred/written
 @param     *address                the start address of the transferring/writing the memory
 @param     *new_data               the new data to be written to memory
 **/

VOID
TransferFromInBufToMem (
  IN    UINTN                       Length,
  IN    unsigned char               *Address,
  IN    CHAR8                       *NewData
  )
{
  CHAR8 c1;
  CHAR8 c2;

  while (Length-- > 0) {
    c1 = (CHAR8)HexCharToInt (*NewData++);
    c2 = (CHAR8)HexCharToInt (*NewData++);

    if ((c1 < 0) || (c2 < 0)) {
      Print ((CHAR16 *)L"Bad message from write to memory..\n");
      SendError (GDB_EBADMEMDATA);
      return;
    }
    *Address++ = (UINT8)((c1 << 4) + c2);
  }

  SendSuccess();
}


/**
 Transfer Length bytes of memory starting at Address to an output buffer, OutBuffer. This function will finally send the buffer
 as a packet.

 @param     Length                  the number of the bytes to be transferred/read
 @param     *address                pointer to the start address of the transferring/reading the memory
 **/

VOID
TransferFromMemToOutBufAndSend (
  IN    UINTN                       Length,
  IN    unsigned char               *Address
  )
{
  // there are Length bytes and every byte is represented as 2 hex chars
  CHAR8   OutBuffer[MAX_BUF_SIZE];
  CHAR8   *OutBufPtr;             // pointer to the output buffer
  CHAR8   Char;

  if (ValidateAddress(Address) == FALSE) {
    SendError(14);
    return;
  }

  OutBufPtr = OutBuffer;
  while (Length > 0) {

    Char = mHexToStr[*Address >> 4];
    if ((Char >= 'A') && (Char <= 'F')) {
      Char = Char - 'A' + 'a';
    }
    *OutBufPtr++ = Char;

    Char = mHexToStr[*Address & 0x0f];
    if ((Char >= 'A') && (Char <= 'F')) {
      Char = Char - 'A' + 'a';
    }
    *OutBufPtr++ = Char;

    Address++;
    Length--;
  }

  *OutBufPtr = '\0' ;  // the end of the buffer
  SendPacket (OutBuffer);
}



/**
  Send a GDB Remote Serial Protocol Packet

  $PacketData#checksum PacketData is passed in and this function adds the packet prefix '$',
  the packet teminating character '#' and the two digit checksum.

  If an ack '+' is not sent resend the packet, but timeout eventually so we don't end up
  in an infinit loop. This is so if you unplug the debugger code just keeps running

  @param PacketData   Payload data for the packet


  @retval             Number of bytes of packet data sent.

**/
UINTN
SendPacket (
  IN  CHAR8 *PacketData
  )
{
  UINT8 CheckSum;
  UINTN Timeout;
  CHAR8 *Ptr;
  CHAR8 TestChar;
  UINTN Count;

  Timeout = PcdGet32 (PcdGdbMaxPacketRetryCount);

  Count = 0;
  do {

    Ptr = PacketData;

    if (Timeout-- == 0) {
      // Only try a finite number of times so we don't get stuck in the loop
      return Count;
    }

    // Packet prefix
    GdbPutChar ('$');

    for (CheckSum = 0, Count =0 ; *Ptr != '\0'; Ptr++, Count++) {
      GdbPutChar (*Ptr);
      CheckSum = CheckSum + *Ptr;
    }

    // Packet terminating character and checksum
    GdbPutChar ('#');
    GdbPutChar (mHexToStr[CheckSum >> 4]);
    GdbPutChar (mHexToStr[CheckSum & 0x0F]);

    TestChar =  GdbGetChar ();
  } while (TestChar != '+');

  return Count;
}

/**
  Receive a GDB Remote Serial Protocol Packet

  $PacketData#checksum PacketData is passed in and this function adds the packet prefix '$',
  the packet teminating character '#' and the two digit checksum.

  If host re-starts sending a packet without ending the previous packet, only the last valid packet is proccessed.
  (In other words, if received packet is '$12345$12345$123456#checksum', only '$123456#checksum' will be processed.)

  If an ack '+' is not sent resend the packet

  @param PacketData   Payload data for the packet

  @retval             Number of bytes of packet data received.

**/
UINTN
ReceivePacket (
  OUT  CHAR8 *PacketData,
  IN   UINTN PacketDataSize
 )
{
  UINT8 CheckSum;
  UINTN Index;
  CHAR8 Char;
  CHAR8 SumString[3];
  CHAR8 TestChar;

  ZeroMem (PacketData, PacketDataSize);

  for (;;) {
      // wait for the start of a packet
    TestChar = GdbGetChar ();
    while (TestChar != '$') {
      TestChar = GdbGetChar ();
    };

  retry:
    for (Index = 0, CheckSum = 0; Index < (PacketDataSize - 1); Index++) {
      Char = GdbGetChar ();
      if (Char == '$') {
        goto retry;
      }
      if (Char == '#') {
        break;
      }

      PacketData[Index] = Char;
      CheckSum = CheckSum + Char;
    }
    PacketData[Index] = '\0';

    if (Index == PacketDataSize) {
      continue;
    }

    SumString[0] = GdbGetChar ();
    SumString[1] = GdbGetChar ();
    SumString[2] = '\0';

    if (AsciiStrHexToUintn (SumString) == CheckSum) {
      // Ack: Success
      GdbPutChar ('+');

      // Null terminate the callers string
      PacketData[Index] = '\0';
      return Index;
    } else {
      // Ack: Failure
      GdbPutChar ('-');
    }
  }

  //return 0;
}


/**
 Empties the given buffer
 @param   Buf          pointer to the first element in buffer to be emptied
 **/
VOID
EmptyBuffer (
  IN  CHAR8           *Buf
  )
{
  *Buf = '\0';
}


/**
 Converts an 8-bit Hex Char into a INTN.

 @param   Char the hex character to be converted into UINTN
 @retval  a INTN, from 0 to 15, that corressponds to Char
 -1 if Char is not a hex character
 **/
INTN
HexCharToInt (
  IN  CHAR8           Char
  )
{
  if ((Char >= 'A') && (Char <= 'F')) {
    return Char - 'A' + 10;
  } else if ((Char >= 'a') && (Char <= 'f')) {
    return Char - 'a' + 10;
  } else if ((Char >= '0') && (Char <= '9')) {
    return Char - '0';
  } else { // if not a hex value, return a negative value
    return -1;
  }
}

  // 'E' + the biggest error number is 255, so its 2 hex digits + buffer end
CHAR8 *gError = "E__";

/** 'E NN'
 Send an error with the given error number after converting to hex.
 The error number is put into the buffer in hex. '255' is the biggest errno we can send.
 ex: 162 will be sent as A2.

 @param   errno           the error number that will be sent
 **/
VOID
EFIAPI
SendError (
  IN  UINT8              ErrorNum
  )
{
  //
  // Replace _, or old data, with current errno
  //
  gError[1] = mHexToStr [ErrorNum >> 4];
  gError[2] = mHexToStr [ErrorNum & 0x0f];

  SendPacket (gError); // send buffer
}



/**
 Send 'OK' when the function is done executing successfully.
 **/
VOID
EFIAPI
SendSuccess (
  VOID
  )
{
  SendPacket ("OK"); // send buffer
}


/**
 Send empty packet to specify that particular command/functionality is not supported.
 **/
VOID
EFIAPI
SendNotSupported (
  VOID
  )
{
  SendPacket ("");
}


/**
 Send the T signal with the given exception type (in gdb order) and possibly with n:r pairs related to the watchpoints

 @param  SystemContext        Register content at time of the exception
 @param  GdbExceptionType     GDB exception type
 **/
VOID
GdbSendTSignal (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINT8               GdbExceptionType
  )
{
  CHAR8 TSignalBuffer[128];
  CHAR8 *TSignalPtr;
  UINTN BreakpointDetected;
  BREAK_TYPE BreakType;
  UINTN DataAddress;
  CHAR8 *WatchStrPtr = NULL;
  UINTN RegSize;

  TSignalPtr = &TSignalBuffer[0];

  //Construct TSignal packet
  *TSignalPtr++ = 'T';

  //
  // replace _, or previous value, with Exception type
  //
  *TSignalPtr++ = mHexToStr [GdbExceptionType >> 4];
  *TSignalPtr++ = mHexToStr [GdbExceptionType & 0x0f];

  if (GdbExceptionType == GDB_SIGTRAP) {
    if (gSymbolTableUpdate) {
      //
      // We can only send back on reason code. So if the flag is set it means the breakpoint is from our event handler
      //
      WatchStrPtr = "library:;";
      while (*WatchStrPtr != '\0') {
        *TSignalPtr++ = *WatchStrPtr++;
      }
      gSymbolTableUpdate = FALSE;
    } else {


      //
      // possible n:r pairs
      //

      //Retrieve the breakpoint number
      BreakpointDetected = GetBreakpointDetected (SystemContext);

      //Figure out if the exception is happend due to watch, rwatch or awatch.
      BreakType = GetBreakpointType (SystemContext, BreakpointDetected);

      //INFO: rwatch is not supported due to the way IA32 debug registers work
      if ((BreakType == DataWrite) || (BreakType == DataRead) || (BreakType == DataReadWrite)) {

        //Construct n:r pair
        DataAddress = GetBreakpointDataAddress (SystemContext, BreakpointDetected);

        //Assign appropriate buffer to print particular watchpoint type
        if (BreakType == DataWrite) {
          WatchStrPtr = "watch";
        } else if (BreakType == DataRead) {
          WatchStrPtr = "rwatch";
        } else if (BreakType == DataReadWrite) {
          WatchStrPtr = "awatch";
        }

        while (*WatchStrPtr != '\0') {
          *TSignalPtr++ = *WatchStrPtr++;
        }

        *TSignalPtr++ = ':';

        //Set up series of bytes in big-endian byte order. "awatch" won't work with little-endian byte order.
        RegSize = REG_SIZE;
        while (RegSize > 0) {
          RegSize = RegSize-4;
          *TSignalPtr++ = mHexToStr[(UINT8)(DataAddress >> RegSize) & 0xf];
        }

        //Always end n:r pair with ';'
        *TSignalPtr++ = ';';
      }
    }
  }

  *TSignalPtr = '\0';

  SendPacket (TSignalBuffer);
}


/**
 Translates the EFI mapping to GDB mapping

 @param   EFIExceptionType    EFI Exception that is being processed
 @retval  UINTN that corresponds to EFIExceptionType's GDB exception type number
 **/
UINT8
ConvertEFItoGDBtype (
  IN  EFI_EXCEPTION_TYPE      EFIExceptionType
  )
{
  UINTN Index;

  for (Index = 0; Index < MaxEfiException () ; Index++) {
    if (gExceptionType[Index].Exception == EFIExceptionType) {
      return gExceptionType[Index].SignalNo;
    }
  }
  return GDB_SIGTRAP; // this is a GDB trap
}


/** "m addr,length"
 Find the Length of the area to read and the start addres. Finally, pass them to
 another function, TransferFromMemToOutBufAndSend, that will read from that memory space and
 send it as a packet.
 **/

VOID
EFIAPI
ReadFromMemory (
  CHAR8 *PacketData
  )
{
  UINTN Address;
  UINTN Length;
  CHAR8 AddressBuffer[MAX_ADDR_SIZE]; // the buffer that will hold the address in hex chars
  CHAR8 *AddrBufPtr; // pointer to the address buffer
  CHAR8 *InBufPtr; /// pointer to the input buffer

  AddrBufPtr = AddressBuffer;
  InBufPtr = &PacketData[1];
  while (*InBufPtr != ',') {
    *AddrBufPtr++ = *InBufPtr++;
  }
  *AddrBufPtr = '\0';

  InBufPtr++; // this skips ',' in the buffer

  /* Error checking */
  if (AsciiStrLen (AddressBuffer) >= MAX_ADDR_SIZE) {
    Print((CHAR16 *)L"Address is too long\n");
    SendError (GDB_EBADMEMADDRBUFSIZE);
    return;
  }

  // 2 = 'm' + ','
  if (AsciiStrLen (PacketData) - AsciiStrLen (AddressBuffer) - 2 >= MAX_LENGTH_SIZE) {
    Print((CHAR16 *)L"Length is too long\n");
    SendError (GDB_EBADMEMLENGTH);
    return;
  }

  Address = AsciiStrHexToUintn (AddressBuffer);
  Length = AsciiStrHexToUintn (InBufPtr);

  TransferFromMemToOutBufAndSend (Length, (unsigned char *)Address);
}


/** "M addr,length :XX..."
 Find the Length of the area in bytes to write and the start addres. Finally, pass them to
 another function, TransferFromInBufToMem, that will write to that memory space the info in
 the input buffer.
 **/
VOID
EFIAPI
WriteToMemory (
  IN CHAR8 *PacketData
  )
{
  UINTN Address;
  UINTN Length;
  UINTN MessageLength;
  CHAR8 AddressBuffer[MAX_ADDR_SIZE]; // the buffer that will hold the Address in hex chars
  CHAR8 LengthBuffer[MAX_LENGTH_SIZE]; // the buffer that will hold the Length in hex chars
  CHAR8 *AddrBufPtr; // pointer to the Address buffer
  CHAR8 *LengthBufPtr; // pointer to the Length buffer
  CHAR8 *InBufPtr; /// pointer to the input buffer

  AddrBufPtr = AddressBuffer;
  LengthBufPtr = LengthBuffer;
  InBufPtr = &PacketData[1];

  while (*InBufPtr != ',') {
    *AddrBufPtr++ = *InBufPtr++;
  }
  *AddrBufPtr = '\0';

  InBufPtr++; // this skips ',' in the buffer

  while (*InBufPtr != ':') {
    *LengthBufPtr++ = *InBufPtr++;
  }
  *LengthBufPtr = '\0';

  InBufPtr++; // this skips ':' in the buffer

  Address = AsciiStrHexToUintn (AddressBuffer);
  Length = AsciiStrHexToUintn (LengthBuffer);

  /* Error checking */

  //Check if Address is not too long.
  if (AsciiStrLen (AddressBuffer) >= MAX_ADDR_SIZE) {
    Print ((CHAR16 *)L"Address too long..\n");
    SendError (GDB_EBADMEMADDRBUFSIZE);
    return;
  }

  //Check if message length is not too long
  if (AsciiStrLen (LengthBuffer) >= MAX_LENGTH_SIZE) {
    Print ((CHAR16 *)L"Length too long..\n");
    SendError (GDB_EBADMEMLENGBUFSIZE);
    return;
  }

  // Check if Message is not too long/short.
  // 3 = 'M' + ',' + ':'
  MessageLength = (AsciiStrLen (PacketData) - AsciiStrLen (AddressBuffer) - AsciiStrLen (LengthBuffer) - 3);
  if (MessageLength != (2*Length)) {
    //Message too long/short. New data is not the right size.
    SendError (GDB_EBADMEMDATASIZE);
    return;
  }
  TransferFromInBufToMem (Length, (unsigned char *)Address, InBufPtr);
}

/**
  Parses breakpoint packet data and captures Breakpoint type, Address and length.
  In case of an error, function returns particular error code. Returning 0 meaning
  no error.

  @param  PacketData  Pointer to the payload data for the packet.
  @param  Type        Breakpoint type
  @param  Address     Breakpoint address
  @param  Length      Breakpoint length in Bytes (1 byte, 2 byte, 4 byte)

  @retval 1           Success
  @retval {other}     Particular error code

**/
UINTN
ParseBreakpointPacket (
  IN  CHAR8 *PacketData,
  OUT UINTN *Type,
  OUT UINTN *Address,
  OUT UINTN *Length
  )
{
  CHAR8 AddressBuffer[MAX_ADDR_SIZE];
  CHAR8 *AddressBufferPtr;
  CHAR8 *PacketDataPtr;

  PacketDataPtr = &PacketData[1];
  AddressBufferPtr = AddressBuffer;

  *Type = AsciiStrHexToUintn (PacketDataPtr);

  //Breakpoint/watchpoint type should be between 0 to 4
  if (*Type > 4) {
    Print ((CHAR16 *)L"Type is invalid\n");
    return 22; //EINVAL: Invalid argument.
  }

  //Skip ',' in the buffer.
  while (*PacketDataPtr++ != ',');

  //Parse Address information
  while (*PacketDataPtr != ',') {
    *AddressBufferPtr++ = *PacketDataPtr++;
  }
  *AddressBufferPtr = '\0';

  //Check if Address is not too long.
  if (AsciiStrLen (AddressBuffer) >= MAX_ADDR_SIZE) {
    Print ((CHAR16 *)L"Address too long..\n");
    return 40; //EMSGSIZE: Message size too long.
  }

  *Address = AsciiStrHexToUintn (AddressBuffer);

  PacketDataPtr++; //This skips , in the buffer

  //Parse Length information
  *Length = AsciiStrHexToUintn (PacketDataPtr);

  //Length should be 1, 2 or 4 bytes
  if (*Length > 4) {
    Print ((CHAR16 *)L"Length is invalid\n");
    return 22; //EINVAL: Invalid argument
  }

  return 0; //0 = No error
}

UINTN
gXferObjectReadResponse (
  IN  CHAR8         Type,
  IN  CHAR8         *Str
  )
{
  CHAR8   *OutBufPtr;             // pointer to the output buffer
  CHAR8   Char;
  UINTN   Count;

  // Response starts with 'm' or 'l' if it is the end
  OutBufPtr = gOutBuffer;
  *OutBufPtr++ = Type;
  Count = 1;

  // Binary data encoding
  OutBufPtr = gOutBuffer;
  while (*Str != '\0') {
    Char = *Str++;
    if ((Char == 0x7d) || (Char == 0x23) || (Char == 0x24) || (Char == 0x2a)) {
      // escape character
      *OutBufPtr++ = 0x7d;

      Char ^= 0x20;
    }
    *OutBufPtr++ = Char;
    Count++;
  }

  *OutBufPtr = '\0' ;  // the end of the buffer
  SendPacket (gOutBuffer);

  return Count;
}


/**
  Note: This should be a library function.  In the Apple case you have to add
  the size of the PE/COFF header into the starting address to make things work
  right as there is no way to pad the Mach-O for the size of the PE/COFF header.


  Returns a pointer to the PDB file name for a PE/COFF image that has been
  loaded into system memory with the PE/COFF Loader Library functions.

  Returns the PDB file name for the PE/COFF image specified by Pe32Data.  If
  the PE/COFF image specified by Pe32Data is not a valid, then NULL is
  returned.  If the PE/COFF image specified by Pe32Data does not contain a
  debug directory entry, then NULL is returned.  If the debug directory entry
  in the PE/COFF image specified by Pe32Data does not contain a PDB file name,
  then NULL is returned.
  If Pe32Data is NULL, then ASSERT().

  @param  Pe32Data   Pointer to the PE/COFF image that is loaded in system
                     memory.
  @param  DebugBase  Address that the debugger would use as the base of the image

  @return The PDB file name for the PE/COFF image specified by Pe32Data or NULL
          if it cannot be retrieved. DebugBase is only valid if PDB file name is
          valid.

**/
VOID *
EFIAPI
PeCoffLoaderGetDebuggerInfo (
  IN VOID     *Pe32Data,
  OUT VOID    **DebugBase
  )
{
  EFI_IMAGE_DOS_HEADER                  *DosHdr;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr;
  EFI_IMAGE_DATA_DIRECTORY              *DirectoryEntry;
  EFI_IMAGE_DEBUG_DIRECTORY_ENTRY       *DebugEntry;
  UINTN                                 DirCount;
  VOID                                  *CodeViewEntryPointer;
  INTN                                  TEImageAdjust;
  UINT32                                NumberOfRvaAndSizes;
  UINT16                                Magic;
  UINTN                                 SizeOfHeaders;

  ASSERT (Pe32Data   != NULL);

  TEImageAdjust       = 0;
  DirectoryEntry      = NULL;
  DebugEntry          = NULL;
  NumberOfRvaAndSizes = 0;
  SizeOfHeaders       = 0;

  DosHdr = (EFI_IMAGE_DOS_HEADER *)Pe32Data;
  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    //
    // DOS image header is present, so read the PE header after the DOS image header.
    //
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)((UINTN) Pe32Data + (UINTN) ((DosHdr->e_lfanew) & 0x0ffff));
  } else {
    //
    // DOS image header is not present, so PE header is at the image base.
    //
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)Pe32Data;
  }

  if (Hdr.Te->Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
    if (Hdr.Te->DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress != 0) {
      DirectoryEntry  = &Hdr.Te->DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG];
      TEImageAdjust   = sizeof (EFI_TE_IMAGE_HEADER) - Hdr.Te->StrippedSize;
      DebugEntry = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *)((UINTN) Hdr.Te +
                    Hdr.Te->DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress +
                    TEImageAdjust);
    }
    SizeOfHeaders = sizeof (EFI_TE_IMAGE_HEADER) + (UINTN)Hdr.Te->BaseOfCode - (UINTN)Hdr.Te->StrippedSize;

    // __APPLE__ check this math...
    *DebugBase = ((CHAR8 *)Pe32Data) -  TEImageAdjust;
  } else if (Hdr.Pe32->Signature == EFI_IMAGE_NT_SIGNATURE) {

    *DebugBase = Pe32Data;


    //
    // NOTE: We use Machine field to identify PE32/PE32+, instead of Magic.
    //       It is due to backward-compatibility, for some system might
    //       generate PE32+ image with PE32 Magic.
    //
    switch (Hdr.Pe32->FileHeader.Machine) {
    case EFI_IMAGE_MACHINE_IA32:
      //
      // Assume PE32 image with IA32 Machine field.
      //
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
      break;
    case EFI_IMAGE_MACHINE_X64:
    case EFI_IMAGE_MACHINE_IA64:
      //
      // Assume PE32+ image with X64 or IPF Machine field
      //
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
      break;
    default:
      //
      // For unknow Machine field, use Magic in optional Header
      //
      Magic = Hdr.Pe32->OptionalHeader.Magic;
    }

    if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      //
      // Use PE32 offset get Debug Directory Entry
      //
      SizeOfHeaders = Hdr.Pe32->OptionalHeader.SizeOfHeaders;
      NumberOfRvaAndSizes = Hdr.Pe32->OptionalHeader.NumberOfRvaAndSizes;
      DirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *)&(Hdr.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG]);
      DebugEntry     = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *) ((UINTN) Pe32Data + DirectoryEntry->VirtualAddress);
    } else if (Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
      //
      // Use PE32+ offset get Debug Directory Entry
      //
      SizeOfHeaders = Hdr.Pe32Plus->OptionalHeader.SizeOfHeaders;
      NumberOfRvaAndSizes = Hdr.Pe32Plus->OptionalHeader.NumberOfRvaAndSizes;
      DirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *)&(Hdr.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG]);
      DebugEntry     = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *) ((UINTN) Pe32Data + DirectoryEntry->VirtualAddress);
    }

    if (NumberOfRvaAndSizes <= EFI_IMAGE_DIRECTORY_ENTRY_DEBUG) {
      DirectoryEntry = NULL;
      DebugEntry = NULL;
    }
  } else {
    return NULL;
  }

  if (DebugEntry == NULL || DirectoryEntry == NULL) {
    return NULL;
  }

  for (DirCount = 0; DirCount < DirectoryEntry->Size; DirCount += sizeof (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY), DebugEntry++) {
    if (DebugEntry->Type == EFI_IMAGE_DEBUG_TYPE_CODEVIEW) {
      if (DebugEntry->SizeOfData > 0) {
        CodeViewEntryPointer = (VOID *) ((UINTN) DebugEntry->RVA + ((UINTN)Pe32Data) + (UINTN)TEImageAdjust);
        switch (* (UINT32 *) CodeViewEntryPointer) {
        case CODEVIEW_SIGNATURE_NB10:
          return (VOID *) ((CHAR8 *)CodeViewEntryPointer + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY));
        case CODEVIEW_SIGNATURE_RSDS:
          return (VOID *) ((CHAR8 *)CodeViewEntryPointer + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_RSDS_ENTRY));
        case CODEVIEW_SIGNATURE_MTOC:
          *DebugBase = (VOID *)(UINTN)((UINTN)DebugBase - SizeOfHeaders);
          return (VOID *) ((CHAR8 *)CodeViewEntryPointer + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_MTOC_ENTRY));
        default:
          break;
        }
      }
    }
  }

  (void)SizeOfHeaders;
  return NULL;
}


/**
  Process "qXfer:object:read:annex:offset,length" request.

  Returns an XML document that contains loaded libraries. In our case it is
  information in the EFI Debug Image Table converted into an XML document.

  GDB will call with an arbitrary length (it can't know the real length and
  will reply with chunks of XML that are easy for us to deal with. Gdb will
  keep calling until we say we are done. XML doc looks like:

  <library-list>
    <library name="/a/a/c/d.dSYM"><segment address="0x10000000"/></library>
    <library name="/a/m/e/e.pdb"><segment address="0x20000000"/></library>
    <library name="/a/l/f/f.dll"><segment address="0x30000000"/></library>
  </library-list>

  Since we can not allocate memory in interrupt context this module has
  assumptions about how it will get called:
  1) Length will generally be max remote packet size (big enough)
  2) First Offset of an XML document read needs to be 0
  3) This code will return back small chunks of the XML document on every read.
     Each subsequent call will ask for the next available part of the document.

  Note: The only variable size element in the XML is:
  "  <library name=\"%s\"><segment address=\"%p\"/></library>\n" and it is
  based on the file path and name of the symbol file. If the symbol file name
  is bigger than the max gdb remote packet size we could update this code
  to respond back in chunks.

 @param Offset  offset into special data area
 @param Length  number of bytes to read starting at Offset

 **/
VOID
QxferLibrary (
  IN  UINTN   Offset,
  IN  UINTN   Length
  )
{
  VOID                              *LoadAddress;
  CHAR8                             *Pdb;
  UINTN                             Size;

  if (Offset != gPacketqXferLibraryOffset) {
    SendError (GDB_EINVALIDARG);
    Print (L"\nqXferLibrary (%d, %d) != %d\n", Offset, Length, gPacketqXferLibraryOffset);

    // Force a retry from the beginning
    gPacketqXferLibraryOffset = 0;

    return;
  }

  if (Offset == 0) {
    gPacketqXferLibraryOffset += gXferObjectReadResponse ('m', "<library-list>\n");

    // The owner of the table may have had to ralloc it so grab a fresh copy every time
    // we assume qXferLibrary will get called over and over again until the entire XML table is
    // returned in a tight loop. Since we are in the debugger the table should not get updated
    gDebugTable = gDebugImageTableHeader->EfiDebugImageInfoTable;
    gEfiDebugImageTableEntry = 0;
    return;
  }

  if (gDebugTable != NULL) {
    for (; gEfiDebugImageTableEntry < gDebugImageTableHeader->TableSize; gEfiDebugImageTableEntry++, gDebugTable++) {
      if (gDebugTable->NormalImage != NULL) {
        if ((gDebugTable->NormalImage->ImageInfoType == EFI_DEBUG_IMAGE_INFO_TYPE_NORMAL) &&
            (gDebugTable->NormalImage->LoadedImageProtocolInstance != NULL)) {
          Pdb = PeCoffLoaderGetDebuggerInfo (
                 gDebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase,
                 &LoadAddress
                 );
          if (Pdb != NULL) {
            Size = AsciiSPrint (
                    gXferLibraryBuffer,
                    sizeof (gXferLibraryBuffer),
                    "  <library name=\"%a\"><segment address=\"0x%p\"/></library>\n",
                    Pdb,
                    LoadAddress
                    );
            if ((Size != 0) && (Size != (sizeof (gXferLibraryBuffer) - 1))) {
              gPacketqXferLibraryOffset += gXferObjectReadResponse ('m', gXferLibraryBuffer);

              // Update loop variables so we are in the right place when we get back
              gEfiDebugImageTableEntry++;
              gDebugTable++;
              return;
            } else {
              // We could handle <library> entires larger than sizeof (gXferLibraryBuffer) here if
              // needed by breaking up into N packets
              // "<library name=\"%s
              // the rest of the string (as many packets as required
              // \"><segment address=\"%d\"/></library> (fixed size)
              //
              // But right now we just skip any entry that is too big
            }
          }
        }
      }
    }
  }


  gXferObjectReadResponse ('l', "</library-list>\n");
  gPacketqXferLibraryOffset = 0;
  return;
}


/**
 Exception Hanldler for GDB. It will be called for all exceptions
 registered via the gExceptionType[] array.

 @param ExceptionType     Exception that is being processed
 @param SystemContext     Register content at time of the exception
 **/
VOID
EFIAPI
GdbExceptionHandler (
  IN  EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT     SystemContext
  )
{
  UINT8   GdbExceptionType;
  CHAR8   *Ptr;


  if (ValidateException (ExceptionType, SystemContext) == FALSE) {
    return;
  }

  RemoveSingleStep (SystemContext);

  GdbExceptionType = ConvertEFItoGDBtype (ExceptionType);
  GdbSendTSignal (SystemContext, GdbExceptionType);

  for( ; ; ) {
    ReceivePacket (gInBuffer, MAX_BUF_SIZE);

    switch (gInBuffer[0]) {
      case '?':
        GdbSendTSignal (SystemContext, GdbExceptionType);
        break;

      case 'c':
        ContinueAtAddress (SystemContext, gInBuffer);
        return;

      case 'g':
        ReadGeneralRegisters (SystemContext);
        break;

      case 'G':
        WriteGeneralRegisters (SystemContext, gInBuffer);
        break;

      case 'H':
        //Return "OK" packet since we don't have more than one thread.
        SendSuccess ();
        break;

      case 'm':
        ReadFromMemory (gInBuffer);
        break;

      case 'M':
        WriteToMemory (gInBuffer);
        break;

      case 'P':
        WriteNthRegister (SystemContext, gInBuffer);
        break;

      //
      // Still debugging this code. Not used in Darwin
      //
      case 'q':
        // General Query Packets
        if (AsciiStrnCmp (gInBuffer, "qSupported", 10) == 0) {
          // return what we currently support, we don't parse what gdb suports
          AsciiSPrint (gOutBuffer, MAX_BUF_SIZE, "qXfer:libraries:read+;PacketSize=%d", MAX_BUF_SIZE);
          SendPacket (gOutBuffer);
        } else if (AsciiStrnCmp (gInBuffer, "qXfer:libraries:read::", 22) == 0) {
          // ‘qXfer:libraries:read::offset,length
          // gInBuffer[22] is offset string, ++Ptr is length string’
          for (Ptr = &gInBuffer[22]; *Ptr != ','; Ptr++);

          // Not sure if multi-radix support is required. Currently only support decimal
          QxferLibrary (AsciiStrHexToUintn (&gInBuffer[22]), AsciiStrHexToUintn (++Ptr));
        } if (AsciiStrnCmp (gInBuffer, "qOffsets", 10) == 0) {
          AsciiSPrint (gOutBuffer, MAX_BUF_SIZE, "Text=1000;Data=f000;Bss=f000");
          SendPacket (gOutBuffer);
        } else {
          //Send empty packet
          SendNotSupported ();
        }
        break;

      case 's':
        SingleStep (SystemContext, gInBuffer);
        return;

      case 'z':
        RemoveBreakPoint (SystemContext, gInBuffer);
        break;

      case 'Z':
        InsertBreakPoint (SystemContext, gInBuffer);
        break;

      default:
        //Send empty packet
        SendNotSupported ();
        break;
    }
  }
}


/**
 Periodic callback for GDB. This function is used to catch a ctrl-c or other
 break in type command from GDB.

 @param SystemContext     Register content at time of the call
 **/
VOID
EFIAPI
GdbPeriodicCallBack (
  IN OUT EFI_SYSTEM_CONTEXT     SystemContext
  )
{
  //
  // gCtrlCBreakFlag may have been set from a previous F response package
  // and we set the global as we need to process it at a point where we
  // can update the system context. If we are in the middle of processing
  // a F Packet it is not safe to read the GDB serial stream so we need
  // to skip it on this check
  //
  if (!gCtrlCBreakFlag && !gProcessingFPacket) {
    //
    // Ctrl-C was not pending so grab any pending characters and see if they
    // are a Ctrl-c (0x03). If so set the Ctrl-C global.
    //
    while (TRUE) {
      if (!GdbIsCharAvailable ()) {
        //
        // No characters are pending so exit the loop
        //
        break;
      }

      if (GdbGetChar () == 0x03) {
        gCtrlCBreakFlag = TRUE;
        //
        // We have a ctrl-c so exit the loop
        //
        break;
      }
    }
  }

  if (gCtrlCBreakFlag) {
    //
    // Update the context to force a single step trap when we exit the GDB
    // stub. This will transfer control to GdbExceptionHandler () and let
    // us break into the program. We don't want to break into the GDB stub.
    //
    AddSingleStep (SystemContext);
    gCtrlCBreakFlag = FALSE;
  }
}
